#!/usr/bin/env bash

# Note all these filepaths are inside the tpotInit container

# 
COMPOSE="/tmp/tpot/docker-compose.yml"

# Writes the stdIn output to a log file (this might be on the host)
exec > >(tee /data/tpotinit.log) 2>&1

# Function to handle SIGTERM

# what is SIGTERM?
# SIGTERM is a signal used in Unix-like operating systems to request the graceful termination of a process.
# It stands for "Signal Terminate". When a process receives a SIGTERM signal, 
# it has the opportunity to clean up resources, save its state, and perform any other necessary shutdown tasks before terminating.

cleanup() {
  echo "# SIGTERM received, cleaning up ..."
  echo
  echo "## ... removing firewall rules."
  /opt/tpot/bin/rules.sh ${COMPOSE} unset
  echo
  kill -TERM "$PID"
  rm -f /tmp/success
  echo "# Cleanup done."
  echo
}
trap cleanup SIGTERM

# Takes in a enviormnet variable name, checks if empty, stops script if is does nothing if not.
check_var() {
    local var_name="$1"
    local var_value=$(eval echo \$$var_name)

    # Check if variable is set and not empty
    if [[ -z "$var_value" ]];
      then
        echo "# Error: $var_name is not set or empty. Please check T-Pot .env config."
        echo
        echo "# Aborting"
        exit 1
    fi
}

# Function to check for potentially unsafe characters in most variables (variable name santization)
check_safety() {
    local var_name="$1"
    local var_value=$(eval echo \$$var_name)

    # General safety check for most variables
    if [[ $var_value =~ [^a-zA-Z0-9_/.:-] ]];
      then
        echo "# Error: Unsafe characters detected in $var_name. Please check T-Pot .env config."
        echo
        echo "# Aborting"
        exit 1
    fi
}

validate_ip_or_domain() {
    local myCHECK=$1

    # Regular expression for validating IPv4 addresses
    local ipv4Regex='^(([0-9]|[1-9][0-9]|1[0-9]{2}|2[0-4][0-9]|25[0-5])\.){3}([0-9]|[1-9][0-9]|1[0-9]{2}|2[0-4][0-9]|25[0-5])$'

    # Regular expression for validating domain names (including subdomains)
    local domainRegex='^(([a-zA-Z0-9]|[a-zA-Z0-9][a-zA-Z0-9\-]*[a-zA-Z0-9])\.)*([A-Za-z0-9]|[A-Za-z0-9][A-Za-z0-9\-]*[A-Za-z0-9])$'

    # Check if TPOT_HIVE_IP matches IPv4 or domain name
    if [[ ${myCHECK} =~ $ipv4Regex ]]; then
        echo "${myCHECK} is a valid IPv4 address."
    elif [[ ${myCHECK} =~ $domainRegex ]]; then
        echo "${myCHECK} is a valid domain name."
    else
        echo "# Error: $myCHECK is not a valid IPv4 address or domain name. Please check T-Pot .env config."
        echo
        echo "# Aborting"
        exit 1
    fi
}

# Defining Update permissions function
update_permissions() {
	echo
	echo "# Updating permissions ..."
	echo
	chown -R tpot:tpot /data
	chmod -R 770 /data
}

# Calling Update permissions function
update_permissions

# Array of enviornment variables
tpot_vars=("TPOT_PERSISTENCE" "TPOT_REPO" "TPOT_PULL_POLICY")

# Validate input of environment variables
for var in "${tpot_vars[@]}"; do
    check_var "$var"
    check_safety "$var"
done

# # Sensor Tpot type
# check_var "TPOT_HIVE_USER"
# check_var "TPOT_HIVE_IP"
# validate_ip_or_domain "$TPOT_HIVE_IP"
# WEB_USER=""

echo

# Removing blackhole feature
echo
echo "# Removing Blackhole routes."
/opt/tpot/bin/blackhole.sh del
echo
echo
echo "# Blackhole is not active."

# Get IP
echo
echo "# Updating IP Info ..."
echo
/opt/tpot/bin/updateip.sh

# Calling Update permissions function
update_permissions

# Update interface settings (p0f and Suricata) and setup iptables to support NFQ based honeypots (glutton, honeytrap)
echo
echo "# Get IF, disable offloading, enable promiscious mode for p0f and suricata ..."
echo
ethtool --offload $(/sbin/ip address | grep "^2: " | awk '{ print $2 }' | tr -d [:punct:]) rx off tx off
ethtool -K $(/sbin/ip address | grep "^2: " | awk '{ print $2 }' | tr -d [:punct:]) gso off gro off
ip link set $(/sbin/ip address | grep "^2: " | awk '{ print $2 }' | tr -d [:punct:]) promisc on
echo
echo "# Adding firewall rules ..."
echo
/opt/tpot/bin/rules.sh ${COMPOSE} set

# Display open ports
echo
echo "# This is a list of open ports on the host (netstat -tulpen)."
echo "# Make sure there are no conflicting ports by checking the docker compose file."
echo "# Conflicting ports will prevent the startup of T-Pot."
echo
netstat -tulpen | grep -Eo ':([0-9]+)' | cut -d ":" -f 2 | uniq
echo

# Done
echo "Starting"
touch /tmp/success

# We want to see true source for UDP packets in container
sleep 60
echo "# Dropping UDP connection tables to improve visibility of true source IPs."
/usr/sbin/conntrack -D -p udp