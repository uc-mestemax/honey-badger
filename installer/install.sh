#!/bin/bash

# Check if the script is being run as 'tpot' or 'root'
currentUser=$(whoami)
if [ "$currentUser" == "tpot" ] || [ "$currentUser" == "root" ]; then
    echo "This script cannot be run as $currentUser. Please run as regular user"
    exit 1
fi

### Remove Docker Packages installed with distro ###

# Stop and disable docker service
sudo systemctl stop docker
sudo systemctl disable docker
sudo systemctl stop docker.socket
sudo systemctl mask docker.socket

sudo apt purge docker -y
sudo apt purge docker-engine -y
sudo apt purge docker.io -y
sudo apt purge containerd -y
sudo apt purge runc -y
sudo apt purge podman-docker -y
sudo apt purge podman -y

### Install Docker Packages ###

# Directory containing the downloaded .deb packages
packageFolder="./aptPackages/"

# Change to the specified directory
cd $packageFolder || exit

sudo apt install --no-install-recommends ./containerd.io* -y
sudo apt install --no-install-recommends ./docker-ce-cli* -y
sudo apt install --no-install-recommends ./docker-ce* -y
sudo apt install --no-install-recommends ./docker-compose-plugin* -y

cd .. || exit

# Sync Clock
# I don't know if I need to do this or not
# "hwclock --hctosys"

# Stop and disable docker service
sudo systemctl stop docker
sudo systemctl disable docker
sudo systemctl stop docker.socket
sudo systemctl mask docker.socket

### Adjust Configs Add Users Groups Etc ###

# Create T-Pot group
sudo groupadd -g 2000 tpot

# Create T-Pot user
sudo useradd -u 2000 -g tpot -s /bin/false -d /nonexistent -M tpot

# Disable ssh.socket unit
sudo systemctl stop ssh.socket
sudo systemctl disable ssh.socket

# Remove ssh.socket.conf file
sudo rm -f /etc/systemd/system/ssh.service.d/00-socket.conf

# Change SSH Port to 64295
sudo sed -i '$ a\Port 64295' /etc/ssh/sshd_config
sudo systemctl restart sshd

# Stop Resolved
sudo systemctl stop systemd-resolved

# Modify DNSStubListener in resolved.conf
sudo sed -i 's/^.*DNSStubListener=.*$/DNSStubListener=no/' /etc/systemd/resolved.conf

#Restart and Enable Services
sudo systemctl restart systemd-resolved
sudo systemctl unmask docker.socket
sudo systemctl enable docker
sudo systemctl restart docker
# Fix this parts
sudo systemctl restart ssh
sudo systemctl enable ssh
sudo systemctl restart sshd
sudo systemctl enable sshd

# Add the current user to the docker group
sudo usermod -aG docker "$currentUser"

# Add the current user to the tpot group
sudo usermod -aG tpot "$currentUser"

### Install tpot systemd service ###
pwd
serviceSource="./tpot.service"
tpotService="/etc/systemd/system/tpot.service"

# Change Permissions on Service
sudo cp "$serviceSource" "$tpotService"
sudo chown root:root "$tpotService"
sudo chmod 0755 "$tpotService"

# Start and enable service
sudo systemctl daemon-reload
sudo systemctl enable tpot.service
sudo systemctl start tpot.service

### Setup Daily Cron ###
# Setup a randomized daily reboot
# Maybe simplify the random time?
random_minute=$(shuf -i 0-59 -n 1)
random_hour=$(shuf -i 0-4 -n 1)
# Removed the part about the image prune
cron_job="bash -c 'systemctl stop tpot.service && docker container prune -f && docker volume prune -f; /usr/sbin/shutdown -r +1 \"T-Pot Daily Reboot\"'"

(crontab -l ; echo "$random_minute $random_hour * * * $cron_job") | crontab -

### Load Docker Images ###

pwd

$testPath="./docker/dockerImages/"

cd $testPath

pwd

# Directory containing your docker images
filePath="/home/user/installer/docker/dockerImages/"

# Loop through all .tar files in the directory
for file in $filePath*.tar; do
    fileName=$(basename "$file")
    sudo docker load -i "$file" # > /dev/null 2>&1
    if [ $? -eq 0 ]; then
        echo "Successfully loaded $fileName"
    else
        echo "Failed to load $fileName"
        exit 1
    fi
done

# ### Run Docker Compose ###
cd /home/user/installer/docker/

# Generates web user inside container I have no idea what for
sudo docker run -v $HOME/installer:/data --entrypoint bash -it -u $(id -u):$(id -g) dtagdevsec/tpotinit:24.04 "/opt/tpot/bin/genuser.sh"

sudo docker compose --env-file "./custom.env" -f "./customSensor.yml" up -d




