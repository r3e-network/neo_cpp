variable "project_name" {
  description = "Name of the project"
  type        = string
  default     = "neo-cpp"
}

variable "environment" {
  description = "Environment name"
  type        = string
  default     = "production"
}

variable "aws_region" {
  description = "AWS region"
  type        = string
  default     = "us-east-1"
}

variable "vpc_cidr" {
  description = "CIDR block for VPC"
  type        = string
  default     = "10.0.0.0/16"
}

variable "private_subnets" {
  description = "List of private subnet CIDR blocks"
  type        = list(string)
  default     = ["10.0.1.0/24", "10.0.2.0/24", "10.0.3.0/24"]
}

variable "public_subnets" {
  description = "List of public subnet CIDR blocks"
  type        = list(string)
  default     = ["10.0.101.0/24", "10.0.102.0/24", "10.0.103.0/24"]
}

variable "instance_type" {
  description = "EC2 instance type for Neo nodes"
  type        = string
  default     = "c6i.xlarge"
}

variable "node_count" {
  description = "Number of Neo nodes to deploy"
  type        = number
  default     = 3
}

variable "root_volume_size" {
  description = "Size of root volume in GB"
  type        = number
  default     = 50
}

variable "data_volume_size" {
  description = "Size of data volume in GB"
  type        = number
  default     = 500
}

variable "key_pair_name" {
  description = "Name of EC2 key pair"
  type        = string
}

variable "neo_version" {
  description = "Neo C++ version to deploy"
  type        = string
  default     = "latest"
}

variable "neo_network" {
  description = "Neo network (mainnet/testnet)"
  type        = string
  default     = "mainnet"
}

variable "rpc_allowed_cidrs" {
  description = "CIDR blocks allowed to access RPC"
  type        = list(string)
  default     = ["0.0.0.0/0"]
}

variable "ssh_allowed_cidrs" {
  description = "CIDR blocks allowed to SSH"
  type        = list(string)
  default     = ["10.0.0.0/8"]
}

variable "ssl_certificate_arn" {
  description = "ARN of SSL certificate for ALB"
  type        = string
}

variable "enable_deletion_protection" {
  description = "Enable deletion protection for ALB"
  type        = bool
  default     = true
}

variable "asg_min_size" {
  description = "Minimum size of Auto Scaling Group"
  type        = number
  default     = 3
}

variable "asg_max_size" {
  description = "Maximum size of Auto Scaling Group"
  type        = number
  default     = 10
}

variable "asg_desired_capacity" {
  description = "Desired capacity of Auto Scaling Group"
  type        = number
  default     = 3
}

variable "alert_email" {
  description = "Email address for alerts"
  type        = string
}