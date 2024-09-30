# Senior Design - Honey Badger - HoneyNet

![Bearcat Badger](/src/bearcat-badger.png)

## Problem Statement

For our Senior Design project, we sought out to build an automation framework for deploying a reusable, composble honeynet environment for three purposes
- Researching and Analyzing Current Threat Landscape
- Allowing for honeynet deployment in strategic network segments to find indictaors of compromise
- A disposble, sandbox environment for offensive security and red team use

## Project Roadmap

![Project Roadmap](/src/Senior%20Design%20Roadmap.drawio.png)

## Project Documentation

### Launching the Infrastructure

#### Getting Started

### Understanding the composition of the project


This project is structured in three folders

- Scripts contains any bash scripts neccessary for initialzing the VM and installing ansible depencies
- Ansible Contains the playbook neccessary for  automating the install of tpot, Wireguard, and Docker
- Terraform contains the infrastructure as code required for deploying a templatized virtual machine into our Azure cloud environment.