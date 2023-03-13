#pragma once

/* github.com/sjoon-oh/hartebeest
 * Author: Sukjoon Oh, sjoon@kaist.ac.kr
 * RdmaConfigurator.hpp
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

#include <arpa/inet.h>
// #include <sys/types.h>
#include <sys/socket.h>
#include <sys/epoll.h>

#include "nlohmann/json.hpp"

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
            if (isCqAssociated(arg_cq_name)) false;
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

            if (isQpAssociated(arg_qp_name)) false;
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

        //
        // <Call Sequence 8>
        // doExportAll exports all associated information needed for connection (of remotes) to 
        //  JSON file. The header-only JSON parser library for C++ (https://github.com/nlohmann/json) 
        //  is used in this function. 
        //  Check the source in the "extern" directory.
        bool doExportAll(std::string arg_export_path = "") {

            nlohmann::json export_obj;
            
            export_obj["node_id"] = 0;
            export_obj["qp_conn"] = nlohmann::json::array();

            for (const auto& elem : qp_pd_regbook) {
                
                std::string qp_name{elem.first};
                std::string pd_name{elem.second};

                export_obj["qp_conn"].push_back(
                    {
                        {"qp_num", qs_manager->getQp(qp_name)->qp_num},
                        {"port_id", dev_manager->getHcaDevice().getPortId()},
                        {"port_lid", dev_manager->getHcaDevice().getPortLid()}
                    }
                );
            }

            if (arg_export_path == "")
                arg_export_path = RDMA_CONF_DEFAULT_PATH;

            std::ofstream export_out{arg_export_path};
            export_out << std::setw(4) << export_obj << std::endl;

            return true;
        }
    };

    /* RDMA Queue Pairs requires the remotes' information to initiate communication. 
     *  It is a user's responsibility to define how to exchange the metadata.
     *
     * There are two configuration files the RdmaConfigurator requries:
     *  - Pre-Configuration File
     *      This file, which is named "hb_rdma_pre_config.json" (default)
     *      Ihe file should strictly follow the form of:
     *      {
     *          "port" :    <int>, 
     *          "index" :   <int>,
     *          "participants" : [
     *              {
     *                  "node_id" :     <int>,
     *                  "ip" :          <string>,
     *                  "alias" :       <string> [OPTIONAL]
     *              }, \
     *              { "node_id": <int>, "ip": <string>, "alias": <string> }, 
     *              { "node_id": <int>, "ip": <string>, "alias": <string> }, 
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
     */
    class ConfigFileExchanger {
    private:
        nlohmann::json      pre_conf;
        nlohmann::json      post_conf;

        struct Node {
            uint32_t            role;
            int                 node_id;
            std::string         addr;

            int                 state;
            int                 fd;
        };

        int                         this_node_idx;
        std::vector<struct Node>    players;

        //
        // Epoll Related
        bool            epoll_init_flag;
        int             epoll_fd;

        int             port;
        int             sock_listen_fd;
        int             sock_conn_fd;
        

    public:
        ConfigFileExchanger() : 
            this_node_idx(-1),
            epoll_init_flag(false) {
                
            }
        ~ConfigFileExchanger() {}

        //
        // Getters and Setters. Follows the same naming convention.
        std::string getObjDump() {
            return pre_conf.dump();  // set pretty off.
        }

        struct Node getPlayer(uint32_t arg_idx) {
            return players.at(arg_idx);
        }

        int getNumOfPlayers() {
            return players.size();
        }

        int getThisNodeIdx() { return this_node_idx; }
        int getThisNodeRole() { return players.at(this_node_idx).role; }

        void setPostConf(nlohmann::json arg_post_conf) {
            post_conf = arg_post_conf;
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
            pre_conf.at(
                __KEY(INDEX)
                ).get_to(this_node_idx);

            pre_conf.at(__KEY(PORT)).get_to(port);

            for (auto& participant: pre_conf.at(__KEY(PARTICIPANTS))) {
                
                struct Node member;
                // std::string ip;
                // int         port;

                member.node_id = participant.at(__KEY(NODE_ID));

                if (member.node_id == 0)
                    member.role = ROLE_SERVER;
                
                else member.role = ROLE_CLIENT;
                
                participant.at(__KEY(IP)).get_to(member.addr);
                // pre_conf.at(__KEY(PORT)).get_to(port);

                

                // // Socket related
                // memset(&member.sockaddr, 0, sizeof(struct sockaddr_in));

                // member.sockaddr.sin_family = AF_INET;
                // member.sockaddr.sin_port = htons(port);
                // ret = inet_aton(ip.c_str(), &(member.sockaddr.sin_addr));   // Address Set

                // Initial connection state
                member.state = STATE_UNKNOWN;
                member.fd = -1;

                players.push_back(member);  // Register
            }

            return true;
        }

        // This is a blocking function.
        bool doInitServer() {
            
            char buf[8192];

            struct Node this_node = players.at(this_node_idx); // This throws!

            if (this_node.role != ROLE_SERVER 
                || epoll_init_flag == true)
                return false;
            
            if (epoll_fd = epoll_create(512) > 0) 
                epoll_init_flag = true;

            return true;
        }

        bool doInitClient() {
            return true;
        }

        //
        // This is raw test code.
        bool doTestServer() {
            
            int                 bsize = 1;
            int                 sock_listen_fd = -1;

            const int           buf_size = 8192;
            char                buf[buf_size];

            struct Node& this_node = players.at(this_node_idx); // This throws!

            struct sockaddr_in  addr;

            addr.sin_family = AF_INET;
            addr.sin_port = htons(port);
            addr.sin_addr.s_addr = htonl(INADDR_ANY);

            //
            // epoll related.
            struct epoll_event  ev, *events;
            int                 event_num;

            events = (struct epoll_event *)malloc(sizeof(struct epoll_event) * 11);

            if (this_node.role != ROLE_SERVER 
                || epoll_init_flag == true)
                return false;

            if ((sock_listen_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
                return false;

            setsockopt(sock_listen_fd, SOL_SOCKET, SO_REUSEADDR, (char *)&bsize, sizeof(bsize));

            if (bind(
                    sock_listen_fd, 
                    (struct sockaddr*)&addr, 
                    sizeof(addr)) == -1)
                return false;
std::cout << "Phase 1\n";
            if (epoll_fd = epoll_create(512) > 0) 
                epoll_init_flag = true;

            else return false;

            ev.events  = EPOLLIN | EPOLLOUT | EPOLLERR;
            ev.data.fd = epoll_fd;

            epoll_ctl(epoll_fd, EPOLL_CTL_ADD, sock_listen_fd, &ev);

            while (1) {
            
                event_num = epoll_wait(epoll_fd, events, 11, -1); 

                for (int i = 0; i < event_num; i++) {

                    // If event has been on (sock_listen_fd)
                    if (events[i].data.fd == sock_listen_fd) {
                        
                        struct sockaddr_in  client_sockaddr;
                        socklen_t           client_socklen;
                        int                 sock_client_fd = -1;

                        sock_client_fd = accept(sock_listen_fd, (struct sockaddr *)&client_sockaddr, &client_socklen);
                        
                        std::string dump_info{getObjDump()};

                        write(sock_client_fd, dump_info.c_str(), dump_info.size());

                    }
                }
            }









            


            return true;
        }

        //
        // This is raw test code.
        bool doTestClient() {
            
            int sock_conn_fd = -1;

            // Find a server info.
            struct Node server;
            char* buf[8192] = { 0, };

            for (auto& member: players) 
                if (member.role == ROLE_SERVER)
                    server = member;

            if (sock_conn_fd = socket(AF_INET, SOCK_STREAM, 0) < 0 )
                return false;

            // if (connect(
            //     sock_conn_fd,
            //     (struct sockaddr *)&server.sockaddr, sizeof(server.sockaddr)
            // ) == -1)
            //     return false;

            // // write(sock_conn_fd, sample_str, 12);
            // read(sock_conn_fd, buf, 100);

            // std::cout << buf << std::endl;


            return true;
        }



    };

}