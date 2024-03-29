#include <res_mgr_frontend.h>
#include <zmsg_viewer.h>
#include <zmsg_builder.h>
#include <config_components.h>
#include <pico_utils.h>
#include <boost/range/algorithm.hpp>

#define BIND_ACTION(action)            \
  std::bind(&resmgr_frontend::action,\
            this,                      \
            std::placeholders::_1,     \
            std::placeholders::_2);

resmgr_frontend::resmgr_frontend()
{
  using ctrl = nexus_utils::nexus_ctrl;
  //main resource allocation methods
  command_set[ctrl::ping]          = BIND_ACTION(_res_ping_);
  command_set[ctrl::rem_jinit]     = BIND_ACTION(_res_jinit_);
  command_set[ctrl::rem_claim]     = BIND_ACTION(_res_claim_);
  command_set[ctrl::rem_cclaim]    = BIND_ACTION(_res_cclaim_);
  command_set[ctrl::rem_rsync]     = BIND_ACTION(_res_rsync_);
  command_set[ctrl::rem_dealloc]   = BIND_ACTION(_res_dealloc_);
  command_set[ctrl::rem_setparm]   = BIND_ACTION(_res_setparams_);
  command_set[ctrl::rem_manifest]   = BIND_ACTION(_res_manifest_);
  //methods to register new HW microservices
  //this function is used for expressing publishing to all HW umicro services
  command_set[ctrl::hw_rollcall]   = BIND_ACTION(_res_hwrollcall_);
  //used to return information from the micro services
  command_set[ctrl::hw_id]         = BIND_ACTION(_res_hwidentity_);
  command_set[ctrl::hw_qry]        = BIND_ACTION(_res_hwqueryres_);
  //I forogot what this was for
  command_set[ctrl::hw_reg]        = BIND_ACTION(_res_hwreg_);
  //I forogot what this was for
  command_set[ctrl::hw_upt]        = BIND_ACTION(_res_hwupdate_);
  //runtime 
  //used to initialize a job's ranks zero and send out
  //job specification to all nexsus
  command_set[ctrl::hw_init]       = BIND_ACTION(_res_nxinitjob_);
  command_set[ctrl::nex_upt]      = BIND_ACTION(_res_nxuptcache_);
  //used to estanblish a connection between nexus
  //first rank global rank 0 calls INITJOB, all other nexus call
  command_set[ctrl::nex_gid]       = BIND_ACTION(_res_nxgidreq_);
  //NX_CONN
  command_set[ctrl::nex_conn]      = BIND_ACTION(_res_nxconn_);
  //THIS IS THE INGRESS FUNCTION TO THE NEXUS SUBSYSTEM
  command_set[ctrl::nex_snd]       = BIND_ACTION(_res_nxsnddata_);
  //THIS IS THE EGRESS FUNCTION TO THE NEXUS SUBSYSTEM
  command_set[ctrl::nex_recv]      = BIND_ACTION(_res_nxrecvdata_);
  //THIS IS THE RETURN RESULTS DATA FROM THE LOCAL HW ACCEL
  command_set[ctrl::nex_uptd]      = BIND_ACTION(_res_nxuptdata_);
  //THIS IS THE RETURN RESULTS DATA FROM THE LOCAL HW ACCEL
  command_set[ctrl::nex_def]       = BIND_ACTION(_res_response_);

}

resmgr_frontend::resmgr_frontend(zmq::socket_t * entry_rtr, zmq::socket_t * publisher_entry, std::string port, std::string repo, std::string home_nex) :
  resmgr_frontend()
{
  bool found;
  std::string home_addr = home_nex + std::string(":") + port;
  //connection to data channel (bidirectional)
  //eternal connection
  hw_entry     = entry_rtr;
  //only out (unidirectional)
  hw_broadcast = publisher_entry;
  //nexus port
  nx_port      = port;
  //generate repo list
  //it is a semicolon separated list
  repos        = split_str<':'>( repo ); 
  //add home nexus
  home_nexus  =_find_create_nexcache(home_addr, found);
  //get agent root directory
  agent_root_dir = _get_root_agent_dir();  
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
////////////////////////////INTERNAL API//////////////////////////////////
//the purpose of this function is to send a connection request to the
//remote nexus - only spreads the node 0 word to all the nexuses
int resmgr_frontend::_init_remote_conn( std::string hostname, std::string init_home, std::vector<std::string> & host_list)
{
  std::list<std::string> hList;

  zmq::multipart_t req, rep;
  auto& conn = nexus_cache.at(hostname);
  
  if( host_list.size() == 0)
  {
    std::cout << "No host list found"<<std::endl;
    return 0;
  }

  hList.push_back(init_home);
  hList.insert(hList.end(), host_list.begin(), host_list.end() );

  //build msg header
  zmsg_builder mbuilder(current_mpirun_id, current_proc_id, 
                        std::string("__NEX_INIT_JOB__") );

  mbuilder.add_sections(hList).finalize();
  
  conn->send( mbuilder.get_zmsg() );

  return 0;
}
//builds header information for this nexus
int resmgr_frontend::_build_nex_conn( zmq::multipart_t & msg)
{
  msg.addstr( current_mpirun_id );
  msg.addstr( current_proc_id   );
  msg.addstr( "__NEX_INITJOB__" );
  return 0;
}

int resmgr_frontend::complete_request()
{
 std::cout <<" resmgr_frontend = complete_request"<<std::endl;
 return 0;
}

int resmgr_frontend::set_zmq_context(zmq::context_t * ctx)
{
  nexus_zmq_ctx = ctx; 
  return 0;
}

void resmgr_frontend::_internal_name(std::string & name)
{
    if( name.find("nexus") != std::string::npos)
    {
      name.replace(name.find("nexus"), 5, "nex");
    }
}
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
////////////////////////////EXZTENRAL API/////////////////////////////////
resmgr_frontend::action_func resmgr_frontend::action_ctrl(std::string req_addr, std::string method, std::string key)
{
  bool bt = false;
  //set update on current mpirun_id
  current_key       = key;
  current_req_addr  = req_addr;

  auto ctrl = nexus_utils::reverse_lookup( method );
  auto func = command_set[ctrl];
  if( func == nullptr ) func = command_set[nexus_utils::nexus_ctrl::nex_def];
 
  return func;
}

int resmgr_frontend::_res_response_(zmq::multipart_t * req, zmq::multipart_t * rep)
{
  std::cout <<"res_reponse function "<<std::endl;
  rep->clear();
  return 0;
}

int resmgr_frontend::_res_ping_(zmq::multipart_t * req, zmq::multipart_t * rep)
{ 
  rep->addstr("PING_OK");
  return 0;
}


int resmgr_frontend::_respond_with_manifest( accel_utils::accel_ctrl method, zmq::multipart_t * rep )
{
  //DO SOME INITIALIZATION WORK
  //CALL MANIFEST
  std::cout << "sending manifest..." << std::endl;
  auto key    = generate_random_str();
  //auto msg = accel_utils::start_routed_message(method, current_req_addr, key);
  auto msg = accel_utils::start_request_message(method, key);
  auto hw_cache_slist = _get_nex_cache_data();
  msg.add_sections( hw_cache_slist );
  msg.finalize();
  *rep = std::move( msg.get_zmsg() );
  return 0;
}

void resmgr_frontend::_launch_bridge( std::string bridge_parms )
{
  std::string cmd="";

  if( agent_root_dir )
    cmd = agent_root_dir.value() + "bridge_agent.bin -- " + bridge_parms + " &"; 
  else
    cmd = "bridge_agent.bin -- " + bridge_parms + " &"; 
  
  std::cout << "Spawning bridge: " << cmd << std::endl;  
  std::system( cmd.c_str() );
}

std::optional<std::string>
resmgr_frontend::_find_src_bridge( std::string job_id)
{
  auto bridge_addr = _bridge_registry.find( job_id );

  if( bridge_addr != _bridge_registry.end() ) 
    return bridge_addr->second;
  else return {};

}


int resmgr_frontend::_res_jinit_(zmq::multipart_t * req, zmq::multipart_t * rep)
{ 
  zmsg_viewer<> msgv(*req);
  std::string job_id = msgv.get_section<std::string>(0).front();
  bool is_local = msgv.get_section<bool>(1).front();
  bool spawn_bridge = msgv.get_section<bool>(2).front();
  if( spawn_bridge )
  {
    auto _bridge_parms = msgv.get_section<std::string>(3).front();
    _bridge_registry[job_id] = current_req_addr;

    _launch_bridge( _bridge_parms );
    //dont have to do anything
    //wait for next message from local bridge
    rep->clear();
  }
  else
  {
    //DO SOME INITIALIZATION WORK
    //CALL MANIFEST
    auto src_bridge = _find_src_bridge( job_id );
    _respond_with_manifest( accel_utils::accel_ctrl::ninit, rep );

    if( src_bridge && is_local )
    {
       //push in the reply message
       multi_reply_buffer.emplace_back( 3, rep->clone() );
       rep->pushstr( src_bridge.value() );
       multi_reply_buffer.emplace_back( 4, std::move(*rep) );
       //erase entry from bridge registry
       _bridge_registry.erase(job_id);
       // clearing the message
       rep->clear();
       return 2; //two indicates look into the buffer for the messages
    }
    else
    {
      std::cout << "No source bridge found..." << std::endl;
    }
  }

  return 0;

}

int resmgr_frontend::_res_manifest_(zmq::multipart_t * req, zmq::multipart_t * rep)
{ 
  return _respond_with_manifest( accel_utils::accel_ctrl::updt_manifest, rep );
}

int resmgr_frontend::_res_rsync_(zmq::multipart_t * req, zmq::multipart_t * rep)
{
  zmsg_viewer<> msgv(*req);
  zmsg_builder<> mbuilder;

  std::cout << "Calling into _res_rsync_ " << std::endl;
  return 0;
}

int resmgr_frontend::_res_cclaim_(zmq::multipart_t * req, zmq::multipart_t * rep)
{
  zmsg_viewer<> msgv(*req);
  zmsg_builder<> mbuilder;

  using S = std::string;

  auto pico_name =  msgv.get_section<S>(0).front();
  auto group_id  =  msgv.get_section<ulong>(1).front();
  auto global_id =  msgv.get_section<ulong>(2).front();
  auto func_id   =  msgv.get_section<S>(3).front();
  auto extra     =  msgv.get_section<S>(4);
  
  //find the hw/pico service
  auto hwt = std::find(hw_list.begin(), hw_list.end(), pico_name);
 
  if( hwt != hw_list.end() ) 
  {
    std::cout << "Calling into _res_cclaim " << std::endl;
  
    hwt->request_rank( current_mpirun_id, current_proc_id, 
                       std::make_tuple(group_id, global_id, func_id ),
                       extra );
  } 
  else 
  { 
    std::cout << "Could not find pico service" << std::endl; 
  }

  rep->clear();
 
  return 0;
}

int resmgr_frontend::_res_claim_(zmq::multipart_t * req, zmq::multipart_t * rep)
{ 
  zmsg_viewer<> msgv(*req);
  auto key = generate_random_str();
  auto method = accel_utils::accel_ctrl::claim_response;
  auto msg = accel_utils::start_request_message(method, current_key);
  /////////////////////////////////
  //LOG current_key, accel key, 
  //claim information, and pico service address 
  /////////////////////////////////
  auto resource_str = msgv.get_section<std::string>(0).front();
  auto claim_key    = current_req_addr + current_key;
  //add claim to registry
  _claim_registry.emplace(claim_key, resource_str );

  msg.add_arbitrary_data( true );
  msg.finalize();
  *rep = std::move( msg.get_zmsg() ); 

  return 0;
}

int resmgr_frontend::_res_dealloc_(zmq::multipart_t * req, zmq::multipart_t * rep)
{ 
  rep->addstr("OK");
  return 0;
}

int resmgr_frontend::_res_setparams_(zmq::multipart_t * req, zmq::multipart_t * rep)
{ 
  rep->addstr("OK");
  return 0;
}

int resmgr_frontend::_res_hwrollcall_(zmq::multipart_t * req, zmq::multipart_t  * rep)
{
  std::cout << "_res_hwrollcall req: " << *req << std::endl;
  auto method = pico_utils::pico_ctrl::reg_resource;
  auto key    = generate_random_str();

  auto msg = pico_utils::start_request_message(method, key);

  msg.add_arbitrary_data( std::string("ALL") ).finalize();
  msg.get_zmsg().send(*hw_broadcast);
  
  rep->clear();
  return 0;
}

int resmgr_frontend::_res_hwreg_(zmq::multipart_t * req, zmq::multipart_t  * rep)
{

  std::cout << "Entering res_hwreg ("<<current_req_addr <<")..." << std::endl;
  std::cout << "request : " << *req <<std::endl;
  //the request should be in the following format
  //list of: vid:pid:ss_vid::ss_pid
  ushort type      = req->poptyp<ushort>();
  ulong  len       = req->poptyp<ulong>();
  std::string addr = req->popstr();
  /////////////////////////////////////////
  type             = req->poptyp<ushort>();
  len              = req->poptyp<ulong>();
  /////////////////////////////////////////

  auto hwt    = std::find(hw_list.begin(), hw_list.end(), current_req_addr);
  //create a placeholder for hw_tracker
  //step 1: create an entry in hw_tracker
  hw_tracker new_hwt = hw_tracker(current_req_addr, addr, nexus_zmq_ctx);
  //add it to the list if its not there
  if( hwt == hw_list.end() )
  {
     std::cout << "Adding pico service "<< current_req_addr << std::endl;
     hw_list.push_back(new_hwt);
  }
  //re-establish reference to the added item
  hwt = std::find(hw_list.begin(), hw_list.end(), current_req_addr);
  //build all hardware elements
  for(ulong i=0; i < len; i++)
  {
    std::string kcache = req->popstr();
    std::cout << "Adding resource to hw_tracker : " << kcache << std::endl;
    hwt->edit_caches().add_resource( kcache );
  }

  return 0;
}

int resmgr_frontend::_res_hwqueryres_(zmq::multipart_t * req, zmq::multipart_t  * rep)
{
  std::cout << "Entering _res_hwqueryres_" << std::endl;
  auto cache = _get_nex_cache_data();

  rep->addtyp<ushort>(0); 
  rep->addtyp<ulong>( cache.size() );
  //adding the entries with no local sw kernels
  for(auto entry : cache) rep->addstr(entry);

  insert_completion_tag(*rep);
  std::cout << "Exiting _res_hwqueryres_" << std::endl;
  
  return 0; 
}


int resmgr_frontend::_res_hwidentity_(zmq::multipart_t * req, zmq::multipart_t  * rep)
{
  /////////////////////////////////////////////////////
  std::cout << "_res_ident req: " << *req << std::endl;
  /////////////////////////////////////////////////////
  ushort type    = req->poptyp<ushort>();
  ulong len      = req->poptyp<ulong>();
  bool commit    = req->poptyp<bool>();
  /////////////////////////////////////////////////////
  type           = req->poptyp<ushort>();
  len            = req->poptyp<ulong>();

  //TBD dont forget to add D: or F: to represent directory entries or files
  std::vector<std::string> file_repo_list(len);

  while( len--)
    file_repo_list.push_back(req->popstr());

  zmsg_builder mbuilder( current_req_addr, 
                         current_mpirun_id, 
                         current_proc_id,
                         std::string("identify_resource") );
  ps_create_identify_resource_msg(std::move(mbuilder), commit, repos, rep);

  std::cout <<"Sending bdc req" << rep <<std::endl;
  //even though this is a broadcast
  //the pico services should filter requests that dont belong to them
  //for certain functions
  rep->send(*hw_broadcast);
  std::cout << "end _res_ident " << std::endl;

  //does not repuire reply  a reply (this automatically happend)
  //no_response_pkt( rep );
  return 0;
}

int resmgr_frontend::_res_hwupdate_(zmq::multipart_t * req, zmq::multipart_t  * rep)
{
  ushort type    = req->poptyp<ushort>();
  ulong len      = req->poptyp<ulong>();

  auto tHW = std::find(hw_list.begin(), hw_list.end(), current_req_addr);

  if( tHW == hw_list.end()) std::cout << "Couldnt find pico service" <<std::endl;

  for(ulong i=0; i < len; i++)
  {
    
    std::string entry = req->popstr();
    //add resource to the hw_list
    tHW->edit_caches().add_resource( entry );
  }
  rep->addstr("NX-UPDATE: OK");
  return 0;
}

int resmgr_frontend::_res_nxinitjob_(zmq::multipart_t * req, zmq::multipart_t  * rep)
{
  zmsg_viewer<> msgv(*req);
  
  //1 = from app, 0= from nexus
  bool from_app       = *msgv.get_section<bool>(0).begin(); // is it from the application or nex
  ulong max_cpu_ranks = *msgv.get_section<ulong>(1).begin();
  //pass host list to vector string
  //2nd section
  auto host_list      = msgv.get_section<std::string>(2);
  //pass function list to vector string
  auto function_list  = msgv.get_section<std::string>(3);
  //extra params
  auto extra_params   = msgv.get_section<std::string>(4);

  ///////////////////////////////////////////////////////////
  /////////////////////Check all the nexus entries///////////
  ///////////////////////////////////////////////////////////
  bool already_exists = false;
  //Step 1: Create job_interface
  auto job_interface = _find_create_jobinterface(current_mpirun_id, 
                                                 already_exists);
  //add sections of configuration
  job_interface->set_configfile(host_list, function_list, extra_params);

  auto hostaddrs = job_interface->get_hostaddrs_from_config();
  bool is_local=true;

  for( const auto& hostname : hostaddrs )
  {
    std::cout <<"*************************************"<<std::endl;
    std::cout <<"hostnames = " << hostname <<std::endl;
    std::cout <<"*************************************"<<std::endl;
    //nex is filled in the other function uptnexcache
    auto nex = _find_create_nexcache(hostname, already_exists);

    //make sure this is at the end
    //KIND OF DANGEROUS BECAUSE ASSUME THAT hostnames
    //are UNIQUE regardless of domain
    is_local = (get_hostname() == nex->host_name);

    //if is_first is false only take nex information
    //discard the rank information
    //if nex and rank already exists do nothing    
    job_interface->add_nex_and_rank (is_local, nex, current_proc_id);
    //add rank_zero information if its local and
    if( is_local && (current_proc_id == "0") )
      job_interface->add_rank_zero( nex, max_cpu_ranks); 
  }
  //send request(s) out for nexus updated and downstream inits
  if( from_app ) 
  {
    std::cout << "Forwarding Request to other nex" << std::endl;
    //step 1: send init to all the nexuses in job
    //remember to set the from_app to false in the forwarding
    job_interface->sync_job(current_proc_id);
    //step 2: send an update cache message to all the nexus
    //        once per nexus per job
    //note:   the local nexus is updated locally within update_nexus
    auto cache = _get_nex_cache_data();
    job_interface->update_nexus(cache, current_proc_id);
    
  }

  //rep->addstr("NX-INITJOB: OK");
  rep->clear();
  return 0;
}

int resmgr_frontend::_res_nxconn_(zmq::multipart_t * req, zmq::multipart_t  * rep)
{
  std::cout << "NXCONN PACKET: " << req << std::endl;
  rep->addstr("NX-CONN: OK");
  return 0;
}

std::string resmgr_frontend::_recommend_va( std::string claim )
{
  //create candidate list
  std::vector<
    std::pair<std::string, ulong> > cand_list;

  ////////////////////////////////////////////////
  boost::for_each( hw_list, [&]( auto candidate ) 
  {
    if( candidate.can_support( claim ) )
    {
      auto addr = candidate.get_id();
      auto cong = candidate.get_congestion();
      cand_list.emplace_back( addr, cong );   
    }
 
  } );
  ///////////////////////////////////////////////
  boost::sort( cand_list, [](auto lhs, auto rhs){ return lhs.second < rhs.second; });
  if( cand_list.empty() ) std::cout << "Could not find virtualization agent" << std::endl;
  else
  {
    std::cout << "Found Virtualization Agent : "<< cand_list[0].first << std::endl;
    return cand_list[0].first;
  }

}

int resmgr_frontend::_res_nxsnddata_(zmq::multipart_t * req, zmq::multipart_t  * rep)
{
  std::cout << "entering : " << __func__ << std::endl;
  auto& pmr = _pending_msgs_reg;

  zmsg_viewer<> zmsgv(*req);

  auto AccId = zmsgv.get_section<std::string>( 0 ).front();
  std::string resource_key = current_req_addr + AccId;
  auto resource_str = _claim_registry.at(resource_key);

  auto pico_address = _recommend_va( resource_str );
  auto pmethod = pico_utils::pico_rolodex.at(pico_utils::pico_ctrl::send);
  auto key    = generate_random_str();
  //register pending message
  auto nspm  = nsend_pending_msg( current_key, current_req_addr ); 
  pmr.add_pending_message( key, std::move( nspm ) );

  //create multipart to pico
  *rep = std::move(*req);
  //removing accel old key
  (rep->poptyp<ushort>(), rep->poptyp<ulong>(), rep->popstr() );
  auto zmsgb = zmsg_builder<std::string, std::string, std::string> ( std::move(*rep) );
  zmsgb.add_arbitrary_data_top( resource_str )
       .add_raw_data_top( key, pmethod, pico_address);

  
  *rep = std::move( zmsgb.get_zmsg() );
  //returning one means to disregard to auto response
  return 1;
}
int resmgr_frontend::_res_nxrecvdata_(zmq::multipart_t * req, zmq::multipart_t  * rep)
{
  std::cout << "entering : " << __func__ << std::endl;
  auto& pmr = _pending_msgs_reg;
  auto amethod = accel_utils::accel_rolodex.at(accel_utils::accel_ctrl::recv);
  //TBD passing mpi tag
  int tag=0;
  //move data to response 
  *rep = std::move(*req);
  auto zmsgb = zmsg_builder<std::string, std::string, std::string> ( std::move(*rep) );
  //get the pending message from send
  auto nspm = pmr.get_pending_message<nsend_pending_msg>( current_key ); 
  auto[accel_key, accel_address] = nspm->get().get_ids();
  
  zmsgb.add_arbitrary_data_top( tag )
    .add_raw_data_top( accel_key, std::string(PLACEHOLDER),  amethod, accel_address);    

  *rep = std::move( zmsgb.get_zmsg() );

  std::cout << "returning data: " << accel_key << " to " << accel_address << std::endl;

  return 1;
}
int resmgr_frontend::_res_nxuptdata_(zmq::multipart_t * req, zmq::multipart_t  * rep)
{

  //Step 1: get work_id and data
  //Step 2: Lookup work_id and retrieve associated local nexus
  //Step 3: if nexus is local forward to local process
  //Step 3a: if nexus is remote forward to remote nexus process
  //Step 2: get outstanding request_list with (work_id, alloc_tracker) pair
  //Step 3: get 
  rep->addstr("NX-UPT-DATA: OK");
  return 0;

}

int resmgr_frontend::_res_nxgidreq_(zmq::multipart_t * req, zmq::multipart_t * rep)
{
  ulong group_id=0;
  zmsg_builder mbuilder(current_mpirun_id, current_proc_id, 
                        std::string("") );

  zmsg_viewer<std::string, std::string, std::string> mviewer(*req);
  ulong id_cnt = mviewer.get_section<ulong>(0).front();
  
  //allocating IDs
  std::list<ulong> ids(id_cnt);
  current_job_intf->allocate_global_ids(id_cnt, group_id, ids);

  mbuilder.add_arbitrary_data(group_id).
  add_sections<ulong>( ids ).finalize();

  *rep = std::move( mbuilder.get_zmsg() ); 

  return 0;
}

int resmgr_frontend::_res_nxuptcache_(zmq::multipart_t * req, zmq::multipart_t * rep)
{
  zmsg_viewer<> zmsgv(*req);

  bool already_exists  = false;
  bool respond         = *(zmsgv.get_section<bool>(0).begin());
  auto req_addr        = *(zmsgv.get_section<std::string>(1).begin());
  auto cache_lines     = zmsgv.get_section<std::string>(2);
  nex_cache_ptr & nptr = _find_create_nexcache( req_addr, already_exists );  

  //fill in the cache information into the nptr
  //make sure to override everything
  nptr->add_cache_data( cache_lines );

  if( respond )
  {
      zmsg_builder mbuilder(current_mpirun_id, current_proc_id, 
                            std::string("__NEX_UPDATECACHE__") );

      mbuilder.add_arbitrary_data<bool>(false)
              .add_arbitrary_data<std::string>(home_nexus->host_addr)
              .add_sections<std::string>(_get_nex_cache_data())
              .finalize();  
     
      //send response
      nptr->send( mbuilder.get_zmsg() );
  }
  //rep->addstr("NX-UPT-CACHE: OK");
  return 0;
}

nex_cache_ptr& resmgr_frontend::_find_create_nexcache( std::string rem_addr, bool& bFound)
{
  bFound = false;
  auto nex_cache = nexus_cache.find( rem_addr );

  if( nex_cache == nexus_cache.end() )
  {
   //emplace construct and immediately destroys if key is present
   //it also returns an iterator immediately to existing element
   //!!!!WARNING ONLY WORKS WITH DNS NAMES RIGHT NOW!!!!!
   //!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    std::string rem_name = split_str<'.'>(rem_addr)[0];
    nexus_cache.emplace(
                std::make_pair(rem_addr, 
                               std::make_shared<hw_nex_cache>(rem_name, rem_addr) ) );

  }else bFound = true;

  nex_cache = nexus_cache.find( rem_addr );

  return nex_cache->second;

}

job_interface_ptr& resmgr_frontend::_find_create_jobinterface( std::string jobId, bool & bFound)
{
  bFound = false;

  auto job = job_registry.find( jobId );

  if( job == job_registry.end() )
  { 
    job_registry.emplace(
                std::make_pair(jobId, 
                               std::make_shared<job_interface>( jobId ) ) );
  }else bFound = true;

  job = job_registry.find( jobId );

  return job->second;

}

function_entry resmgr_frontend::_generate_func_entry(std::string res, std::string meta)
{
 
  //extract alias function name
  auto func_parm_list = split_str<','>(res);
  auto alias_def = std::find_if(func_parm_list.begin(), func_parm_list.end(),
                                [](std::string keyval)
                                {
                                  auto kv = split_str<'='>(keyval);
                                  return  ( kv[0] == g_func_map.at(vfh::FUNC_ALIAS) );
                                } );

  std::string alias = split_str<'='>(*alias_def)[1];

  std::cout << "getting template function for : " << alias << std::endl;

  //get base parameters
  function_entry fe = current_job_intf->get_function_template( alias );
  //this function adds any parameters from the header mmap
  //overrides parameters in meta
  fe.import(res, meta);

  return fe;
}

std::list<std::string> resmgr_frontend::_get_nex_cache_data()
{
  std::list<std::string> out;
  auto tHW = std::for_each(hw_list.begin(), hw_list.end(), 
                           [&](const auto& hwt)
             {
               auto res_list = hwt.get_caches().dump_manifest();
               out.insert(out.end(), res_list.begin(), res_list.end() );
             } );
  return out;
}
nex_cache_ptr& resmgr_frontend::_find_nex_cache_by_hostname(std::string hostname)
{
  auto& nex = *(std::find_if(nexus_cache.begin(), nexus_cache.end(),
                               [&](const auto inp)->bool
                               {
                                 return ( inp.second->host_name == hostname );
                               }
                    ) );
  return nex.second;
}
