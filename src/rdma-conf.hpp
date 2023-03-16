#pragma once

/* github.com/sjoon-oh/hartebeest
 * Author: Sukjoon Oh, sjoon@kaist.ac.kr
 * rdma-conf.hpp
 * 
 * Project hartebeest is a RDMA(IB) connector,
 *  a refactored version from Mu.
 */

#include <map>
#include <string>

// Config File Export/Importer
#include <fstream>
#include <ostream>
#include <iostream>

#include <cstdlib>
#include <cstring>

// for serv, cli
#include <arpa/inet.h>
// #include <sys/types.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <unistd.h> // For write(), read()

// extern.
#include "nlohmann/json.hpp"

// src
#include "device.hpp"
#include "mem-region.hpp"
#include "queues.hpp"
#include "prot-domain.hpp"

#include "file-conf.hpp"


namespace hartebeest {
    
    /*
     * Class RdmaConfigurator (Temporary Class Name)
     * 
     * This instance holds all RDMA-related managers. 
     * Each manager handles single-type ibv* api instances.
     *  
     * - DeviceManager : Manages HCA
     * - MrManager : Manages memory-related stuff, i.e. Memory Region, Buffers
     * - QueueManager : Provides basic IBV queues APIs.
     * - PdManager : Manages Protection Domains.
     * 
     * RdmaConfigurator works as a coordinator of these instances. 
     *  It provides few member functions, so that all managers may interact
     *  without big errors.
     * 
     * A program may have multiple IBV-resources, 
     *  such as many Queue Pairs, Protection Domains, and much more.
     *  Single RdmaConfigurator instance handles all of them in one instance, thus
     *  this is not designed to have many RdmaConfigurator objects in a program. 
     * 
     * Please refer to README.md for more information.
     * 
     */
    class RdmaConfigurator {
    private:
        
        bool                            dev_init;
        // It is reasonable to have a single device manager.

        //
        // Creation/Deletion/Linker
        std::unique_ptr<DeviceManager>  dev_manager;    // Device Management
        std::unique_ptr<MrManager>      mr_manager;     // Memory Management
        std::unique_ptr<QueueManager>   qs_manager;     // Queue Management
        std::unique_ptr<PdManager>      pd_manager;     // Protection Domain Management

        // All IB resources are associated to the RDMA context the dev_manager handles.

        //
        // Metadata Management.
        // Records what resource(key) is associated to which Protection Domain.
        // <K, V> = <mr_name, pd_name>
        // <K, V> = <qp_name, pd_name>
        // <K, V> = <cq_name, hca_idx>
        std::map<std::string, std::string>  mr_pd_regbook; 
        std::map<std::string, std::string>  qp_pd_regbook;
        std::map<std::string, int>          cq_ctx_regbook;

    public:
        RdmaConfigurator() : dev_init(false) {
            
            dev_manager.reset(new DeviceManager());
            mr_manager.reset(new MrManager());
            qs_manager.reset(new QueueManager());
            pd_manager.reset(new PdManager());
        }
        ~RdmaConfigurator() {

            mr_manager.release();
            qs_manager.release();
            pd_manager.release();
            dev_manager.release();

            // Resource release order matters. 
            // May change, but it is not recommended.
            // Do it at your own risk.
        }

        DeviceManager&  getDeviceManager() { return *dev_manager.get(); }
        MrManager&      getMrManager() { return *mr_manager.get(); }
        QueueManager&   getQManager() { return *qs_manager.get(); }
        PdManager&      getPdManager() { return *pd_manager.get(); }
        
        //
        // The inteface naming convention is designed to have:
        //  - do** : These are management functions. 
        //      Does something important. Directly updates its member. 
        //  - is** : Check status.
        //  - get** : Returns reference/value of a member. 
        //
        //  Members follow the underscore, methods follow the CamelCase naming convention.
        inline bool isPdRegistered(std::string arg_pd_name) {
            return pd_manager->isPdRegistered(arg_pd_name);
        }

        inline bool isBufferAllocated(std::string arg_buf_name) {
            return mr_manager->isBufferAllocated(arg_buf_name);
        }
        
        inline bool isMrRegistered(std::string arg_mr_name) {
            return mr_manager->isMrRegistered(arg_mr_name);
        }

        inline bool isMrAssociated(std::string arg_mr_name) {
            if (mr_pd_regbook.find(arg_mr_name) != mr_pd_regbook.end())
                return true;
            return false;
        }

        inline bool isQpAssociated(std::string arg_qp_name) {
            if (qp_pd_regbook.find(arg_qp_name) != qp_pd_regbook.end())
                return true;
            return false;
        }

        inline bool isCqAssociated(std::string arg_cq_name) {
            if (cq_ctx_regbook.find(arg_cq_name) != cq_ctx_regbook.end())
                return true;
            return false;
        }

        inline std::string getAssociatedPdFromMr(std::string arg_mr_name) {
            if (isMrAssociated(arg_mr_name))
                return mr_pd_regbook.find(arg_mr_name)->second;
            else return std::string("");
        }

        inline std::string getAssociatedPdFromQp(std::string arg_qp_name) {
            if (isQpAssociated(arg_qp_name))
                return qp_pd_regbook.find(arg_qp_name)->second;
            else return std::string("");
        }


        inline int getAssociatedHcaIdxFromCq(std::string arg_cq_name) {
            if (isCqAssociated(arg_cq_name))
                return cq_ctx_regbook.find(arg_cq_name)->second;
            else return int{-1};
        }


        //
        // Here I provide call interface sequence. 
        //  This RdmaConfig instance do not block multiple calls, 
        //  since there may be several CQs, PDs and MRs to create/register.
        //  Howver, ignoring orders between interfaces may cause unexpected behavior,
        //  thus read the document before using them. 
        //
        // This is temporary, set device/port 0 as default.
        // <Call Sequence 0>
        //  Initializes HCA device, using its device manager (dev_mmanager).
        //  Do not call multiple times.
        bool doInitDevice() {
            
            bool ret = true;

            const int dev_id = 0;
            const int dev_idx = 0;
            
            if (!dev_init) {
                ret = dev_manager->doGetDevice();
                ret = dev_manager->doOpenDevice(dev_id);
                ret = dev_manager->doPortBind(dev_id, dev_idx);
            }

            dev_init = true;

            return ret;
        }
        
        //
        // <Call Sequence 1>
        //  Device initialization must be done beforehand. 
        //  Creates a protection domain. 
        bool doRegisterPd(std::string arg_pd_name) {
            return pd_manager->doRegisterPd(arg_pd_name, dev_manager->getHcaDevice());
        }
        
        //
        // <Call Sequence 2>
        //  Allocates a buffer. All buffers are managed by the MrManager instance.
        bool doAllocateBuffer(std::string arg_buf_name, size_t arg_len, int arg_align) {
            return mr_manager->doAllocateBuffer(arg_buf_name, arg_len, arg_align);
        }

        //
        // <Call Sequence 3>
        //  Creates MR with arg_mr_name using existing buffer.
        bool doCreateAndRegisterMr(std::string arg_pd_name, 
            std::string arg_buf_name, std::string arg_mr_name) {
            
            // If arg_mr_name is associated to some PD, it cannot be registered.
            if (isMrAssociated(arg_mr_name)) return false;
            
            void* addr = mr_manager->getBufferAddress(arg_buf_name);
            size_t len = mr_manager->getBufferLen(arg_buf_name);

            // Creates an MR.
            struct ibv_mr* mr;
            mr = pd_manager->doCreateMr(
                arg_pd_name, 
                addr, 
                len, 
                0 | IBV_ACCESS_LOCAL_WRITE | IBV_ACCESS_REMOTE_READ | IBV_ACCESS_REMOTE_WRITE
            );

            if (mr == nullptr) return false;
            
            mr_pd_regbook.insert(std::pair<std::string, std::string>(arg_mr_name, arg_pd_name));

            return mr_manager->doRegisterMr2(arg_mr_name, mr);
        }

        //
        // <Call Sequence 4>
        //  Creates CQ using arg_cq_name. The CQ registered to the RDMA context (HCA).
        bool doCreateAndRegisterCq(std::string arg_cq_name) {
            
            // If arg_mr_name is associated to some context, it cannot be registered.
            if (isCqAssociated(arg_cq_name)) return false;
            cq_ctx_regbook.insert(std::pair<std::string, int>(arg_cq_name, 0));

            return
                qs_manager->doRegisterCq(
                    arg_cq_name, 
                    dev_manager->getHcaDevice().getContext());
        }

        //
        // <Call Sequence 5>
        //  Creates Queue Pair with arg_qp_name and register to PD of arg_pd_name.
        //  Send queue and receive queue (CQs) must be generated beforehand.
        //  The CQs only in this RdmaConfigurator instance can be registered.
        bool doCreateAndRegisterQp(
            std::string arg_pd_name,
            std::string arg_qp_name,
            std::string arg_send_cq_name,
            std::string arg_recv_cq_name
        ) {

            if (isQpAssociated(arg_qp_name)) return false;
            qp_pd_regbook.insert(std::pair<std::string, std::string>(arg_qp_name, arg_pd_name));

            return
                qs_manager->doCreateAndRegisterQp(
                    arg_qp_name,
                    pd_manager->getPd(arg_pd_name),
                    arg_send_cq_name,
                    arg_recv_cq_name
                );
        }

        //
        // <Call Sequence 6>
        //  Queue pair stays in RESET state after the creation. 
        //  This method simply makes transition to INIT state.
        bool doInitQp(std::string arg_qp_name, int arg_dev_idx = 0) {
            
            return
                qs_manager->doInitQp(
                    arg_qp_name, 
                    dev_manager->getHcaDevice().getPortId()
                    );
        }

        //
        // <Call Sequence 7>
        // doExportAll exports all associated information needed for connection (of remotes) to 
        //  JSON file. The header-only JSON parser library for C++ (https://github.com/nlohmann/json) 
        //  is used in this function. 
        //  Check the source in the "extern" directory.
        bool doExportAll(int arg_my_node_id, std::string arg_export_path = RDMA_MY_CONF_PATH) {

            nlohmann::json export_obj;
            
            export_obj["nid"] = arg_my_node_id;
            export_obj["qps"] = nlohmann::json::array();
            export_obj["mrs"] = nlohmann::json::array();

            //
            // First, export all qps
            for (const auto& elem : qp_pd_regbook) {
                
                std::string qp_name{elem.first};
                std::string pd_name{elem.second};

                export_obj["qps"].push_back(
                    {
                        {"qp_name", qp_name},
                        {"pd_name", pd_name},
                        {"qpn", qs_manager->getQp(qp_name)->qp_num},
                        {"pid", dev_manager->getHcaDevice().getPortId()},
                        {"plid", dev_manager->getHcaDevice().getPortLid()}
                    }
                );
            }

            //
            // First, export all mrs
            for (const auto& elem : mr_pd_regbook) {
                
                std::string mr_name{elem.first};
                std::string pd_name{elem.second};

                export_obj["mrs"].push_back(
                    {
                        {"mr_name", mr_name},
                        {"pd_name", pd_name},
                        {"addr", (unsigned long)(mr_manager->getMrAddr(mr_name))},
                        {"lk", mr_manager->getMrLocalKey(mr_name)},
                        {"rk", mr_manager->getMrRemoteKey(mr_name)}
                    }
                );
            }

            if (arg_export_path == "")
                arg_export_path = RDMA_MY_CONF_PATH;

            std::ofstream export_out{arg_export_path};
            export_out << std::setw(4) << export_obj << std::endl;

            return true;
        }

        //
        // <Call Sequence 8>
        //  
        bool doConnectRcQp(
            std::string arg_qp_name,
            int arg_remote_port_id,
            uint32_t arg_remote_qpn,
            uint16_t arg_remote_lid
        ) {
            return 
                qs_manager->doConnectRemoteRcQp(
                    arg_qp_name, arg_remote_port_id, arg_remote_qpn, arg_remote_lid
                );
        }

        // struct ibv_send_wr doBuildSendWr() {
        //     struct ibv_send_wr      work_req;

        //     return work_req;
        // }

        bool doPosrSendWr(
            std::string arg_qp_name,
            void* arg_buf, 
            uint32_t arg_len, 
            uint32_t arg_lk,
            uintptr_t arg_remote_addr,
            uint32_t arg_remote_rk
        ) {
            
            int ret = 0;

            struct ibv_send_wr      work_req;
            struct ibv_sge          sg_elem;
            struct ibv_send_wr*     bad_work_req = nullptr;

            struct ibv_qp* target_qp = qs_manager->getQp(arg_qp_name);

            memset(&sg_elem, 0, sizeof(sg_elem));
            memset(&work_req, 0, sizeof(work_req));

            sg_elem.addr        = reinterpret_cast<uintptr_t>(arg_buf);
            sg_elem.length      = arg_len;
            sg_elem.lkey        = arg_lk;

            work_req.opcode     = IBV_WR_RDMA_WRITE;
            work_req.send_flags = IBV_SEND_SIGNALED;
            work_req.wr_id      = 0;
            work_req.sg_list    = &sg_elem;

            work_req.wr.rdma.remote_addr    = arg_remote_addr;
            work_req.wr.rdma.rkey           = arg_remote_rk;

            ret = ibv_post_send(target_qp , &work_req, &bad_work_req);
            
            if (bad_work_req != nullptr) {
                std::cout << "bad_work_req not null\n";
                return false;
            }

            if (ret != 0) {
                std::cout << "ret: " << ret << "\n";
                return false;
            }

            return true;
        }

    };

    /* RDMA Queue Pairs requires the remotes' information to initiate communication. 
     *  It is a user's responsibility to define how to exchange the metadata.
     * All communication other than RDMA itself utilizes JSON format.
     * ConfigFileExchanger sends/receives infos in JSON format.
     * 
     * ConfigFileExchanger's mission is to
     *  (1) Gather RDMA information from all participants. The participants are predefined in
     *      a pre-config file. One of the player plays a simple server role that gathers RDMA infos 
     *      in JSON format.
     *  (2) After collection, it forms single JSON file that contains all the RDMA information about 
     *      all of the players. When all participants receive the file, they get the full status of all
     *      existing queue pairs in a network to connect. The file that contains the information is exported 
     *      to a file system named "post-conf".
     * 
     * After the distribution of post-config file, it is RdmaConfigurator's responsibility to create, connect, 
     *  send WRs etc. ConfigFileExchanger instanced will then have accomplished its mission, and allowed to be 
     *  destroyed.
     *
     * There are two configuration files the RdmaConfigurator requries:
     *  - Pre-Configuration File
     *      This file, which is named "pre-conf.json" (default)
     *      should strictly follow the form of:
     *      {
     *          "port" :    <int>, 
     *          "index" :   <int>,
     *          "participants" : [
     *              {
     *                  "nid" :     <int>,
     *                  "ip" :          <string>,
     *                  "alias" :       <string> [OPTIONAL]
     *              }, \
     *              { "nid": <int>, "ip": <string>, "alias": <string> }, 
     *              { "nid": <int>, "ip": <string>, "alias": <string> }, 
     *              ...
     *          ]
     *      }
     * 
     *      "port" name indicates the server port the ConfigFileExhanger instance will launch. 
     *      "index" indicates the correspnding index of the "participant" list elements. 
     *      Index starts from 0, and be sure to designate running machine's information.
     *      "participants" lists holds all information of the players. The node information
     *      of 'myself' should also be included.
     * 
     *  - Post-Configuration File
     *      This file which is named "post-conf.json" (default)
     *      should strictly follow the form of:
     *      
     *      {
     *          "all" : [
     *              {
     *                  "nid": 0,
     *                  "qps": [
     *                      {"pid": 1, "plid": 6, "qp_name": "some-qp-1", "qpn": 21031 },
     *                      {"pid": 1, "plid": 6, "qp_name": "some-qp-2", "qpn": 21032 },
     *                  ]
     *              },
     *              {
     *                  "nid": 1,
     *                  "qps": [
     *                      {"pid": 1, "plid": 5, "qp_name": "some-qp-1", "qpn": 1020 },
     *                      {"pid": 1, "plid": 5, "qp_name": "some-qp-2", "qpn": 1021 },
     *                  ]
     *              },
     *              ...
     *              {
     *                  "nid": 1,
     *                  "mrs": [
     *                      {"addr": 1234, "lk": 5345, "mr_name": "some-mr-1", "pd_name": "some-pd-1", "rk": 12345 },
     *                      ...
     *                  ]
     *              },
     *          ]
     *      }
     * 
     *      The file has single key, "all" which contains an array. 
     *      Each element in the array holds (1) "nid", (2) array of queue pair information under 
     *          the key "qps", and (3) array of memory region under the "mrs".
     *      The file contains all of the queue pairs exist in the network, of the stated nodes pre-defined in
     *      the preconfig file. 
     * 
     * 
     */
    class ConfigFileExchanger {
    private:
        nlohmann::json      pre_conf;
        nlohmann::json      post_conf;

        struct Node {                               // Node infos
            uint32_t            role;               // What role are you? ROLE_SERVER, ROLE_CLIENT ?
            int                 node_id;            // Your identifier?
            std::string         addr_str;           // Human-readable address plz.

            int                 state;              // Current connection state? (socket)
            int                 fd;                 // Connection socket FD.
            struct sockaddr_in  sockaddr_info;      // From the config file.

            //
            // For parsing
            int                 recv_sz;
            char*               buf;

            nlohmann::json      rdma_info;          // Our objective here, 
                                                    // set others' RDMA infos here.
        };

        int                         this_node_idx;  // This node's index
        std::vector<struct Node>    players;        // Node infos of participants

        //
        // Comm Related
        bool            epoll_init_flag;
        int             epoll_fd;

        int             port;
        int             sock_listen_fd;
        int             sock_conn_fd;
        
        //
        // Methods for insiders.
        void __gatherAllConfs() {
            post_conf["all"] = nlohmann::json::array();
            for (auto& member: players) 
                post_conf["all"].push_back(member.rdma_info);   
        }

        bool __recordSingleConf(
            char* arg_buf, 
            struct sockaddr_in arg_cli_sockaddr, 
            int arg_cli_fd) {
            
            try {
                nlohmann::json rdma_info = nlohmann::json::parse(std::string(arg_buf));

                int idx = -1;
                rdma_info.at("nid").get_to(idx);

                if (players.at(idx).state == STATE_FILLED)
                    return false;
                
                players.at(idx).rdma_info       = rdma_info;
                players.at(idx).state           = STATE_FILLED;

                players.at(idx).sockaddr_info   = arg_cli_sockaddr;
                players.at(idx).fd              = arg_cli_fd;

                delete[] players.at(idx).buf;
                players.at(idx).buf = nullptr;

            } catch(...) { return false; }

            return true;
        }

        bool __exportAllConfs() {

            std::ofstream export_out{RDMA_CONF_DEFAULT_PATH};
            export_out << std::setw(4) << post_conf << std::endl;

            return true;
        }

        bool __runServer(int arg_buf_size = 8192, int arg_max_client = 11) {
            
            int                 bsize = 1;
            char*               buf = nullptr;
            int                 player_idx;
            int                 send_sz = 0, recv_sz = 1, offset = 0;

            const int           BUFFER_SIZE = arg_buf_size;
            const int           MAX_CLIENT = arg_max_client;

            bool                ret = false;
            int                 epoll_fd = -1, sock_listen_fd = -1, sock_client_fd = -1;

            struct sockaddr_in  client_sockaddr;
            socklen_t           client_socklen;

            //
            // Setups.
            struct Node& this_node = players.at(this_node_idx); // This throws!
            struct sockaddr_in sockaddr_info = this_node.sockaddr_info;

            sockaddr_info.sin_addr.s_addr = htonl(INADDR_ANY);           // Fix to bind all.

            struct epoll_event  ev, *events;
            int                 event_num;

            // 
            // Now, let's start.
            // Resource alloc.
            if (this_node.role != ROLE_SERVER)
                return ret;

            else {
                buf = new char[BUFFER_SIZE];
                events = (struct epoll_event *)malloc(sizeof(struct epoll_event) * MAX_CLIENT);
            }

            // Socket step 1: Generate a socket.
            if ((sock_listen_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) goto exit_srv;
            setsockopt(sock_listen_fd, SOL_SOCKET, SO_REUSEADDR ,(char *)&bsize, sizeof(bsize));
            // fcntl(socket, F_SETFL, O_NONBLOCK);

            // Socket step 2: Bind() to address.
            if (bind(sock_listen_fd, (struct sockaddr*)&sockaddr_info, sizeof(sockaddr_info)) == -1) goto exit_srv;

            // Socket step 3. Initiate LISTEN
            if (listen(sock_listen_fd, MAX_CLIENT) == -1) goto exit_srv;

            if ((epoll_fd = epoll_create(512)) < 0) goto exit_srv;
            
            ev.events = EPOLLIN;
            ev.data.fd = sock_listen_fd;

            if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, sock_listen_fd, &ev) == -1) goto exit_srv;

            this_node.state = STATE_FILLED;

            //
            // Phase 1. Receive all of the Queue Pair information from each node. 
            // This phase blocks until it receives all the infos.
            while (!isEveryoneInState(STATE_FILLED)) {

                event_num = epoll_wait(epoll_fd, events, MAX_CLIENT, -1); 
                if (event_num == -1) goto exit_srv;

                for (int i = 0; i < event_num; i++) {
                    if (events[i].data.fd == sock_listen_fd) {
                        sock_client_fd = accept(sock_listen_fd, (struct sockaddr *)&client_sockaddr, &client_socklen);
                        
                        memset(buf, 0, sizeof(char) * BUFFER_SIZE);
                        recv_sz = read(sock_client_fd, buf, BUFFER_SIZE);

                        if (recv_sz  == -1) goto exit_srv;

                        player_idx = getPlayerIdx(*(int*)buf);

                        players.at(player_idx).buf = new char[BUFFER_SIZE];
                        players.at(player_idx).recv_sz = 0;
                        players.at(player_idx).fd = sock_client_fd;

                        memset(players.at(player_idx).buf, 0, sizeof(char) * BUFFER_SIZE);
                        
                        ev.events = EPOLLIN;
                        ev.data.fd = sock_client_fd;

                        if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, sock_client_fd, &ev) == -1) goto exit_srv;
                    }
                    else {  // I've seen this fd before!
                        for (auto& member: players) {
                            if (events[i].data.fd == member.fd) {

                                recv_sz = read(sock_client_fd, member.buf + member.recv_sz, BUFFER_SIZE - member.recv_sz);
                                member.recv_sz += recv_sz;

                                std::cout << "Received: " << member.buf << std::endl;

                                if (recv_sz == -1) goto exit_srv;
                                if (__recordSingleConf(member.buf, client_sockaddr, sock_client_fd) == false)
                                    continue;
                            }
                        }
                    }
                }
            }

            __gatherAllConfs();     // Generates single post configuration file in pre_conf member.
            __exportAllConfs();     // Export as a file.

            this_node.state = STATE_DISTRIBUTED;

            send_sz = post_conf.dump().size();
            memset(buf, 0, sizeof(char) * BUFFER_SIZE);
            memcpy(buf, post_conf.dump().c_str(), send_sz);

            while (!isEveryoneInState(STATE_DISTRIBUTED)) {
                
                for (auto& member: players) {
                    if (member.role == ROLE_SERVER) continue;
                    if (member.state == STATE_DISTRIBUTED) continue;

                    // The ConfigFileExchanger does not assume the config file to be extremely large. 
                    // The size of the file must be reasonaly set.
                    // Thus, the function do not execute multiple sends (as data partitions).
                    // If your configuration file must be huge, modify the part that uses the write(), and read() function.
                    
                    offset = 0;
                    while (send_sz > offset) {   
                        offset += write(member.fd, buf + offset, send_sz - offset);
                    }

                    member.state = STATE_DISTRIBUTED;
                }
            }

            ret = true;
exit_srv:
            // Cleans up allocated resources.
            delete[] buf;
            free(events);

            return ret;
        }

        //
        bool __runClient(int arg_buf_size = 8192) {
            
            int                 this_node_id = getThisNodeId();
            char*               buf = nullptr;
            const int           BUFFER_SIZE = arg_buf_size;
            
            int                 sock_conn_fd = -1;
            bool                ret = false;
            int                 send_sz = 0, recv_sz = 1, offset = 0;

            // 
            // Now, let's start.
            // Resource alloc.
            struct Node& this_node = players.at(this_node_idx); // This throws!
            struct Node server_node = getPlayerServer();

            std::string this_node_dump;

            if (this_node.role != ROLE_CLIENT || server_node.role != ROLE_SERVER)
                return ret;

            else
                buf = new char[BUFFER_SIZE];
            
            struct sockaddr_in serv_sockaddr_info = server_node.sockaddr_info;

            if ((sock_conn_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
                goto exit_cli;

            // Retry
            while (connect(
                    sock_conn_fd,
                    (struct sockaddr *)&serv_sockaddr_info, 
                    sizeof(serv_sockaddr_info)
                ) == -1)
                sleep(1);

            // Notify this node.
            // Simple!
            write(sock_conn_fd, &this_node_id, sizeof(this_node_id));

            this_node_dump = getThisNodeRdmaInfo().dump();

            // Send My Info
            memset(buf, 0, sizeof(char) * BUFFER_SIZE);
            memcpy(buf, this_node_dump.c_str(), this_node_dump.size());
            
            send_sz = this_node_dump.size();

            while (send_sz > offset) {   
                offset += write(sock_conn_fd, buf + offset, send_sz - offset);
            }

            // Receive All infos.
            offset = 0;
            memset(buf, 0, sizeof(char) * BUFFER_SIZE);

            while (recv_sz > 0) {
                recv_sz = read(sock_conn_fd, buf + offset, BUFFER_SIZE - offset);
                offset += recv_sz;
            }

            post_conf = nlohmann::json::parse(std::string(buf));

            __exportAllConfs();

            close(sock_conn_fd);
            ret = true;
exit_cli:
            delete[] buf;
            return ret;
        }





    public:
        ConfigFileExchanger() : 
            this_node_idx(-1),
            epoll_init_flag(false) {
                
            }
        ~ConfigFileExchanger() {}

        //
        // Getters and Setters. Follows the same naming convention.
        std::string getPreObjDump() {
            return pre_conf.dump();  // set pretty off.
        }

        int getNumOfPlayers() {
            return players.size();
        }

        int getPlayerIdx(int arg_node_id) {
            int ret = -1;

            for (size_t i = 0; i < players.size(); i++) 
                if (players.at(i).node_id == arg_node_id)
                    return i;

            return ret;
        }

        struct Node getPlayer(uint32_t arg_idx) {
            return players.at(arg_idx);
        }

        struct Node getPlayerServer() { 
            struct Node server;

            for (auto member: players) 
                if (member.role == ROLE_SERVER)
                    server = member;

            return server;
        }

        int getThisNodeIdx() { return this_node_idx; }
        int getThisNodeRole() { return players.at(this_node_idx).role; }
        int getThisNodeId() { return players.at(this_node_idx).node_id; }
        
        nlohmann::json getThisNodeRdmaInfo() {
            return players.at(this_node_idx).rdma_info;
        }

private:
        nlohmann::json __getOtherNodeQp(int arg_node_id, std::string arg_qp_name) {
            std::string qp_name{};

            for (auto& node: post_conf.at("all")) {
                if (node.at("nid") != arg_node_id) continue;

                for (auto& qp: node.at("qps")) {

                    qp.at("qp_name").get_to(qp_name);
                    if (qp_name == arg_qp_name)
                        return qp;
                }
            }

            return nlohmann::json{};
        }

        nlohmann::json __getOtherNodeMr(int arg_node_id, std::string arg_mr_name) {
            std::string mr_name{};

            for (auto& node: post_conf.at("all")) {
                if (node.at("nid") != arg_node_id) continue;

                for (auto& qp: node.at("mrs")) {

                    qp.at("mr_name").get_to(mr_name);
                    if (mr_name == arg_mr_name)
                        return qp;
                }
            }

            return nlohmann::json{};
        }

public:
        int getOtherNodeQpn(int arg_node_id, std::string arg_qp_name) {
            return __getOtherNodeQp(arg_node_id, arg_qp_name).at("qpn");
        }

        int getOtherNodePortId(int arg_node_id, std::string arg_qp_name) {
            return __getOtherNodeQp(arg_node_id, arg_qp_name).at("pid");
        }

        int getOtherNodePortLid(int arg_node_id, std::string arg_qp_name) {
            return __getOtherNodeQp(arg_node_id, arg_qp_name).at("plid");
        }

        uint32_t getOtherNodeMrAddr(int arg_node_id, std::string arg_mr_name) {
            return __getOtherNodeMr(arg_node_id,arg_mr_name).at("addr");
        }

        uint32_t getOtherNodeMrLk(int arg_node_id, std::string arg_mr_name) {
            return __getOtherNodeMr(arg_node_id,arg_mr_name).at("lk");
        }

        uint32_t getOtherNodeMrRk(int arg_node_id, std::string arg_mr_name) {
            return __getOtherNodeMr(arg_node_id,arg_mr_name).at("rk");
        }

        void setPostConf(nlohmann::json arg_post_conf) {
            post_conf = arg_post_conf;
        }

        void setPostConf(std::string arg_post_conf_str) {
            post_conf = arg_post_conf_str;
        }

        bool setThisNodeConf(std::string arg_conf_path = RDMA_MY_CONF_PATH) {

            nlohmann::json rdma_conf;
            std::ifstream conf_input(arg_conf_path);

            if (conf_input.fail()) 
                return false;

            conf_input >> rdma_conf;
            players.at(this_node_idx).rdma_info = rdma_conf;

            return true;
        }

        bool isEveryoneInState(int arg_state) {
            bool ret = true;
            
            for (auto& member: players) 
                if (member.state != arg_state) ret = false;
            
            return ret;
        }

        bool isKeyExist(nlohmann::json& arg_obj, std::string arg_key) {
            return arg_obj.contains(arg_key);
        }

        //
        // Make sure to call doReadConfigFile() first,
        //  before doing anything else. 
        bool doReadConfigFile(std::string arg_conf_file_path = EXCH_CONF_DEFAULT_PATH) {
            
            bool ret = false;
            std::ifstream conf_input(arg_conf_file_path);

            if (conf_input.fail()) 
                return ret;

            conf_input >> pre_conf;

            //
            // Basic configuration starts from here.
            // Initializes "this_node"

            // If any of the JSON defined form (e.g. existance of keys) do not meet, 
            // this function returns false.
            try {
                pre_conf.at(
                    "index"
                    ).get_to(this_node_idx);

                pre_conf.at("port").get_to(port);

                // Note that this doReadConfigFile assumes the values are valid.
                //  For instance, the function do not know how to handle conflicting node IDs,
                //  or multiple nodes who claims to be a server (NODE_ID = 0).

                for (auto& participant: pre_conf.at("participants")) {
                    
                    struct Node member;

                    member.node_id = participant.at("nid");

                    if (member.node_id == 0)
                        member.role = ROLE_SERVER;
                    else member.role = ROLE_CLIENT;
                    
                    participant.at("ip").get_to(member.addr_str);

                    // Socket related (Revive)                
                    memset(&member.sockaddr_info, 0, sizeof(struct sockaddr_in));
                    member.sockaddr_info.sin_family = AF_INET;
                    member.sockaddr_info.sin_port = htons(port);

                    ret = inet_aton(
                            member.addr_str.c_str(), 
                            &(member.sockaddr_info.sin_addr));   // Address Set

                    // Initial connection state
                    member.state = STATE_UNKNOWN;
                    member.fd = -1;

                    players.push_back(member);  // Register
                }
            }
            catch (...) {
                players.clear();
                return false;
            }

            return ret;
        }
  

        bool doExchange() {

            if (getPlayerServer().node_id == this_node_idx)
                return __runServer();
            
            else {
                return __runClient();
            }
        }
    };

}