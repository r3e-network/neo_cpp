#!/bin/bash

# Neo C++ Backup and Restore Script
# Comprehensive backup and disaster recovery system

set -e

# Configuration
OPERATION="${1:-backup}"  # backup, restore, verify
BACKUP_TYPE="${2:-full}"  # full, incremental, snapshot
BACKUP_DESTINATION="${BACKUP_DEST:-/backups}"
DATA_DIR="${DATA_DIR:-/data}"
CONFIG_DIR="${CONFIG_DIR:-/config}"
RETENTION_DAYS="${RETENTION_DAYS:-30}"
ENCRYPTION_ENABLED="${ENCRYPTION_ENABLED:-true}"
ENCRYPTION_KEY="${BACKUP_ENCRYPTION_KEY:-}"
S3_BUCKET="${S3_BUCKET:-}"
TIMESTAMP=$(date +%Y%m%d_%H%M%S)

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
CYAN='\033[0;36m'
NC='\033[0m'

echo "======================================"
echo "Neo C++ Backup & Restore System"
echo "======================================"
echo "Operation: $OPERATION"
echo "Type: $BACKUP_TYPE"
echo "Destination: $BACKUP_DESTINATION"
echo ""

# Helper functions
print_success() { echo -e "${GREEN}✓${NC} $1"; }
print_error() { echo -e "${RED}✗${NC} $1"; }
print_warning() { echo -e "${YELLOW}⚠${NC} $1"; }
print_info() { echo -e "${BLUE}ℹ${NC} $1"; }
print_progress() { echo -e "${CYAN}➤${NC} $1"; }

# Check prerequisites
check_prerequisites() {
    print_info "Checking prerequisites..."
    
    # Check required tools
    local required_tools=("tar" "gzip" "sha256sum")
    
    if [ "$ENCRYPTION_ENABLED" = "true" ]; then
        required_tools+=("openssl")
    fi
    
    if [ -n "$S3_BUCKET" ]; then
        required_tools+=("aws")
    fi
    
    for tool in "${required_tools[@]}"; do
        if ! command -v $tool &> /dev/null; then
            print_error "$tool is required but not installed"
            exit 1
        fi
    done
    
    # Check directories
    if [ "$OPERATION" = "backup" ]; then
        if [ ! -d "$DATA_DIR" ]; then
            print_error "Data directory not found: $DATA_DIR"
            exit 1
        fi
        
        mkdir -p "$BACKUP_DESTINATION"
        mkdir -p "$BACKUP_DESTINATION/metadata"
    fi
    
    print_success "Prerequisites check passed"
}

# Stop Neo node safely
stop_neo_node() {
    print_info "Stopping Neo node for consistent backup..."
    
    # Send graceful shutdown signal
    if pgrep -x "neo-node" > /dev/null; then
        kill -TERM $(pgrep -x "neo-node")
        
        # Wait for graceful shutdown
        local timeout=30
        while [ $timeout -gt 0 ] && pgrep -x "neo-node" > /dev/null; do
            sleep 1
            ((timeout--))
        done
        
        if pgrep -x "neo-node" > /dev/null; then
            print_warning "Force stopping Neo node"
            kill -9 $(pgrep -x "neo-node")
        fi
        
        print_success "Neo node stopped"
        return 0
    else
        print_info "Neo node is not running"
        return 1
    fi
}

# Start Neo node
start_neo_node() {
    print_info "Starting Neo node..."
    
    if [ -f "/usr/local/bin/neo-node" ]; then
        /usr/local/bin/neo-node --config "$CONFIG_DIR/config.json" &
        sleep 5
        
        if pgrep -x "neo-node" > /dev/null; then
            print_success "Neo node started"
        else
            print_error "Failed to start Neo node"
            return 1
        fi
    else
        print_warning "Neo node binary not found"
    fi
}

# Create backup metadata
create_backup_metadata() {
    local backup_name=$1
    local metadata_file="$BACKUP_DESTINATION/metadata/${backup_name}.json"
    
    # Get current block height
    local block_height=0
    if command -v neo-cli &> /dev/null; then
        block_height=$(neo-cli getblockcount 2>/dev/null || echo "0")
    fi
    
    # Get data size
    local data_size=$(du -sb "$DATA_DIR" | cut -f1)
    
    cat > "$metadata_file" << EOF
{
    "backup_name": "$backup_name",
    "timestamp": "$(date -Iseconds)",
    "type": "$BACKUP_TYPE",
    "block_height": $block_height,
    "data_size": $data_size,
    "data_dir": "$DATA_DIR",
    "config_dir": "$CONFIG_DIR",
    "encrypted": $([[ "$ENCRYPTION_ENABLED" == "true" ]] && echo "true" || echo "false"),
    "compression": "gzip",
    "checksum": "",
    "neo_version": "$(neo-node --version 2>/dev/null || echo 'unknown')",
    "hostname": "$(hostname)",
    "backup_tool_version": "1.0.0"
}
EOF
    
    print_success "Metadata created: $metadata_file"
}

# Perform full backup
backup_full() {
    print_progress "Performing full backup..."
    
    local backup_name="neo-backup-full-$TIMESTAMP"
    local backup_file="$BACKUP_DESTINATION/${backup_name}.tar.gz"
    
    # Stop node if running
    local node_was_running=false
    if stop_neo_node; then
        node_was_running=true
    fi
    
    # Create backup metadata
    create_backup_metadata "$backup_name"
    
    # Create tar archive
    print_info "Creating backup archive..."
    tar -czf "$backup_file.tmp" \
        -C / \
        "$DATA_DIR" \
        "$CONFIG_DIR" \
        2>/dev/null
    
    # Calculate checksum
    local checksum=$(sha256sum "$backup_file.tmp" | cut -d' ' -f1)
    
    # Encrypt if enabled
    if [ "$ENCRYPTION_ENABLED" = "true" ]; then
        if [ -z "$ENCRYPTION_KEY" ]; then
            print_error "Encryption enabled but no key provided"
            rm -f "$backup_file.tmp"
            [ "$node_was_running" = true ] && start_neo_node
            exit 1
        fi
        
        print_info "Encrypting backup..."
        openssl enc -aes-256-cbc -salt -in "$backup_file.tmp" -out "$backup_file" -pass pass:"$ENCRYPTION_KEY"
        rm -f "$backup_file.tmp"
    else
        mv "$backup_file.tmp" "$backup_file"
    fi
    
    # Update metadata with checksum
    local metadata_file="$BACKUP_DESTINATION/metadata/${backup_name}.json"
    sed -i "s/\"checksum\": \"\"/\"checksum\": \"$checksum\"/" "$metadata_file"
    
    # Upload to S3 if configured
    if [ -n "$S3_BUCKET" ]; then
        print_info "Uploading to S3..."
        aws s3 cp "$backup_file" "s3://$S3_BUCKET/backups/$backup_name.tar.gz"
        aws s3 cp "$metadata_file" "s3://$S3_BUCKET/backups/metadata/${backup_name}.json"
        print_success "Backup uploaded to S3"
    fi
    
    # Restart node if it was running
    [ "$node_was_running" = true ] && start_neo_node
    
    # Print summary
    local backup_size=$(du -h "$backup_file" | cut -f1)
    print_success "Full backup completed"
    echo ""
    echo "Backup Details:"
    echo "  Name: $backup_name"
    echo "  Size: $backup_size"
    echo "  Location: $backup_file"
    echo "  Checksum: $checksum"
}

# Perform incremental backup
backup_incremental() {
    print_progress "Performing incremental backup..."
    
    # Find last full backup
    local last_full=$(ls -t "$BACKUP_DESTINATION"/neo-backup-full-*.tar.gz 2>/dev/null | head -1)
    
    if [ -z "$last_full" ]; then
        print_warning "No full backup found, performing full backup instead"
        backup_full
        return
    fi
    
    local backup_name="neo-backup-incr-$TIMESTAMP"
    local backup_file="$BACKUP_DESTINATION/${backup_name}.tar.gz"
    local snapshot_file="$BACKUP_DESTINATION/.snapshot"
    
    # Create file list for incremental backup
    print_info "Finding changed files..."
    
    if [ -f "$snapshot_file" ]; then
        find "$DATA_DIR" -newer "$snapshot_file" -type f > /tmp/changed_files.txt
    else
        find "$DATA_DIR" -type f > /tmp/changed_files.txt
    fi
    
    local changed_count=$(wc -l < /tmp/changed_files.txt)
    
    if [ "$changed_count" -eq 0 ]; then
        print_info "No changes detected since last backup"
        return
    fi
    
    print_info "Found $changed_count changed files"
    
    # Create incremental backup
    tar -czf "$backup_file" -T /tmp/changed_files.txt 2>/dev/null
    
    # Update snapshot timestamp
    touch "$snapshot_file"
    
    # Create metadata
    create_backup_metadata "$backup_name"
    
    local backup_size=$(du -h "$backup_file" | cut -f1)
    print_success "Incremental backup completed"
    echo "  Size: $backup_size"
    echo "  Changed files: $changed_count"
    
    rm -f /tmp/changed_files.txt
}

# Create snapshot backup
backup_snapshot() {
    print_progress "Creating snapshot backup..."
    
    # Check if filesystem supports snapshots
    local fs_type=$(df -T "$DATA_DIR" | tail -1 | awk '{print $2}')
    
    case $fs_type in
        btrfs)
            print_info "Using btrfs snapshot"
            local snapshot_name="neo-snapshot-$TIMESTAMP"
            btrfs subvolume snapshot -r "$DATA_DIR" "$BACKUP_DESTINATION/snapshots/$snapshot_name"
            print_success "Btrfs snapshot created: $snapshot_name"
            ;;
            
        zfs)
            print_info "Using ZFS snapshot"
            local dataset=$(df "$DATA_DIR" | tail -1 | awk '{print $1}')
            local snapshot_name="${dataset}@neo-backup-$TIMESTAMP"
            zfs snapshot "$snapshot_name"
            print_success "ZFS snapshot created: $snapshot_name"
            ;;
            
        ext4|xfs)
            print_warning "Filesystem does not support snapshots, using LVM if available"
            
            # Check for LVM
            if command -v lvcreate &> /dev/null; then
                local vg=$(lvdisplay $(df "$DATA_DIR" | tail -1 | awk '{print $1}') 2>/dev/null | grep "VG Name" | awk '{print $3}')
                local lv=$(lvdisplay $(df "$DATA_DIR" | tail -1 | awk '{print $1}') 2>/dev/null | grep "LV Name" | awk '{print $3}')
                
                if [ -n "$vg" ] && [ -n "$lv" ]; then
                    lvcreate -L5G -s -n "neo-snapshot-$TIMESTAMP" "/dev/$vg/$lv"
                    print_success "LVM snapshot created"
                else
                    print_error "LVM not configured for data directory"
                    return 1
                fi
            else
                print_error "Snapshot not supported on $fs_type without LVM"
                return 1
            fi
            ;;
            
        *)
            print_error "Unsupported filesystem: $fs_type"
            return 1
            ;;
    esac
}

# Restore from backup
restore_backup() {
    local backup_file="${2:-}"
    
    if [ -z "$backup_file" ]; then
        # List available backups
        print_info "Available backups:"
        ls -lh "$BACKUP_DESTINATION"/*.tar.gz 2>/dev/null | awk '{print $9, $5}'
        
        echo ""
        read -p "Enter backup file path: " backup_file
    fi
    
    if [ ! -f "$backup_file" ]; then
        print_error "Backup file not found: $backup_file"
        exit 1
    fi
    
    print_progress "Restoring from backup: $(basename $backup_file)"
    
    # Stop node if running
    stop_neo_node || true
    
    # Backup current data
    if [ -d "$DATA_DIR" ]; then
        print_info "Backing up current data..."
        mv "$DATA_DIR" "${DATA_DIR}.backup-$TIMESTAMP"
    fi
    
    # Decrypt if needed
    local restore_file="$backup_file"
    if [ "$ENCRYPTION_ENABLED" = "true" ]; then
        if [ -z "$ENCRYPTION_KEY" ]; then
            print_error "Encryption key required for restore"
            exit 1
        fi
        
        print_info "Decrypting backup..."
        restore_file="/tmp/restore-$TIMESTAMP.tar.gz"
        openssl enc -aes-256-cbc -d -in "$backup_file" -out "$restore_file" -pass pass:"$ENCRYPTION_KEY"
    fi
    
    # Extract backup
    print_info "Extracting backup..."
    tar -xzf "$restore_file" -C / 2>/dev/null
    
    # Clean up temp file
    [ "$restore_file" != "$backup_file" ] && rm -f "$restore_file"
    
    # Verify restoration
    if [ -d "$DATA_DIR" ]; then
        print_success "Data restored successfully"
        
        # Start node
        start_neo_node
        
        # Verify node is operational
        sleep 10
        if curl -s http://localhost:10332/health > /dev/null 2>&1; then
            print_success "Node is operational after restore"
        else
            print_warning "Node health check failed - manual intervention may be required"
        fi
    else
        print_error "Restore failed - data directory not found"
        
        # Restore original data
        if [ -d "${DATA_DIR}.backup-$TIMESTAMP" ]; then
            mv "${DATA_DIR}.backup-$TIMESTAMP" "$DATA_DIR"
        fi
        exit 1
    fi
}

# Verify backup integrity
verify_backup() {
    local backup_file="${2:-}"
    
    if [ -z "$backup_file" ]; then
        print_error "Backup file path required for verification"
        exit 1
    fi
    
    if [ ! -f "$backup_file" ]; then
        print_error "Backup file not found: $backup_file"
        exit 1
    fi
    
    print_progress "Verifying backup: $(basename $backup_file)"
    
    # Check metadata
    local backup_name=$(basename "$backup_file" .tar.gz)
    local metadata_file="$BACKUP_DESTINATION/metadata/${backup_name}.json"
    
    if [ -f "$metadata_file" ]; then
        print_info "Checking metadata..."
        local stored_checksum=$(jq -r '.checksum' "$metadata_file")
        
        # Decrypt if needed for checksum verification
        local verify_file="$backup_file"
        if [ "$(jq -r '.encrypted' "$metadata_file")" = "true" ]; then
            if [ -z "$ENCRYPTION_KEY" ]; then
                print_warning "Cannot verify checksum - backup is encrypted"
            else
                verify_file="/tmp/verify-$TIMESTAMP.tar.gz"
                openssl enc -aes-256-cbc -d -in "$backup_file" -out "$verify_file" -pass pass:"$ENCRYPTION_KEY" 2>/dev/null
            fi
        fi
        
        if [ -f "$verify_file" ]; then
            local actual_checksum=$(sha256sum "$verify_file" | cut -d' ' -f1)
            [ "$verify_file" != "$backup_file" ] && rm -f "$verify_file"
            
            if [ "$stored_checksum" = "$actual_checksum" ]; then
                print_success "Checksum verification passed"
            else
                print_error "Checksum verification failed"
                exit 1
            fi
        fi
    else
        print_warning "No metadata found for backup"
    fi
    
    # Test extraction
    print_info "Testing backup extraction..."
    
    local test_dir="/tmp/backup-test-$TIMESTAMP"
    mkdir -p "$test_dir"
    
    if tar -tzf "$backup_file" > /dev/null 2>&1; then
        print_success "Backup archive is valid"
    else
        print_error "Backup archive is corrupted"
        rm -rf "$test_dir"
        exit 1
    fi
    
    rm -rf "$test_dir"
    
    print_success "Backup verification completed successfully"
}

# Cleanup old backups
cleanup_old_backups() {
    print_progress "Cleaning up old backups..."
    
    local deleted_count=0
    local freed_space=0
    
    # Find and delete old backups
    while IFS= read -r backup_file; do
        local file_size=$(stat -c%s "$backup_file" 2>/dev/null || echo 0)
        rm -f "$backup_file"
        
        # Remove associated metadata
        local backup_name=$(basename "$backup_file" .tar.gz)
        rm -f "$BACKUP_DESTINATION/metadata/${backup_name}.json"
        
        ((deleted_count++))
        freed_space=$((freed_space + file_size))
    done < <(find "$BACKUP_DESTINATION" -name "neo-backup-*.tar.gz" -mtime +$RETENTION_DAYS -type f)
    
    if [ $deleted_count -gt 0 ]; then
        local freed_space_human=$(numfmt --to=iec-i --suffix=B $freed_space)
        print_success "Deleted $deleted_count old backups, freed $freed_space_human"
    else
        print_info "No old backups to delete"
    fi
    
    # Clean up old S3 backups if configured
    if [ -n "$S3_BUCKET" ]; then
        print_info "Cleaning up S3 backups..."
        aws s3 ls "s3://$S3_BUCKET/backups/" | while read -r line; do
            local file_date=$(echo "$line" | awk '{print $1}')
            local file_name=$(echo "$line" | awk '{print $4}')
            
            if [ -n "$file_date" ] && [ -n "$file_name" ]; then
                local file_age=$(( ($(date +%s) - $(date -d "$file_date" +%s)) / 86400 ))
                
                if [ $file_age -gt $RETENTION_DAYS ]; then
                    aws s3 rm "s3://$S3_BUCKET/backups/$file_name"
                    print_info "Deleted S3 backup: $file_name"
                fi
            fi
        done
    fi
}

# Main execution
main() {
    # Check prerequisites
    check_prerequisites
    
    case $OPERATION in
        backup)
            case $BACKUP_TYPE in
                full)
                    backup_full
                    ;;
                incremental)
                    backup_incremental
                    ;;
                snapshot)
                    backup_snapshot
                    ;;
                *)
                    print_error "Unknown backup type: $BACKUP_TYPE"
                    exit 1
                    ;;
            esac
            
            # Cleanup old backups
            cleanup_old_backups
            ;;
            
        restore)
            restore_backup "$@"
            ;;
            
        verify)
            verify_backup "$@"
            ;;
            
        cleanup)
            cleanup_old_backups
            ;;
            
        list)
            print_info "Available backups:"
            ls -lh "$BACKUP_DESTINATION"/*.tar.gz 2>/dev/null | awk '{print $9, $5, $6, $7, $8}' || echo "No backups found"
            
            if [ -n "$S3_BUCKET" ]; then
                echo ""
                print_info "S3 backups:"
                aws s3 ls "s3://$S3_BUCKET/backups/" --human-readable
            fi
            ;;
            
        *)
            print_error "Unknown operation: $OPERATION"
            echo ""
            echo "Usage: $0 [operation] [type/file]"
            echo "Operations:"
            echo "  backup [full|incremental|snapshot] - Create backup"
            echo "  restore [backup_file] - Restore from backup"
            echo "  verify [backup_file] - Verify backup integrity"
            echo "  cleanup - Remove old backups"
            echo "  list - List available backups"
            exit 1
            ;;
    esac
    
    print_success "Operation completed successfully"
}

# Run main function
main "$@"