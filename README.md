![logo](./logo/hartebeest-60.png)
# Project hartebeest

<!-- 
github.com/sjoon-oh/hartebeest
Author: Sukjoon Oh, sjoon@kaist.ac.kr
build-all.cpp 
-->

Project **Hartebeest** is an RDMA configuration module. 

Hartebeest is just a raw skeleton code for now. It just targets an InbiniBand based RDMA RC communication. Other features may be added in the future. 

The binary `demo2.cpp` is tested on:
- CentOS7, 5.11.1-1.el7.elrepo.x86_64
- Mellanox Driver 5.7-1.0.2.0-rhel7.9-x86_64
- Mellanox ConnectX-5

This document briefly describes the project. Author: Sukjoon Oh, [sjoon@kaist.ac.kr](mailto:sjoon@kaist.ac.kr).

## Description

**Hartebeest** is a configurator of RDMA communication. Each machine in a network should know the remotes' RDMA resource status before initiating RDMA communication. However, it is upto an application programmer to fetch such status information. InfiniBand specification do not define a way to notify the status, or provide any mechanism to do such operation. 

**Hartebeest** exchanges the machines' status with simple socket. A node with the lowest node ID plays as a server, which gathers all other clients' RDMA resource status. RDMA resource allocation information changes everytime an application launches. At the first launch of the application, these status should be shared among the participant machines. This project targets machines in a local network (e.g. data centers), thus it does not provide any security features. It is assumed that there are no Byzantine players, or intervention from outside.

**Hartebeest** is designed as single header-only library. If you use the provided `CMakeFile.txt`, it compiles a library that can be used with a single `include` of `rdma-conf.hpp`. 

## Notes on Maintenance

### RdmaConfigurator

This instance holds all RDMA-related managers. Each manager handles single-type `ibv*` api instances.

- class `DeviceManager` : Manages HCA
- class `MrManager` : Manages memory-related stuff, i.e. Memory Region, Buffers
- class `QueueManager` : Provides basic IBV queues APIs.
- class `PdManager` : Manages Protection Domains.

`RdmaConfigurator` works as a coordinator of these instances. It provides few member functions, so that all managers may interact without crucial errors.

A program may have multiple IBV-resources, such as many Queue Pairs, Protection Domains, and much more. Single `RdmaConfigurator` instance handles all of them in one instance, thus this is not designed to have many `RdmaConfigurator` objects in a program. 

`RdmaConfigurator` may expose the resources it handles. `RdmaConfigurator` assumes that there will be some cases when an application want to explicitly control them. Still, the instance will think that it is the owner of all of the allocated resources. Make sure not to modify the management mechansim or deallocate the resource outside of the instance fence.


### Naming Convention

The inteface naming convention is designed to have:
- `do**` : These are management functions. Does something important. Directly updates its member. 
- `is**` : Check status.
- `get**` : Returns reference/value of a member. Members follow the underscore, methods follow the CamelCase naming convention.
- `__**` : These underscored functions are only used inside of an instance. The default access setting is `private`.


### ConfigFileExchanger

All the configuration or status are recorded to the filesystem in JSON form. For parsing the JSON file, **hartebeest** utilizes an open source JSON parser, [nlohmann/json](https://github.com/nlohmann/json).

Class `ConfigFileExchanger` is responsible for sharing all the RDMA resource status in a network. Participants stated in the initial configuration file will share all Queue Pairs, Memory Region, and remote keys in a network. 

There are two major files the `ConfigFileExchanger` controls: pre-configuration file and the post-configuration file. 

ConfigFileExchanger's mission is to
1. Gather RDMA information from all participants. The participants are predefined in a pre-config file. One of the player plays a simple server role that gathers RDMA information in JSON format. 
2. After collection, it forms single JSON file that contains all the RDMA information about  all of the players. When all participants receive the file, they get the full status of all existing queue pairs in a network to connect. The file that contains the information is exported to a file system named `"post-conf"`.

After the distribution of post-config file, it is `RdmaConfigurator`'s responsibility to create, connect, send WRs etc. ConfigFileExchanger instanced will then have accomplished its mission, and allowed to be destroyed.

Pre-configuration file which is named `"pre-conf.json"` should strictly follow the form of:

```json
    {
        "port" : <int>, 
        "index" : <int>,
        "participants" : [
            {
                "nid" : <int>,
                "ip" : <string>,
                "alias" : <string> [OPTIONAL]
            },
            { "nid": <int>, "ip": <string>, "alias": <string> }, 
            { "nid": <int>, "ip": <string>, "alias": <string> }, 
            ...
        ]
    }
```

`"port"` name indicates the server port the `ConfigFileExhanger` instance will launch. `"index"` indicates the correspnding index of the "participant" list elements. Index starts from `0`, and be sure to designate running machine's information. `"participants"` lists holds all information of the players. The node information of 'myself' should also be included.

Post-configuration file which is named `"pre-conf.json"` should strictly follow the form of:

```json
[
    {
        "n": <node_id>, 
        "p": [
            {"i": <pd_id>, "m": [<list_of_mr>], "q": [<list_of_qp>]}, 
            {"i": <pd_id>, "m": [<list_of_mr>], "q": [<list_of_qp>]}, 
            ...
        ]
    }, 
    {
        ...
    },
    ...
]
```

This file is an array of multiple objects. Each objects represent a node. Each node may hold mutliple protection domains, which are comprised of multiple abstract RDMA resources, MRs and QPs. An object has its node ID under a key `"n"`, and a key `"p"` contains a list of protection domains. Each sub-object in the list of protection domain has three keys: `"i"` for the protection domain ID, `"m"` for the list of MRs, and `"q"` for the list of QPs. 

The file contains all of the queue pairs exist in the network, of the stated nodes pre-defined in the preconfig file.

The JSON format should strictly follow the design memtioned in this section. Otherwise, it will fail to parse or find any value from arbitrary keys.


## Compilation

Make sure you have all the hardware and drivers/libraries ready, for instance Mellanox drivers, subnet managers, etc. The repository provides `CMakeFile.txt` to use. **Hartebeest** is designed to serve as a user library, thus generates `libhartebeest.so`. However, there is a guide file `demo2.cpp` under the `test` directory that an application can test on.

```
project(HARTEBEEST)
cmake_minimum_required(VERSION 3.10)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(${CMAKE_SOURCE_DIR}/extern)

# Test Target
set(DEMO_TARGET demo.run)
set(LIB_HARTEBEEST libhartebeest)

# Demo
add_executable(
    ${DEMO_TARGET}
    ${CMAKE_SOURCE_DIR}/test/demo2.cpp 
)
set_target_properties(
    ${DEMO_TARGET} 
    PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/build
)

# Lib
add_library(
    ${LIB_HARTEBEEST} SHARED ${CMAKE_SOURCE_DIR}/src/rdma-conf.cpp
)
set_target_properties(
    ${LIB_HARTEBEEST} 
    PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/build
)
target_compile_options(
    ${LIB_HARTEBEEST} PRIVATE -Wall -Werror
)

# Essentials
target_link_libraries(${DEMO_TARGET}        PUBLIC ibverbs)
target_link_libraries(${LIB_HARTEBEEST}     PUBLIC ibverbs)
```

The default target is C++11. You can run the demo by:

```sh
./build/demo.run
```


