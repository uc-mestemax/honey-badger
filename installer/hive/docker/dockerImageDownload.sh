#!/bin/bash

# Check if the script is run with sudo
if [[ $EUID -ne 0 ]]; then
   echo "This script must be run as root (use sudo)."
   exit 0
fi

function display_usage() {
  echo "Usage: ./my_script.sh -d -c -o <output_dir>"
  echo "Options:"
  echo "  -o <output_dir>: (Mandatory) Specify the output directory where the images will be stored."
  echo "  -d This will delete all your existing docker images"
  echo "  -c This will clear your docker images that you just pulled and will only store the images as .tar files"
  echo "  -h: Display this help information."
  exit 0;
}

output=""
deleteExisting=false
cleanup=false
invalidArgument=""

while getopts ":o:dch" flag; do
    case $flag in
        h) display_usage;;
        o) 
            if [ -n "$OPTARG" ] && [[ "$OPTARG" != -* ]]; then
                output=${OPTARG}
            else
                echo "Option -o requires an argument."
                echo ""
                display_usage
            fi
            ;;
        d) deleteExisting=true;;
        c) cleanup=true;;
        \?) echo "Invalid option: -$OPTARG" >&2; exit 1;;
    esac
done

if [ -z "$output" ]; then
    echo "Option -o is required."
    echo ""
    display_usage
elif [ ! -d "$output" ]; then
  echo "Directory $output does not exist. Exiting the script."
  exit 0;
fi

dockerImages=(
    # Tpot Init
    dtagdevsec/tpotinit:24.04

    # Tools
    "dtagdevsec/beelzebub:24.04"
    "dtagdevsec/elasticsearch:24.04"
    "dtagdevsec/kibana:24.04"
    "dtagdevsec/logstash:24.04"
    "dtagdevsec/redis:24.04"
    "dtagdevsec/map:24.04"
    "dtagdevsec/ewsposter:24.04"
    "dtagdevsec/nginx:24.04"
    "dtagdevsec/spiderfoot:24.04"
)

for image in "${dockerImages[@]}"; do
    echo "$image"
    docker pull "$image"
done

regex='s/[\/:.]/-/g'

for image in "${dockerImages[@]}"; do
    fileName=$(echo "$image" | sed "$regex")
    echo "$fileName"
    fullPath="$output$fileName.tar"
    docker save "$image" > "$fullPath"
done

if $deleteExisting; then
    image_ids=$(docker images -q)

    if [ -n "$image_ids" ]; then
    for id in $image_ids; do
        echo "Removing image ID: $id"
        docker rmi "$id"
    done
    fi
fi

if $cleanup; then
    for image in "${dockerImages[@]}"; do
        docker rmi "$image"
    done
fi