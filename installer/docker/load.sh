# Directory containing your docker images
filePath="./docker/dockerImages"

cd $filePath

pwd

# Loop through all .tar files in the directory
for file in *.tar; do
    echo "$filePath"
    echo "$file"
    fileName=$(basename "$file")
    echo "$fileName"
    sudo docker load -i "$file" # > /dev/null 2>&1
    if [ $? -eq 0 ]; then
        echo "Successfully loaded $fileName"
    else
        echo "Failed to load $fileName"
        exit 1
    fi
done