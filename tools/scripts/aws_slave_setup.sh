#!/bin/bash

# 1. install docker
sudo snap install docker
sleep 3
sudo chmod 666 /var/run/docker.sock

# 2. join swarm

# 3. chmod nfs path
sudo chmod 777 /mnt/efs/fs1/

