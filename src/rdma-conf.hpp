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

#ifdef LOG_ENABLED
#include "spdlog/spdlog.h"
#endif

namespace hartebeest {

    enum {
        CONFR_NOERROR = 0,
        CONFR_ERROR_GENERAL = 0x10,
    };

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
        DeviceManager       dv_mgr;     // Device Management
        MrManager           mr_mgr;     // Memory Management
        QueueManager        qs_mgr;     // Queue Management
        PdManager           pd_mgr;     // Protection Domain Management

        // Work Requests lists
        std::map<std::string, std::vector<struct ibv_wc>>
                                            cq_watchp_list;

    public:
        RdmaConfigurator() : dev_init(false) {
#ifdef LOG_ENABLED
            spdlog::info("[HB] RdmaConfigurator instance initiated.");
#endif
        }
        ~RdmaConfigurator() { }
        
        //
        // The inteface naming convention is designed to have:
        //  - do** : These are management functions. 
        //      Does something important. Directly updates its member. 
        //  - is** : Check status.
        //  - get** : Returns reference/value of a member. 
        //
        //  Members follow the underscore, methods follow the CamelCase naming convention.

        inline bool isPdRegistered2(uint32_t arg_pd_id) {
            return pd_mgr.isPdRegistered2(arg_pd_id);
        }

        inline bool isMrRegistered2(uint32_t arg_mr_id) {
            return mr_mgr.isMrRegistered2(arg_mr_id);
        }

        inline struct ibv_mr* getMr(uint32_t arg_mr_id) {
            return mr_mgr.getMr2(arg_mr_id);
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
        int doInitDevice2() {
            
            bool ret = true;

            const int dev_id = 0;
            const int dev_idx = 0;
            
            if (!dev_init) {
                ret = dv_mgr.doGetDevice();
                ret = dv_mgr.doOpenDevice(dev_id);
                ret = dv_mgr.doPortBind(dev_id, dev_idx);
            }

            dev_init = true;

            if (!ret) return CONFR_ERROR_GENERAL;

            return CONFR_NOERROR;
        }
        
        //
        // <Call Sequence 1>
        //  Device initialization must be done beforehand. 
        //  Creates a protection domain.   
        int doRegisterPd2(uint32_t arg_pd_id) {
            return pd_mgr.doRegisterPd2(arg_pd_id, dv_mgr.getHcaDevice());
        }

        //
        // <Call Sequence 2>
        //  Allocates a buffer. All buffers are managed by the MrManager instance.
        uint8_t* doAllocateBuffer2(size_t arg_len, int arg_align) {
            return mr_mgr.doAllocateBuffer2(arg_len, arg_align);
        }

        //
        // <Call Sequence 3>
        //  Creates MR with arg_mr_name using existing buffer.
        int doCreateAndRegisterMr2(uint32_t arg_pd_id, uint32_t arg_mr_id, uint8_t* buf, size_t arg_len) {
            
            // If arg_mr_name is associated to some PD, it cannot be registered.
            // if (isMrAssociated(arg_mr_name)) return false;
            int rights = 0 | IBV_ACCESS_LOCAL_WRITE | IBV_ACCESS_REMOTE_READ | IBV_ACCESS_REMOTE_WRITE;

            // Creates an MR.
            struct ibv_mr* mr = pd_mgr.doCreateMr2(arg_pd_id, buf, arg_len, rights);
            if (mr == nullptr) 
                return CONFR_ERROR_GENERAL;

            return mr_mgr.doRegisterMr22(arg_mr_id, mr);
        }

        //
        // <Call Sequence 4>
        //  Creates CQ using arg_cq_name. The CQ registered to the RDMA context (HCA).
        int doCreateAndRegisterCq2(uint32_t arg_cq_id) {
            return qs_mgr.doRegisterCq2(arg_cq_id, dv_mgr.getHcaDevice().getContext());
        }

        //
        // <Call Sequence 5>
        //  Creates Queue Pair with arg_qp_name and register to PD of arg_pd_name.
        //  Send queue and receive queue (CQs) must be generated beforehand.
        //  The CQs only in this RdmaConfigurator instance can be registered.
        int doCreateAndRegisterRcQp2(
            uint32_t    arg_pd_id,
            uint32_t    arg_qp_id,
            uint32_t    arg_send_cq_id,
            uint32_t    arg_recv_cq_id
        ) {

            return qs_mgr.doCreateAndRegisterRcQp2(
                arg_qp_id, pd_mgr.getPd2(arg_pd_id), arg_send_cq_id, arg_recv_cq_id);
        }

        //
        // <Call Sequence 6>
        //  Queue pair stays in RESET state after the creation. 
        //  This method simply makes transition to INIT state.
        int doInitQp2(uint32_t arg_qp_id, int arg_dev_idx = 0) {
            return qs_mgr.doInitQp2(arg_qp_id, dv_mgr.getHcaDevice(arg_dev_idx).getPortId());
        }

        //
        // <Call Sequence 7>
        // doExportAll exports all associated information needed for connection (of remotes) to 
        //  JSON file. The header-only JSON parser library for C++ (https://github.com/nlohmann/json) 
        //  is used in this function. 
        //  Check the source in the "extern" directory.
        int doExportAll2(int arg_my_node_id, std::string arg_export_path = RDMA_MY_CONF_PATH) {

            nlohmann::json export_obj;

            export_obj["n"] = arg_my_node_id;           // "n": Node ID
            export_obj["p"] = nlohmann::json::array();  // "c": RDMA Context

            for (auto& pd: pd_mgr.getPdMap()) {

                nlohmann::json sub_obj;

                sub_obj["i"] = pd.first;                    // "p": PD ID
                sub_obj["m"] = nlohmann::json::array();     // "m": MRs
                sub_obj["q"] = nlohmann::json::array();     // "q": QPs

                for (auto& mr: mr_mgr.getMrMap()) {
                    if (mr.second->pd == pd.second) {
                        sub_obj["m"].push_back(
                            {{"i", mr.first},
                                {"a", (uintptr_t)mr.second->addr},
                                {"s", mr.second->length},
                                {"r", mr.second->rkey}}); 
                    }
                }

                for (auto& qp: qs_mgr.getQpMap()) {
                    if (qp.second->pd == pd.second) {
                        sub_obj["q"].push_back(
                            {{"i", qp.first},
                                {"q", qp.second->qp_num},
                                {"p", dv_mgr.getHcaDevice().getPortId()},
                                {"l", dv_mgr.getHcaDevice().getPortLid()}});
                    }
                }
                
                export_obj["p"].push_back(sub_obj);
                sub_obj.clear();
            }

            if (arg_export_path == "")  
                arg_export_path = RDMA_MY_CONF_PATH;

            std::ofstream export_out{arg_export_path};
            export_out << std::setw(4) << export_obj << std::endl;
            
            return CONFR_NOERROR;
        }

        //
        // <Call Sequence 8>
        int doConnectRcQp2(
            uint32_t        arg_qp_id,
            int             arg_remote_port_id,
            uint32_t        arg_remote_qpn,
            uint16_t        arg_remote_lid
        ) {
            return qs_mgr.doConnectRemoteRcQp2(arg_qp_id, arg_remote_port_id, arg_remote_qpn, arg_remote_lid);
        }

        int doPost2(
            ibv_wr_opcode   arg_opcode,
            uint32_t        arg_qp_id,
            uintptr_t       arg_buf, 
            uint32_t        arg_len, 
            uint32_t        arg_lk,
            uintptr_t       arg_remote_addr,
            uint32_t        arg_remote_rk
        ) {
            
            int ret = 0;

            struct ibv_send_wr      work_req;
            struct ibv_sge          sg_elem;
            struct ibv_send_wr*     bad_work_req = nullptr;

            struct ibv_qp* target_qp = qs_mgr.getQp2(arg_qp_id);

            std::memset(&sg_elem, 0, sizeof(sg_elem));
            std::memset(&work_req, 0, sizeof(work_req));

            sg_elem.addr        = arg_buf;
            sg_elem.length      = arg_len;
            sg_elem.lkey        = arg_lk;

            work_req.wr_id      = 0;
            work_req.num_sge    = 1;
            work_req.opcode     = arg_opcode;
            work_req.send_flags = IBV_SEND_SIGNALED;
            work_req.wr_id      = 0;
            work_req.sg_list    = &sg_elem;
            work_req.next       = nullptr;

            work_req.wr.rdma.remote_addr    = arg_remote_addr;
            work_req.wr.rdma.rkey           = arg_remote_rk;

            ret = ibv_post_send(target_qp , &work_req, &bad_work_req);
            
            if (bad_work_req != nullptr) return CONFR_ERROR_GENERAL;
            if (ret != 0) return CONFR_ERROR_GENERAL;

            return CONFR_NOERROR;
        }

        int doRdmaWrite2(
            uint32_t        arg_qp_id,
            uintptr_t       arg_buf, 
            uint32_t        arg_len, 
            uint32_t        arg_lk,
            uintptr_t       arg_remote_addr,
            uint32_t        arg_remote_rk
        ) {
            return doPost2(IBV_WR_RDMA_WRITE, arg_qp_id, arg_buf, arg_len, arg_lk, arg_remote_addr, arg_remote_rk);
        }

        int doRdmaRead2(
            uint32_t        arg_qp_id,
            uintptr_t       arg_buf, 
            uint32_t        arg_len, 
            uint32_t        arg_lk,
            uintptr_t       arg_remote_addr,
            uint32_t        arg_remote_rk
        ) {
            return doPost2(IBV_WR_RDMA_READ, arg_qp_id, arg_buf, arg_len, arg_lk, arg_remote_addr, arg_remote_rk);
        }

        // int doPollSingleCq(std::string arg_cq_name) {
        //     int num_wc = 0;
        //     struct ibv_wc wc;

        //     struct ibv_cq* cq = qs_manager->getCq(arg_cq_name);
        //     std::vector<struct ibv_wc> watch_list = cq_watchp_list.find(arg_cq_name)->second;

        //     num_wc = ibv_poll_cq(cq, 1, &wc);
            
        //     if (wc.status != IBV_WC_SUCCESS) return -1;
        //     if (num_wc >= 0) watch_list.push_back(wc);

        //     return num_wc;
        // }

        // int doClearCqWatchlist(std::string arg_cq_name) {
            
        //     std::vector<struct ibv_wc> watch_list = cq_watchp_list.find(arg_cq_name)->second;
        //     int num_wcs = watch_list.size();

        //     watch_list.clear();
        //     return num_wcs;
        // }

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
     *      should strictly follow the form of a list of objects:
     *      
     *      [
     *          {
     *              "n": <node_id>, 
     *              "p": [
     *                  {"i": <pd_id>, "m": [<list_of_mr>], "q": [<list_of_qp>]}, 
     *                  {"i": <pd_id>, "m": [<list_of_mr>], "q": [<list_of_qp>]}, 
     *                  ...
     *              ]
     *          }, 
     *          {
     *              ...
     *          },
     *          ...
     *      ]
     * 
     *      This file is an array of multiple objects.
     *      Each objects represent a node. Each node may hold mutliple protection domains, which are comprised
     *      of multiple abstract RDMA resources, MRs and QPs. 
     *      An object has its node ID under a key "n", and a key "p" contains a list of protection domains.
     *      Each sub-object in the list of protection domain has three keys: "i" for the protection domain ID, 
     *      "m" for the list of MRs, and "q" for the list of QPs. 
     *  
     *      The file contains all of the queue pairs exist in the network, of the stated nodes pre-defined in
     *      the preconfig file. 
     * 
     */

    //
    // Elementary structs.
    // These primitives provide basic arrays without using STL.
    // Performance reasons.
    struct Mr {
        uint32_t    mr_id;
        uintptr_t   addr;
        size_t      length;
        uint32_t    rkey;
    };

    struct Qp {
        uint32_t    qp_id;
        uint32_t    qpn;
        uint8_t     pid;
        uint16_t    plid;
    };

    struct Pd {
        uint32_t        pd_id;
        uint32_t        num_mrs;
        uint32_t        num_qps;
        struct Mr*      mrs;
        struct Qp*      qps;
    };

    struct NodeContext {
        uint32_t        nid;
        uint32_t        num_pds;
        struct Pd*      pds;
    };

    struct RdmaNetworkContext {
        uint32_t                num_nodes;
        struct NodeContext*     nodes;
    };

    // 
    // Exchanger
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
        int                         port;

        
        //
        // Methods for insiders.
        void __gatherAllConfs() {
            post_conf = nlohmann::json::array();
            for (auto& member: players) 
                post_conf.push_back(member.rdma_info);
        }


        bool __recordSingleConf(
            char* arg_buf, 
            struct sockaddr_in arg_cli_sockaddr, 
            int arg_cli_fd) {
            
#ifdef LOG_ENABLED
            int node_id;
#endif

            try {
                nlohmann::json rdma_info = nlohmann::json::parse(std::string(arg_buf));

                int idx = -1;
                rdma_info.at("n").get_to(idx);

#ifdef LOG_ENABLED
                node_id = players.at(idx).node_id;
#endif
                if (players.at(idx).state == STATE_FILLED)
                    return false;
                
                players.at(idx).rdma_info       = rdma_info;
                players.at(idx).state           = STATE_FILLED;

                players.at(idx).sockaddr_info   = arg_cli_sockaddr;
                players.at(idx).fd              = arg_cli_fd;

                delete[] players.at(idx).buf;
                players.at(idx).buf = nullptr;

            } catch(...) { 
#ifdef LOG_ENABLED
                spdlog::info("[HB] More to receive from Node ID {}.", node_id);
#endif
                return false;
            }

            return true;
        }

        bool __exportAllConfs() {

            std::ofstream export_out{RDMA_CONF_DEFAULT_PATH};
            export_out << std::setw(4) << post_conf << std::endl;

            return true;
        }

        bool __runServer(int arg_buf_size = 81920, int arg_max_client = 11) {

#ifdef LOG_ENABLED
            spdlog::info("[HB] Running server.");
#endif
            
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
            if (this_node.role != ROLE_SERVER) return ret;  // Role check.

            else {
                buf = new char[BUFFER_SIZE];
                events = (struct epoll_event *)malloc(sizeof(struct epoll_event) * MAX_CLIENT);
            }

            // Socket step 1: Generate a socket.
            if ((sock_listen_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) goto exit_srv;
            setsockopt(sock_listen_fd, SOL_SOCKET, SO_REUSEADDR ,(char *)&bsize, sizeof(bsize));

            // Socket step 2: Bind() to address.
            if (bind(sock_listen_fd, (struct sockaddr*)&sockaddr_info, sizeof(sockaddr_info)) == -1) goto exit_srv;

            // Socket step 3. Initiate LISTEN
            if (listen(sock_listen_fd, MAX_CLIENT) == -1) goto exit_srv;

            // step 4: Prepare epoll
            if ((epoll_fd = epoll_create(512)) < 0) goto exit_srv;
            
            ev.events   = EPOLLIN;
            ev.data.fd  = sock_listen_fd; // Set event to this listen_fd.

            if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, sock_listen_fd, &ev) == -1) goto exit_srv;

            this_node.state = STATE_FILLED; 
                // Server is already filled,
                // Since it has its own RDMA infos. 
                // STATE_FILLED indicates that the RDMA info of a node is prepared (received).
                //  (..in the vector player. )

            //
            // Phase 1. Receive all of the Queue Pair information from each node. 
            // This phase blocks until it receives all the infos.
            while (!isEveryoneInState(STATE_FILLED)) {

#ifdef LOG_ENABLED
            spdlog::info("[HB] Epoll waiting...");
#endif
                
                event_num = epoll_wait(epoll_fd, events, MAX_CLIENT, -1); 
                if (event_num == -1) goto exit_srv;

                for (int i = 0; i < event_num; i++) {
                    if (events[i].data.fd == sock_listen_fd) {
                        // Case, when a node first connects to this server node.

#ifdef LOG_ENABLED
                        spdlog::info("[HB] A new client found.");
#endif

                        sock_client_fd = accept(sock_listen_fd, (struct sockaddr *)&client_sockaddr, &client_socklen);

#ifdef LOG_ENABLED
                        spdlog::info("[HB] Accepted.");
#endif
                        
                        std::memset(buf, 0, sizeof(char) * BUFFER_SIZE);
                        recv_sz = read(sock_client_fd, buf, BUFFER_SIZE);

#ifdef LOG_ENABLED
                        spdlog::info("[HB] Node ID ({}) read from socket {}.", *(int*)buf, sock_client_fd);
#endif

                        //
                        // At the first conection, client node sends its node ID as a "hello".
                        // There is no Byzntine players here, thus server believes that the value is not faked.
                        // Node ID is assigned by each node's configuration file, thus make sure to 
                        // set the correct one. 

                        if (recv_sz  == -1) goto exit_srv;

                        player_idx = getPlayerIdx(*(int*)buf);  // "hello", your node ID is recognized.

                        players.at(player_idx).buf = new char[BUFFER_SIZE];
                        players.at(player_idx).recv_sz = 0;
                        players.at(player_idx).fd = sock_client_fd;     // OK, you are acked.

                        std::memset(players.at(player_idx).buf, 0, sizeof(char) * BUFFER_SIZE);
                        
                        ev.events   = EPOLLIN;
                        ev.data.fd  = sock_client_fd; 
                            // If any further request comes, it will be distinguished by its fd.

                        if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, sock_client_fd, &ev) == -1) goto exit_srv;
                    }
                    else {  // I've seen this fd before!
                        for (auto& member: players) {
                            if (events[i].data.fd == member.fd) {
                                
                                // Sever has seen this client node before. 
                                // In this scope, it reads JSON data from the client that contains only its RDMA QP/MR context. 

#ifdef LOG_ENABLED
                                spdlog::info("[HB] Event again from Node ID {}", member.node_id);
#endif

                                recv_sz = read(sock_client_fd, member.buf + member.recv_sz, BUFFER_SIZE - member.recv_sz);
                                member.recv_sz += recv_sz;

#ifdef LOG_ENABLED
                                spdlog::info("[HB] received {} bytes.", member.node_id, recv_sz);
#endif

                                // After executing a single read, further data may remain. 
                                // Client will continue writing to the socket, while some are finished or in progress.
                                // Server records the received byte to the players vector, and directly calls __recordSingleConf.
                                // __recordSingleConf throws an exception (by nlohmann::json::parse) and 
                                // return false if the string cannot be paresed. 
                                // If it is the case, the member is not recorded as STATE_FILLED. Unfilled member continues to
                                // be recorded (appended) the JSON data by read() in this scope.

                                if (recv_sz == -1) goto exit_srv;
                                if (__recordSingleConf(member.buf, client_sockaddr, sock_client_fd) == false)
                                    continue;
                            }
                        }
                    }
                }
            }

            // STATE_DISTRIBUTED implies that the client has all the information of RDMA MR/QPs in
            // the target network. In this state, the client may be dismissed.

            __gatherAllConfs();     // Generates single post configuration file in pre_conf member.
            __exportAllConfs();     // Export as a file.

            this_node.state = STATE_DISTRIBUTED;

            send_sz = post_conf.dump().size();
            std::memset(buf, 0, sizeof(char) * BUFFER_SIZE);
            std::memcpy(buf, post_conf.dump().c_str(), send_sz);

#ifdef LOG_ENABLED
            spdlog::info("[HB] Server in the distribution phase.");
#endif
            while (!isEveryoneInState(STATE_DISTRIBUTED)) {
                
                for (auto& member: players) {
                    if (member.role == ROLE_SERVER) continue;
                    if (member.state == STATE_DISTRIBUTED) continue;

                    // The ConfigFileExchanger does not assume the config file to be extremely large. 
                    // The size of the file must be reasonaly set.
                    
                    offset = 0;
                    while (send_sz > offset) {
                        offset += write(member.fd, buf + offset, send_sz - offset);
#ifdef LOG_ENABLED
                        spdlog::info("[HB] Written to Node ID {}/{} bytes.", offset, send_sz);
#endif
                    }

                    member.state = STATE_DISTRIBUTED;
                    close(member.fd);
                }
            }

            ret = true;
exit_srv:
            // Cleans up allocated resources.
            delete[] buf;
            free(events);

            close(epoll_fd);

#ifdef LOG_ENABLED
            spdlog::info("[HB] Server terminated.");
#endif
            return ret;
        }

        //
        bool __runClient(int arg_buf_size = 81920) {
            
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

            // Role check.
            if (this_node.role != ROLE_CLIENT || server_node.role != ROLE_SERVER)
                return ret;

            else buf = new char[BUFFER_SIZE];
            
            struct sockaddr_in serv_sockaddr_info = server_node.sockaddr_info;

            if ((sock_conn_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
                goto exit_cli;

            // Retry
            while (connect(sock_conn_fd, (struct sockaddr *)&serv_sockaddr_info, sizeof(serv_sockaddr_info)) 
                == -1)
                sleep(1);

#ifdef LOG_ENABLED
            spdlog::info("[HB] Conntected to Node ID {}.", server_node.node_id);
#endif

            // Notify this node.
            // Simple!
            write(sock_conn_fd, &this_node_id, sizeof(this_node_id));
                // It is a simple 4-byte write. The client do not assume to be failed.

#ifdef LOG_ENABLED
            spdlog::info("[HB] Notiied this Node ID {} to Node ID {}.", this_node.node_id, server_node.node_id);
#endif

            this_node_dump = getThisNodeRdmaInfo().dump();

            // Send My Info
            std::memset(buf, 0, sizeof(char) * BUFFER_SIZE);
            std::memcpy(buf, this_node_dump.c_str(), this_node_dump.size());
            
            send_sz = this_node_dump.size();

#ifdef LOG_ENABLED
            spdlog::info("[HB] Waiting for 5 seconds...");
#endif
            sleep(5);

            while (send_sz > offset) {
                offset += write(sock_conn_fd, buf + offset, send_sz - offset);
#ifdef LOG_ENABLED
            spdlog::info("[HB] Sent this configuration of {}/{} bytes.", offset, send_sz);
#endif
            }

            // Receive All infos.
            offset = 0;
            std::memset(buf, 0, sizeof(char) * BUFFER_SIZE);

            while (recv_sz > 0) {
                recv_sz = read(sock_conn_fd, buf + offset, BUFFER_SIZE - offset);
                offset += recv_sz;

#ifdef LOG_ENABLED
            spdlog::info("[HB] Received post configuration of {} bytes.", recv_sz);
#endif
            }

            post_conf = nlohmann::json::parse(std::string(buf));

            __exportAllConfs();

            close(sock_conn_fd);
            ret = true;
exit_cli:
            delete[] buf;

#ifdef LOG_ENABLED
            spdlog::info("[HB] Client terminated.");
#endif
            return ret;
        }


    public:
        ConfigFileExchanger() : 
            this_node_idx(-1) {
                
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

public:
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
                    std::memset(&member.sockaddr_info, 0, sizeof(struct sockaddr_in));
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

        struct RdmaNetworkContext doExportNetworkContext() {

            struct RdmaNetworkContext   actx;
            
            actx.num_nodes = post_conf.size();
            actx.nodes = reinterpret_cast<struct NodeContext*>(
                std::malloc(sizeof(struct NodeContext) * actx.num_nodes));

            int ni = 0, pi = 0, mi = 0, qi = 0;

            for (auto node: post_conf) {
                node.at("n").get_to(actx.nodes[ni].nid);

                actx.nodes[ni].num_pds = node.at("p").size();
                actx.nodes[ni].pds = reinterpret_cast<struct Pd*>(
                    std::malloc(sizeof(struct Pd) * actx.nodes[ni].num_pds));

                for (auto pd: node.at("p")) {
                    
                    pd.at("i").get_to(actx.nodes[ni].pds[pi].pd_id);

                    actx.nodes[ni].pds[pi].num_mrs = pd.at("m").size();
                    actx.nodes[ni].pds[pi].num_qps = pd.at("q").size();

                    actx.nodes[ni].pds[pi].mrs = reinterpret_cast<struct Mr*>(
                        std::malloc(sizeof(struct Mr) * actx.nodes[ni].pds[pi].num_mrs));

                    actx.nodes[ni].pds[pi].qps = reinterpret_cast<struct Qp*>(
                        std::malloc(sizeof(struct Qp) * actx.nodes[ni].pds[pi].num_qps));

                    for (auto mr: pd.at("m")) {
                        
                        mr.at("i").get_to(actx.nodes[ni].pds[pi].mrs[mi].mr_id);
                        mr.at("a").get_to(actx.nodes[ni].pds[pi].mrs[mi].addr);
                        mr.at("s").get_to(actx.nodes[ni].pds[pi].mrs[mi].length);
                        mr.at("r").get_to(actx.nodes[ni].pds[pi].mrs[mi].rkey);

                        mi++;
                    }
                    mi = 0;

                    for (auto qp: pd.at("q")) {
                        
                        qp.at("i").get_to(actx.nodes[ni].pds[pi].qps[qi].qp_id);
                        qp.at("q").get_to(actx.nodes[ni].pds[pi].qps[qi].qpn);
                        qp.at("p").get_to(actx.nodes[ni].pds[pi].qps[qi].pid);
                        qp.at("l").get_to(actx.nodes[ni].pds[pi].qps[qi].plid);

                        qi++;
                    }
                    qi = 0;
                    pi++;
                }
                pi = 0;
                ni++;
            }

            return actx;
        }

        void doCleanNetworkContext(struct RdmaNetworkContext& arg_net_ctx) {
            
            for (uint32_t ni = 0; ni < arg_net_ctx.num_nodes; ni++) {
                for (uint32_t pi = 0; pi < arg_net_ctx.nodes[ni].num_pds; pi++) {
                    std::free(arg_net_ctx.nodes[ni].pds[pi].mrs);
                    std::free(arg_net_ctx.nodes[ni].pds[pi].qps);
                } 

                std::free(arg_net_ctx.nodes[ni].pds);
            }

            std::free(arg_net_ctx.nodes);
        }
    };

}