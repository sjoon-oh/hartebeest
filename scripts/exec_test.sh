#!/bin/bash
#
# github.com/sjoon-oh/hartebeest
# Author: Sukjoon Oh, sjoon@kaist.ac.kr
# config_test.sh

project_home="hartebeest"
workspace_home=`basename $(pwd)`

warning='\033[0;31m[WARNING]\033[0m '
normalc='\033[0;32m[MESSAGE]\033[0m '

# args=$@

#
# Setting proj home
if [[ ${workspace_home} != ${project_home} ]]; then
    printf "${warning}Currently in wrong directory: `pwd`\n"
    exit
fi

export HARTEBEEST_PARTICIPANTS=0,1
export HARTEBEEST_NID=0
export HARTEBEEST_EXC_IP_PORT=123.123.123.123:9999
export HARTEBEEST_CONF_PATH=./hb_config.json

export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$(pwd)/build/lib

./build/bin/$1

