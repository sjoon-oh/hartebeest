#!/bin/bash
#
# github.com/sjoon-oh/hartebeest
# Author: Sukjoon Oh, sjoon@kaist.ac.kr

project_home="hartebeest"
workspace_home=`basename $(pwd)`

warning='\033[0;31m[WARNING]\033[0m '

#
# Setting proj home
if [[ ${workspace_home} != ${project_home} ]]; then
    printf "${warning}Currently in wrong directory: `pwd`\n"
    exit
fi

export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$(pwd)/build/lib
gcc -o build/bin/core-test-c test/core-test-c.c -libverbs -lmemcached -L ./build/lib/ -lhartebeest

