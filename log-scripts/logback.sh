#!/bin/bash

honeybadger_host="adminuser@172.172.133.81"
ssh_key="/home/vboxuser/.ssh/id_rsa"
ssh_port=64295

log_dir="/home/adminuser/tpotce/data"
dest_dir="/home/data"


# RSYNC OVER SSH
rsync -avz -e "ssh -i $ssh_key -p $ssh_port" "$honeybadger_host:$log_dir/" "$dest_dir/"
  

# SUCCESS CHECK & LOGGING
if [ $? -eq 0 ]; then
    echo "RSYNC Forward Successful!"
    echo "RSYNC Forward Successful at $timestamp" >> "/home/backup_records.txt"
else
    echo "RSYNC Forward Failed!"
    echo "RSYNC Forward Failed at $timestamp" >> "/home/backup_records.txt"
fi

# CRON JOB: */10 * * * * /home/logback.sh