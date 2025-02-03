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

# Add the current user to the docker group
sudo usermod -aG docker "$currentUser"

sudo systemctl unmask docker.socket
sudo systemctl enable docker
sudo systemctl restart docker

### Load Docker Images ###

# Directory containing your docker images
filePath="./docker/dockerImages/"

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

# Generates web user inside container I have no idea what for
#sudo docker run -v $HOME/hiveInstall:/data --entrypoint bash -it -u $(id -u):$(id -g) dtagdevsec/tpotinit:24.04 "/opt/tpot/bin/genuser.sh"

sudo docker compose --env-file "./docker/custom.env" -f "./docker/customHive.yml" up -d