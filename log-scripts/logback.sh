#!/bin/bash

backup_dir="/root/packages"
snapshot_dir="$backup_dir/snapshots"
timestamp=$(date +"%Y-%m-%d_%H-%M-%S")
log_dir="/root/data"

# CREATE DIRECTORIES IF THEY DON'T EXIST
mkdir -p "$snapshot_dir"

# CALCULATE TIME 24HRS AGO
start_time=$(date --date="24 hours ago" "+%b %_d %H:%M")

# AWK THE LAST 24HRS OF LOGS INTO LOG FILE
awk -v start="$start_time" '{ if ($0 > start) print }' "$log_dir" > "$snapshot_dir/last_24hrs.log"

# COMPRESS LOGS 
tar -cz -C "$snapshot_dir" last_24hrs.log "$backup_dir/logs_$timestamp.tar.gz"

#backup_file="$output_dir/$(basename "$logfile" .log)_backup_$timestamp.tar.gz"

#TAR THE LOGFILE SNAPSHOT
#verbose "Creating backup tar file..."
#tar -czf "$backup_file" "$logfile"

# SUCCESS CHECK 
if [ $? -eq 0 ]; then
    echo "Packing Successful!"
else
    echo "Packing Failed!"
fi

# REMOVE PACKAGES OLDER THAN 24HRS
find "$backup_dir" -name "*.tar.gz" -mtime +1 -exec rm {} \;