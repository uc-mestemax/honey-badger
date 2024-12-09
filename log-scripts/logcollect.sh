#!/bin/bash
export log_server="$1"
export log_port="$2"

hop_dir="/root/packages/"
collector_dir="/home/packages"


#CAPTURE LS COMMAND INTO AN ARRAY
files=($(ssh -p $log_port root@$log_server "ls -1 $hop_dir; exit"))

if [ -z "$files" ]; then
    echo "The folder is empty."
    exit 1
else
    #LOOP THROUGH ALL FILES IN THE HOP DIRECTORY
    echo -e "__________________________\nLOOPING THROUGH DIRECTORY..."
    for file in "${files[@]}"; do

        echo -e "__________________________\nINITIATING SCP..."
        #PULL ALL PACKAGES TO COLLECTOR
        scp -P "$log_port" root@$log_server:"$hop_dir/$file" $collector_dir

        # CHECK IF SCP WAS SUCCESSFUL
        if [[ $? -eq 0 ]]; then
            echo -e "__________________________\nTRANSFER SUCCESSFUL!!!"
            # DELETE THE FILE IF SCP WAS SUCCESSFUL
            ssh -p $log_port root@$log_server "rm $hop_dir/$file; exit"
            echo -e "DELETING PACKAGE: $file"
        else
            echo "Failed to transfer: $file"
        fi
    done
fi

echo -e "__________________________\nCOMPLETE!"

#ADD TO CRON TAB 
#*/5 * * * * sleep 250 && /root/logcollect.sh



