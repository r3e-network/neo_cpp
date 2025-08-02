terraform {
  required_version = ">= 1.0"
  
  required_providers {
    aws = {
      source  = "hashicorp/aws"
      version = "~> 5.0"
    }
  }
  
  backend "s3" {
    bucket = "neo-terraform-state"
    key    = "neo-cpp/terraform.tfstate"
    region = "us-east-1"
  }
}

provider "aws" {
  region = var.aws_region
}

# VPC Configuration
module "vpc" {
  source = "terraform-aws-modules/vpc/aws"
  version = "~> 5.0"
  
  name = "${var.project_name}-vpc"
  cidr = var.vpc_cidr
  
  azs             = data.aws_availability_zones.available.names
  private_subnets = var.private_subnets
  public_subnets  = var.public_subnets
  
  enable_nat_gateway = true
  enable_vpn_gateway = false
  enable_dns_hostnames = true
  enable_dns_support = true
  
  tags = local.common_tags
}

# Security Groups
resource "aws_security_group" "neo_node" {
  name_prefix = "${var.project_name}-node-"
  description = "Security group for Neo nodes"
  vpc_id      = module.vpc.vpc_id
  
  # P2P port
  ingress {
    from_port   = 10332
    to_port     = 10332
    protocol    = "tcp"
    cidr_blocks = ["0.0.0.0/0"]
    description = "Neo P2P"
  }
  
  # RPC port (restricted)
  ingress {
    from_port   = 10334
    to_port     = 10334
    protocol    = "tcp"
    cidr_blocks = var.rpc_allowed_cidrs
    description = "Neo RPC"
  }
  
  # SSH (restricted)
  ingress {
    from_port   = 22
    to_port     = 22
    protocol    = "tcp"
    cidr_blocks = var.ssh_allowed_cidrs
    description = "SSH access"
  }
  
  # Prometheus metrics
  ingress {
    from_port   = 9090
    to_port     = 9090
    protocol    = "tcp"
    security_groups = [aws_security_group.monitoring.id]
    description = "Prometheus metrics"
  }
  
  egress {
    from_port   = 0
    to_port     = 0
    protocol    = "-1"
    cidr_blocks = ["0.0.0.0/0"]
    description = "Allow all outbound"
  }
  
  tags = merge(local.common_tags, {
    Name = "${var.project_name}-node-sg"
  })
}

resource "aws_security_group" "monitoring" {
  name_prefix = "${var.project_name}-monitoring-"
  description = "Security group for monitoring services"
  vpc_id      = module.vpc.vpc_id
  
  tags = merge(local.common_tags, {
    Name = "${var.project_name}-monitoring-sg"
  })
}

# EC2 Instances
resource "aws_instance" "neo_node" {
  count = var.node_count
  
  ami           = data.aws_ami.ubuntu.id
  instance_type = var.instance_type
  key_name      = var.key_pair_name
  
  subnet_id                   = module.vpc.private_subnets[count.index % length(module.vpc.private_subnets)]
  vpc_security_group_ids      = [aws_security_group.neo_node.id]
  associate_public_ip_address = false
  
  root_block_device {
    volume_type = "gp3"
    volume_size = var.root_volume_size
    iops        = 3000
    throughput  = 125
    encrypted   = true
  }
  
  ebs_block_device {
    device_name = "/dev/sdf"
    volume_type = "gp3"
    volume_size = var.data_volume_size
    iops        = 10000
    throughput  = 500
    encrypted   = true
    delete_on_termination = false
  }
  
  user_data = templatefile("${path.module}/user_data.sh", {
    neo_version = var.neo_version
    neo_network = var.neo_network
  })
  
  tags = merge(local.common_tags, {
    Name = "${var.project_name}-node-${count.index + 1}"
    Type = "neo-node"
  })
  
  lifecycle {
    ignore_changes = [ami]
  }
}

# Application Load Balancer for RPC
resource "aws_lb" "neo_rpc" {
  name               = "${var.project_name}-rpc-alb"
  internal           = false
  load_balancer_type = "application"
  security_groups    = [aws_security_group.alb.id]
  subnets           = module.vpc.public_subnets
  
  enable_deletion_protection = var.enable_deletion_protection
  enable_http2              = true
  
  tags = local.common_tags
}

resource "aws_lb_target_group" "neo_rpc" {
  name     = "${var.project_name}-rpc-tg"
  port     = 10334
  protocol = "HTTP"
  vpc_id   = module.vpc.vpc_id
  
  health_check {
    enabled             = true
    healthy_threshold   = 2
    unhealthy_threshold = 2
    timeout             = 5
    interval            = 30
    path                = "/health"
    matcher             = "200"
  }
  
  tags = local.common_tags
}

resource "aws_lb_listener" "neo_rpc" {
  load_balancer_arn = aws_lb.neo_rpc.arn
  port              = "443"
  protocol          = "HTTPS"
  ssl_policy        = "ELBSecurityPolicy-TLS13-1-2-2021-06"
  certificate_arn   = var.ssl_certificate_arn
  
  default_action {
    type             = "forward"
    target_group_arn = aws_lb_target_group.neo_rpc.arn
  }
}

# Auto Scaling
resource "aws_autoscaling_group" "neo_nodes" {
  name_prefix          = "${var.project_name}-asg-"
  vpc_zone_identifier  = module.vpc.private_subnets
  target_group_arns    = [aws_lb_target_group.neo_rpc.arn]
  health_check_type    = "ELB"
  health_check_grace_period = 300
  
  min_size         = var.asg_min_size
  max_size         = var.asg_max_size
  desired_capacity = var.asg_desired_capacity
  
  launch_template {
    id      = aws_launch_template.neo_node.id
    version = "$Latest"
  }
  
  tag {
    key                 = "Name"
    value               = "${var.project_name}-node"
    propagate_at_launch = true
  }
  
  dynamic "tag" {
    for_each = local.common_tags
    content {
      key                 = tag.key
      value               = tag.value
      propagate_at_launch = true
    }
  }
}

# Launch Template
resource "aws_launch_template" "neo_node" {
  name_prefix   = "${var.project_name}-lt-"
  image_id      = data.aws_ami.ubuntu.id
  instance_type = var.instance_type
  key_name      = var.key_pair_name
  
  vpc_security_group_ids = [aws_security_group.neo_node.id]
  
  block_device_mappings {
    device_name = "/dev/sda1"
    ebs {
      volume_type = "gp3"
      volume_size = var.root_volume_size
      encrypted   = true
    }
  }
  
  block_device_mappings {
    device_name = "/dev/sdf"
    ebs {
      volume_type = "gp3"
      volume_size = var.data_volume_size
      iops        = 10000
      throughput  = 500
      encrypted   = true
    }
  }
  
  user_data = base64encode(templatefile("${path.module}/user_data.sh", {
    neo_version = var.neo_version
    neo_network = var.neo_network
  }))
  
  tag_specifications {
    resource_type = "instance"
    tags = merge(local.common_tags, {
      Name = "${var.project_name}-node"
    })
  }
}

# CloudWatch Alarms
resource "aws_cloudwatch_metric_alarm" "high_cpu" {
  alarm_name          = "${var.project_name}-high-cpu"
  comparison_operator = "GreaterThanThreshold"
  evaluation_periods  = "2"
  metric_name         = "CPUUtilization"
  namespace           = "AWS/EC2"
  period              = "300"
  statistic           = "Average"
  threshold           = "80"
  alarm_description   = "This metric monitors cpu utilization"
  alarm_actions       = [aws_sns_topic.alerts.arn]
  
  dimensions = {
    AutoScalingGroupName = aws_autoscaling_group.neo_nodes.name
  }
}

# SNS Topic for Alerts
resource "aws_sns_topic" "alerts" {
  name = "${var.project_name}-alerts"
  
  tags = local.common_tags
}

resource "aws_sns_topic_subscription" "alerts_email" {
  topic_arn = aws_sns_topic.alerts.arn
  protocol  = "email"
  endpoint  = var.alert_email
}

# Data sources
data "aws_availability_zones" "available" {
  state = "available"
}

data "aws_ami" "ubuntu" {
  most_recent = true
  owners      = ["099720109477"] # Canonical
  
  filter {
    name   = "name"
    values = ["ubuntu/images/hvm-ssd/ubuntu-jammy-22.04-amd64-server-*"]
  }
  
  filter {
    name   = "virtualization-type"
    values = ["hvm"]
  }
}

# Locals
locals {
  common_tags = {
    Project     = var.project_name
    Environment = var.environment
    ManagedBy   = "Terraform"
  }
}