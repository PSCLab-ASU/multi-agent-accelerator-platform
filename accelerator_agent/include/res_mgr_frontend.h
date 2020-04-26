#include <map>
#include <set>
#include <zmq.hpp>
#include <zmq_addon.hpp>
#include <functional>
#include <iostream>
#include <hw_tracker.h>
#include <alloc_tracker.h>
#include <algorithm>
#include <tuple>
#include <utility>
#include <utils.h>
#include <req_meta.h>
#include <pico_helper.h>
#include <vector>
#include <memory>
#include <list>
#include <hw_nex_cache.h>
#include <job_interface.h>
#include <zmsg_builder.h>
#include <zmsg_viewer.h>
#include <json_viewer.h>
#include <nexus_utils.h>
#include <accel_utils.h>
#include <npending_msgs.h>

#ifndef RES_TRACK
#define RES_TRACK
enum  : ushort {RES_CHAR=0, RES_LONG, RES_SHORT, RES_ULONG, RES_FLOAT, RES_DOUBLE, RES_BOOL}; 
#define OPT_IN_TYPE zmq::multipart_t *, zmq::multipart_t *

class resmgr_frontend
{
  public: 
    typedef std::function<int(OPT_IN_TYPE)> action_func;
    typedef std::tuple<unsigned long, alloc_tracker, req_meta_data> work_item;

    resmgr_frontend();
    resmgr_frontend(zmq::socket_t *, zmq::socket_t *, std::string,
                    std::string, std::string );

    //entry point to all local hardware resources
    action_func action_ctrl(std::string, std::string, std::string);
 
    //claiming rank from resource
    int _res_ping_       (OPT_IN_TYPE);
    int _res_jinit_      (OPT_IN_TYPE);
    int _res_claim_      (OPT_IN_TYPE);
    int _res_rsync_      (OPT_IN_TYPE);
    int _res_cclaim_     (OPT_IN_TYPE);
    int _res_dealloc_    (OPT_IN_TYPE);
    int _res_manifest_   (OPT_IN_TYPE);
    int _res_setparams_  (OPT_IN_TYPE);
    //broadcast methods
    int _res_hwrollcall_ (OPT_IN_TYPE);
    int _res_hwidentity_ (OPT_IN_TYPE);
    int _res_hwqueryres_ (OPT_IN_TYPE);
    int _res_hwupdate_   (OPT_IN_TYPE);
    int _res_hwreg_      (OPT_IN_TYPE);
    int _res_nxgidreq_   (OPT_IN_TYPE);
    //runtime control
    int _res_nxinitjob_  (OPT_IN_TYPE);
    int _res_nxuptcache_ (OPT_IN_TYPE);
    int _res_nxconn_     (OPT_IN_TYPE);
    int _res_nxsnddata_  (OPT_IN_TYPE);
    int _res_nxrecvdata_ (OPT_IN_TYPE);
    int _res_nxuptdata_  (OPT_IN_TYPE);
    int _res_response_   (OPT_IN_TYPE);

    int set_zmq_context  (zmq::context_t *);
    int complete_request();
    int get_allFuncPtr( std::set< sfunce * > & );

  private:
    //private functions
    //purpose of this function is to send out P2P connection
    //requests
    int _respond_with_manifest( accel_utils::accel_ctrl, zmq::multipart_t * );
    int _init_remote_conn( std::string, std::string, std::vector<std::string> &);
    int _build_nex_conn( zmq::multipart_t &);
    //find or create entry in the nexus by nexud name
    nex_cache_ptr& _find_create_nexcache( std::string, bool& );
    //find or create entry in the job_registry by jobId
    job_interface_ptr& _find_create_jobinterface( std::string, bool& );

    std::vector< std::list<sfunce>* > _get_allFuncVPtr( );

    void _internal_name(std::string &);

    //get a formatted cache line 
    std::list<std::string> _get_nex_cache_data();

    nex_cache_ptr& _find_nex_cache_by_hostname(std::string);
    //generate function_entry
    function_entry _generate_func_entry(std::string, std::string);    
    //static member variables
    zmq::context_t * nexus_zmq_ctx;
    std::string nx_port;
    std::vector<std::string > repos;

    //this variables gets updated everytime a call is made to
    //the frontend
    std::string       current_req_addr;
    std::string       current_proc_id;
    std::string       current_mpirun_id;
    std::string       current_key;
    job_interface_ptr current_job_intf;  
    
    //mpirun tracker
    //res_tracker res_track;
    //gpucu/cpucu tracker (local on platform)
    //communicates to the resources remote to the CPU rank
    //remote_res_mgr  remote_mgr;
    
    std::map<nexus_utils::nexus_ctrl, resmgr_frontend::action_func >
             command_set;

    //hardware resource tracker
    //this structure only keeps track of all the LOCAL 
    //hardware drivers LOCAL LOCAL LOCAL ASSETS
    std::list<hw_tracker> hw_list;
    //allocation tracker list
    std::vector<alloc_tracker> alloc_list;
    //keeps special track of the home nexus for easy access
    nex_cache_ptr   home_nexus;
    nex_cache_ptr   req_nexus;
    //connection mgr
    //keeps track off all the connections
    //key is the hostname
    std::map<std::string, nex_cache_ptr> nexus_cache;
    //key is the jobId
    std::map<std::string, job_interface_ptr > job_registry;
    //pending messages registry 
    nexus_pending_msg_registry _pending_msgs_reg;
    //claim_registry
    //key = rid + ClaimId
    //value = comma separated
    std::map<std::string, std::string> _claim_registry;
    //local zmq socket
    zmq::socket_t * hw_entry;
    zmq::socket_t * hw_broadcast;
};

#endif
