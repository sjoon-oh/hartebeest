![logo](./logo/hartebeest-60.png)
# Project hartebeest

<!-- 
github.com/sjoon-oh/hartebeest
Author: Sukjoon Oh, sjoon@kaist.ac.kr
build-all.cpp 
-->

Project **Hartebeest** is an RDMA configuration module. 

Hartebeest is just a raw skeleton code for now. It just targets an InbiniBand based RDMA RC communication. Other features may be added in the future. 

The binary `test/core-test.cc`. `test/core-test-c.c` were tested on:
- CentOS7, 5.11.1-1.el7.elrepo.x86_64
- Mellanox Driver 5.7-1.0.2.0-rhel7.9-x86_64
- Mellanox ConnectX-5

This document briefly describes the project. Author: Sukjoon Oh, [sjoon@kaist.ac.kr](mailto:sjoon@kaist.ac.kr).

## Description

**Hartebeest** is a configurator of RDMA communication. Each machine in a network should know the remotes' RDMA resource status before initiating RDMA communication. However, it is upto an application programmer to fetch such status information. InfiniBand specification do not define a way to notify the status, or provide any mechanism to do such operation. 

 RDMA resource allocation information changes everytime an application launches. At the first launch of the application, these status should be shared among the participant machines. This project targets machines in a local network (e.g. data centers), thus it does not provide any security features. It is assumed that there are no Byzantine players, or intervention from outside.


**Hartebeest** is written in C++11, and is designed as single header-only library. If you use the provided `CMakeFile.txt`, it compiles a library that can be used with a single `include` of `src/includes/hartebeest.hh`, or alternatively `src/includes/hartebeest-c.h` for C bindings. 



## Guide

### Environment Variables

**Hartebeest** requires three environment variables before launching an application, that is the node ID of a local machine, the total number of a cluster, and the IP/port of a [Memcached](https://memcached.org/) server.

```cpp
export HARTEBEEST_PARTICIPANTS=0,1
export HARTEBEEST_NID=0
export HARTEBEEST_EXC_IP_PORT=111.222.333.444:9999
```

- `HARTEBEEST_NPARTICIPANTS`: Set the machine identifiers of machines in a cluster.
- `HARTEBEEST_NID`: Set the number of a machine identifier. Starting the number of `0` is recommended.
- `HARTEBEEST_EXC_IP_PORT`: Set the number of the Memcached server to communicate.
- `HARTEBEEST_CONF_PATH`: Set the configuration file path.

### Dependencies

**Hartebeest** requires [Memcached](https://memcached.org/) server to run in any reachable machine. Memcached stores RDMA resource information and this is shared among the cluster. **Hartebeest** also provides a way to exchange arbitrary message through Memcached.

Following libraries are necessary to compile **Hartebeest**:

- `libmemcached`
- `libibverbs`

Check `CMakeFiles.txt` for more information.


### Basic Usage

**Hartebeest** manages RDMA resource a single instance in an application. The core `HartebeestCore` follows the singleton design. This instance can be obtained by:

```cpp
hartebeest::HartebeestCore::get_instance()
```

, or just use predefined one:

```cpp
#define HARTEBEEST_CORE_HDL \
    hartebeest::HartebeestCore::get_instance()
```

Every function call must be through this singleton `HartebeestCore` instance, since it manages all the `ResourceCache<>` itself.

All RDMA resources such as `struct ibv_qp*`, `struct ibv_context*` are cached in to `ResourceCache<>` instances. `ResourceCache<>` does not create or allocate any resource, but only caches the allocated instances' address. The `std::map` holds the address, distinguished by simple string. The RDMA resources are wrapped by each class `Mr`, `Pd`, `Qp` and `Cq`. Resource wrapper class automatically allocates and releases resource. Each are identified by names, such as `"some-pd-1"`, `"some-mr-1"`.

For example, to allocate protection domain,
```cpp
hartebeest::HartebeestCore::get_instance().create_local_pd(
    "some-pd-1"
)
```
will do.

Since protection domain uses memory region and queue pair information, each `Pd` object contains MR cache `ResourceCache<Mr>` and QP cache `ResourceCache<Qp>`. `Pd` instance plays a role as a parent directory. Thus, to find or create memory region or queue pair, the application must designate existing protection domain.

```cpp
size_t buffer_size = 8192;
hartebeest::HartebeestCore::get_instance().create_local_mr(
    "some-pd-1",
    "new-mr-1",
    buffer_size,
    IBV_ACCESS_LOCAL_WRITE | IBV_ACCESS_REMOTE_READ | IBV_ACCESS_REMOTE_WRITE
);
```

Accessing local resource can be done by:
```cpp
struct ibv_qp* local_qp = 
    hartebeest::HartebeestCore::get_instance().get_local_qp(
        "some-pd-1",
        "new-qp-1"
    )->get_qp();

struct ibv_mr* local_mr = 
    hartebeest::HartebeestCore::get_instance().get_local_mr(
        "some-pd-1",
        "new-mr-1"
    )->get_mr();
```

After allocating local resource, the application must notify to others by setting values to the Memcached server. **Hartebeest** can push the value to the server, or wait the value fetched from the server. `memc_` interface interacts with the Memcached server. In general, the first parameter should be a key string of the Memacahed, and the others should be local identifiers.

When fetched, the remote resources such as `struct ibv_mr` and `struct ibv_qp` are automatically registered to the local cache. However, note that only part of the fields are valid. Local machine do not have all the information of the remote resource. Minimum fields necessary for RDMA communication are exchanged through the Memcached. For example, only `mr->addr` and `mr->rkey` are valid in the fetched `struct ibv_mr`.


```cpp
hartebeest::HartebeestCore::get_instance().memc_push_local_qp(
        "node0-qp-1", "some-pd-1", "new-qp-1");

hartebeest::HartebeestCore::get_instance().memc_fetch_remote_qp("node1-qp-1");
```

The `libibverbs` API requires some parameters for queue pair connection, state transitions or completion queue creation. These parameters are predefined in `hb_cfgldr.hh` and `hb_cfgldr.cc`. The parameters defined at these files can be changed by external `JSON` configuration file. The file should contain the predefined key, and the value to update. These values are updated when initializing the configurator instance, at the call of `init_params()`.

The configuration file should be like:

```json
{
    "cq_attr": {
        "cq_depth": 128
    },
    "qp_init_attr": {
        "rc:qp_type": 2,
        "rc:cap.max_send_wr": 128,
        "rc:cap.max_send_sge": 16,
        "rc:cap.max_recv_sge": 16,
        "rc:cap.max_inline_data": 256
    },
    "qp_attr": {
        "rc:path_mtu": 5
    }
}
```

For RC connections, put `rc:` prefix at the configuration files. These predefined settings can be viewed at the `hb_cfgldr.cc`.

Flow of the application should be like this:
1. `HARTEBEEST_CORE_HDL.create_local_pd()`
2. `HAREBEEST_CORE_HDL.create_local_mr()`
3. `HARTEBEEST_CORE_HDL.memc_push_local_mr()`
4. `HARTEBEEST_CORE_HDL.memc_fetch_remote_mr()`
5. `HARTEBEEST_CORE_HDL.create_basiccq()`
6. `HARTEBEEST_CORE_HDL.create_local_qp()`
7. `HARTEBEEST_CORE_HDL.init_local_qp()`
8. `HARTEBEEST_CORE_HDL.memc_push_local_qp()`
9. `HARTEBEEST_CORE_HDL.memc_fetch_remote_qp()`
10. `HARTEBEEST_CORE_HDL.connect_local_qp()`
11. `HARTEBEST_CORE_HDL.rdma_post_single_fast()`
12. `HARTEBEST_CORE_HDL.rdma_poll()`

For general message exchange:
- `HARTEBEEST_CORE_HDL.memc_wait_general()`
- `HARTEBEEST_CORE_HDL.memc_del_general()`
- `HARTEBEEST_CORE_HDL.memc_push_general()`

For C applications, C-like functions are provided in `hartebeest-c.h`.
1. `hartebeest_create_local_pd()`
2. `hartebeest_create_local_mr()`
3. `hartebeest_memc_push_local_mr()`
4. `hartebeest_memc_fetch_remote_mr()`
5. `hartebeest_create_basiccq()`
6. `hartebeest_create_local_qp()`
7. `hartebeest_init_local_qp()`
8. `hartebeest_memc_push_local_qp()`
9. `hartebeest_memc_fetch_remote_qp()`
10. `hartebeest_connect_local_qp()`
11. `hartebeest_rdma_post_single_fast()`
12. `hartebeest_rdma_poll()`

- `hartebeest_memc_wait_general()`
- `hartebeest_memc_del_general()`
- `hartebeest_memc_push_general()`
