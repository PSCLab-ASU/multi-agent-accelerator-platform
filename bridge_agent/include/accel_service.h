#include <map>
#include <list>
#include <vector>
#include <zmq.hpp>
#include <zmq_addon.hpp>
#include <functional>
#include <iostream>
#include <variant>
#include <utility>
#include <bbackend_intf.h>
#include <zmsg_builder.h>
#include <zmsg_viewer.h>
#include <client_utils.h>
#include <functional>
#include <thread_manager.h>
#include <config_components.h>
#include <anode_manager.h>
#include <ads_state.h>
#include <pending_msgs.h>
#include <ads_registry.h>
#include <accel_utils.h>

#ifndef ACCEL_SERV
#define ACCEL_SERV

#define API_MODULE(func) \
        std::vector<zmq::multipart_t> func (accel_header, zmq::multipart_t&& );

typedef std::function<std::vector<zmq::multipart_t>(accel_header, zmq::multipart_t&&)> action_func;

typedef thread_manager<accel_header, zmq::multipart_t, zmq::multipart_t,
                       action_func, 64> accel_thread_manager;

class accel_service : accel_thread_manager
{
  public: 
    accel_service();
    accel_service( std::string, std::string, std::string, std::string, bool );
    ~accel_service();

    bool check_stop(){ 
      bool stopped = _stop.load() && 
                     ( _data_steering_reg.size() > 0);
      //std::cout << " -- stopped = " << stopped << std::endl;
      return stopped;
    }
    mpi_return submit( zmq::multipart_t&& );

    std::optional<zmq::multipart_t> get_message();

    std::vector<std::optional<zmq::multipart_t> > get_messages();

    virtual accel_header get_header( zmq::multipart_t& )       final;
    virtual action_func action_ctrl( accel_utils::accel_ctrl ) final;
 
  private:
    void _remove_nexus_index( zmq::multipart_t& req)
    {
      //remove
      //type      size       val
      req.pop(); req.pop(); req.pop();
    } 
     
    std::string _generate_bridge_parms();
 
    void _add_nexresp_hdr( uint, zmq::multipart_t& );
    void _index_repositories( std::string, bool bAppend = false );

    std::optional<configfile> _import_configfile( std::string ); 
    std::vector<zmq::multipart_t> _read_all_nex_msgs();

    mpi_return _build_nexus_comm( std::vector<std::string> );
    //claiming rank from resource
    API_MODULE(_accel_ping_)
    API_MODULE(_accel_updt_manifest_)
    API_MODULE(_accel_ninit_)
    API_MODULE(_accel_sinit_)
    API_MODULE(_accel_rinit_)
    API_MODULE(_accel_claim_)
    API_MODULE(_accel_claim_resp_)
    API_MODULE(_accel_test_)
    API_MODULE(_accel_send_)
    API_MODULE(_accel_recv_)
    API_MODULE(_accel_nexus_redir_)
    API_MODULE(_accel_finalize_)
    API_MODULE(_accel_shutdown_)

    //stop service
    std::atomic_bool _stop;
    //spawn bridges if so or not
    bool _spawn_bridges;
    std::string _job_id; 
    //host file path 
    std::string _host_file;
    //repository directory
    std::string _repo;
    //configuration file
    std::optional<configfile> _config_file;
    //save total number of processor ranks
    ulong _max_proc_ranks;
    //accelerator node manager
    anode_manager _node_manager;
    //command set for remote resource manager
    std::map<accel_utils::accel_ctrl, action_func > command_set;
    //pending message subsys
    accel_pending_msg_registry _pending_msgs_reg;  
    //        requester_id  state 
    ads_registry _data_steering_reg;
    // kernel repo - holds potential 
    ncache_registry _kernel_registry;
 
    status_agent stat_agent;
};

#endif
