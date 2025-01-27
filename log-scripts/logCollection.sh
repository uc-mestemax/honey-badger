#!/bin/bash

# ssh-keygen -t rsa -b 4096 -f ~/.ssh/192-168-1-1 -N ""

# ssh-copy-id -i ~/.ssh/192-168-1-1.pub root@192.168.1.1

remoteIps=(
  "192.168.1.1"
  #"192.168.1.2"
  #"192.168.1.3"
  #"192.168.1.4"
  #"192.168.1.5"
)

# For each ip in array
for remoteIp in "${remoteIps[@]}"; do

  name="${remoteIps//./-}"

  port="22"

  localUser="user"
  localDirectory="/home/$localUser/data/$name/data/"
  privateKey="/home/$localUser/.ssh/$name"

  remoteUser="root"
  remoteDirectory="/home/user/data/"

  # If there is no private key it will end the script
  if [ ! -f "$privateKey" ]; then
    echo "Error: Private Key $privateKey not found."
    exit 1
  fi

  rsync -avz --ignore-existing --mkpath -e "ssh -i $privateKey -p $port" $remoteUser@$remoteIp:$remoteDirectory $localDirectory

done

installCronJob() {
  pwd=$(pwd)

  # Path to your script
  scriptFilePath="$pwd/logCollection.sh"

  # Path to cron job log file
  cronLogFilePath="$pwd/logCollectionCronLog.log"

  [ ! -f "$LOG_FILE" ] && touch $cronLogFilePath

  # Cron job entry for running the script every 5 minutes
  cronJob="*/5 * * * * $scriptFilePath >> $cronLogFilePath 2>&1"

  # Check if the cron job already exists and store the result in a variable
  isCronJob=$(crontab -l | grep -F "$cronJob" > /dev/null; echo $?)

  # Add the cron job if it doesn't exist
  if [ $isCronJob -ne 0 ]; then
    (crontab -l ; echo "$cronJob") | crontab -
  fi
}

installCronJob
