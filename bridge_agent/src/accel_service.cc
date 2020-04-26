#include <accel_service.h>
#include <thread_manager.h>
#include <boost/range/algorithm/transform.hpp>

#define NUM_THREADS 4

#define BIND_ACTION(action)            \
  std::bind(&accel_service::action,\
            this,                      \
            std::placeholders::_1,     \
            std::placeholders::_2);

accel_service::accel_service()
: thread_manager(NUM_THREADS)
{
  using ctrl = accel_utils::accel_ctrl;

  command_set[ctrl::ping]           = BIND_ACTION(_accel_ping_);
  command_set[ctrl::ninit]          = BIND_ACTION(_accel_ninit_);
  command_set[ctrl::rinit]          = BIND_ACTION(_accel_rinit_);
  command_set[ctrl::updt_manifest]  = BIND_ACTION(_accel_updt_manifest_);
  command_set[ctrl::claim]          = BIND_ACTION(_accel_claim_);
  command_set[ctrl::claim_response] = BIND_ACTION(_accel_claim_resp_);
  command_set[ctrl::test]           = BIND_ACTION(_accel_test_);
  command_set[ctrl::send]           = BIND_ACTION(_accel_send_);
  command_set[ctrl::recv]           = BIND_ACTION(_accel_recv_);
  command_set[ctrl::nexus_rd]       = BIND_ACTION(_accel_nexus_redir_);
  command_set[ctrl::finalize]       = BIND_ACTION(_accel_finalize_);
  command_set[ctrl::shutdown]       = BIND_ACTION(_accel_shutdown_);
}

accel_service::accel_service(std::string jobId, std::string hnex, 
                             std::string host_file, std::string repo)
: accel_service()
{
  _job_id = jobId;
  _repo   = repo;
  _stop.store( false );

  _index_repositories( _repo );

  if( !host_file.empty() )
  {
    std::cout << "xcelerate: Loading host file" << std::endl;
    //save host_file
    _host_file = host_file;
    //load configuration file
    _config_file = _import_configfile( host_file );

    auto rnex = _config_file->get_all_conn_str(zmq_transport_t::EXTERNAL);
    std::cout << "hnex = " << hnex << std::endl;
    
    for( auto nex : rnex ) std::cout << "rnex = "<< nex << std::endl;

    _node_manager = anode_manager( _job_id,
                                   hnex, 
                                   std::vector<std::string>( rnex.begin(), rnex.end()) );
  }
  else std::cout <<"xcelerate: no host file!" << std::endl;

}

accel_service::~accel_service() 
{
  
}
///////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////
void accel_service::_index_repositories( std::string repos, bool bAppend )
{
  //repos separated via colons
  //if( !bAppend ) 
  //return {};
}

void accel_service::_add_nexresp_hdr( uint nodeIdx, zmq::multipart_t& msg)
{
  std::string method  = msg.popstr();
  std::string nex_id = msg.popstr();

  msg.pushstr( std::to_string( nodeIdx ) );
  msg.pushstr( method ); 
}

mpi_return accel_service::submit(zmq::multipart_t && msg)
{
  ///////////////////////////////////////////////////////////////////////////////
  //submit job into the queue
  int stat =0;
  if( !msg.empty() )
  {
    std::cout << "submitting request from ADS: " << msg << std::endl;
    stat =  accel_thread_manager::submit( std::forward<zmq::multipart_t>(msg) );
  }
  ////////////////////////////////////////////////////////////////////////////////
  //submit jobs from the nexus 
  auto nresps = std::move( _read_all_nex_msgs() );

  for(uint i=0; auto&& resp : nresps )
  { 
    if( !resp.empty() )
    {
      _add_nexresp_hdr(i, resp ); 
      //std::cout << "submitting request from Nexus: " << resp << std::endl;
      stat =  accel_thread_manager::submit( std::forward<zmq::multipart_t>(resp) ); 
    }
    ++i;
  } 
  ////////////////////////////////////////////////////////////////////////////////
  return mpi_return{ stat };
}

std::vector<zmq::multipart_t> accel_service::_read_all_nex_msgs()
{
  return std::move( _node_manager.recv_all() );
}

std::optional<zmq::multipart_t>
accel_service::get_message( )
{
  return std::move( try_read() );
}

std::vector<
  std::optional<zmq::multipart_t> >
accel_service::get_messages( )
{
  return std::move( try_read_all() );
}

accel_header accel_service::get_header( zmq::multipart_t& msg )
{
  //FORMAT:
  //--SOURCE ADDR
  //--METHOD
  //-NEX_IDX-
  //--RECIEPT
  //----DATA
  accel_header accel_hdr;
  std::string pos1, pos2;
  pos1 = msg.popstr();
  pos2 = msg.popstr();
  auto lookup_pos1 = accel_utils::reverse_lookup( pos1 );
  auto lookup_pos2 = accel_utils::reverse_lookup( pos2 );
  if( lookup_pos1 )
  {
   
    //found a method in first place
    accel_hdr.requesting_id    = PLACEHOLDER;
    accel_hdr.method           = lookup_pos1.value();
    std::string nex_id         = pos2;
    if( nex_id != PLACEHOLDER )
      accel_hdr.requesting_nex = nex_id;
    accel_hdr.tag              = msg.popstr();  
  }
  else if( lookup_pos2 )
  {
    //found requesting_id
    accel_hdr.requesting_id   = pos1; 
    accel_hdr.method          = lookup_pos2.value();
    msg.popstr(); //PLACEHOLDER
    accel_hdr.tag             = msg.popstr();  
  }
  else
  {
    std::cout << "Packet type incompatible..." << std::endl;
  }
  
  return accel_hdr;
}

action_func accel_service::action_ctrl( accel_utils::accel_ctrl method )
{
  std::cout << "action ctrl : " << (ushort) method << std::endl; 
  return command_set.at(method);
}

///////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////API FUNCS//////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////
mpi_return accel_service::_build_nexus_comm(std::vector<std::string> addrs)
{
  return mpi_return{}; 
}

std::vector<zmq::multipart_t> 
accel_service::_accel_ping_(accel_header hdr, zmq::multipart_t&& req)
{
  std::vector<zmq::multipart_t> out;
  std::string method_str = accel_utils::accel_rolodex.at( hdr.method );
  zmsg_viewer<> zmsgv( req );
  zmsg_builder response(hdr.requesting_id.value(), 
                       std::string("__MPI_RESPONSE__"), 
                       method_str, hdr.tag );
  response.finalize();
  out.push_back( std::move(response.get_zmsg() ) );   

  return out;
}

std::vector<zmq::multipart_t> 
accel_service::_accel_nexus_redir_(accel_header hdr, zmq::multipart_t&& req)
{
 
  std::cout << "entering ... : " << __func__ << std::endl;
  return {};
}

std::vector<zmq::multipart_t> 
accel_service::_accel_ninit_(accel_header hdr, zmq::multipart_t&& req)
{ 
  std::cout << "entering ... : " << __func__ << std::endl;
  std::vector<zmq::multipart_t> out;
  ulong nid = hdr.get_nexus_id().value();
  zmsg_viewer zmsgv( req );
  //this will be init for the nexus comm
   _node_manager.post_init( nid, "", zmsgv.get_section<std::string>(0) );

  std::lock_guard guard( _pending_msgs_reg );

  if( _node_manager.is_fully_inited() )
  {
    using ctrl = client_utils::client_ctrl;
    //run through all the pending messages and return a response
    //rinit pends requests,
    auto init_msgs = _pending_msgs_reg.get_init_msgs( true );
    //set the method on all pending 
    for(auto& msg : init_msgs ) 
    {
      auto [key, rid] = msg.get_ids();
      std::cout << "Sending unblocking request to : " << rid 
                << " with key : " << key << std::endl; 
      auto client_msg = client_utils::start_routed_message(
                        ctrl::resp, {}, rid, key );
      client_msg.finalize();
      out.push_back( std::move( client_msg.get_zmsg() ) ); 
    }
  }

  return out;
}

std::vector<zmq::multipart_t> 
accel_service::_accel_rinit_(accel_header hdr, zmq::multipart_t&& req)
{ 
  std::cout << "entering ... : " << __func__ << std::endl;
  using ctrl = client_utils::client_ctrl;
  std::vector<zmq::multipart_t> out;

  auto& dsr = _data_steering_reg;
  bool bExists = true;
  //this will be the init for the ADS processes
  auto rid = hdr.requesting_id.value();

  //find the ADS state 
  dsr.find_create_adss( rid, "", "", bExists, true);

  if( bExists ) 
    std::cout << "trying to register ADS that already exists" << std::endl;

  if( _node_manager.is_fully_inited() )
  {
    std::cout<< "already inititalized sending unblocking " << std::endl;
    std::cout << "The key is: " << hdr.tag << std::endl;
    auto client_msg = client_utils::start_routed_message(
                      ctrl::resp, {},  rid, hdr.tag );
    client_msg.finalize();
    out.push_back( std::move( client_msg.get_zmsg() ) ); 
  }
  else
  {
    std::cout<< "Sending request to pending init " << std::endl;
    //send to pending messaged registry
    init_pending_msg ipmsg( hdr.tag, rid );
    std::lock_guard guard(_pending_msgs_reg );
    _pending_msgs_reg.add_pending_message( generate_random_str(), 
                                           std::move(ipmsg) );
  }

  return out;
}

std::vector<zmq::multipart_t> 
accel_service::_accel_test_(accel_header hdr, zmq::multipart_t&& req)
{ 
  std::cout << "entering ... : " << __func__ << std::endl;
  return {};
}

std::vector<zmq::multipart_t> 
accel_service::_accel_recv_(accel_header hdr, zmq::multipart_t&& req)
{ 
  std::cout << "entering ... : " << __func__ << std::endl;
  std::vector<zmq::multipart_t> out;
  auto& pmr = _pending_msgs_reg;
  //get pending request by hdr.tag
  //fetch the LibId, and requester ID
  std::lock_guard guard( pmr );
  auto[libKey, rid ] = pmr.get_ids( hdr.tag );
  std::cout << "hdr.tag = " << hdr.tag << 
               "\nlibKey  = " << libKey <<
               "\nrid     = " << rid <<  std::endl;
  //create client message
  auto client_method = client_utils::client_ctrl::recv;
  _remove_nexus_index( req );
  auto client_msg = client_utils::reroute_waitable_message(rid, libKey, client_method, std::move(req) );
  //push msg to output
  out.push_back( std::move( client_msg.get_zmsg() ) ); 
  //clean up pending message
  pmr.remove_message( hdr.tag );

  return out;
}

std::vector<zmq::multipart_t> 
accel_service::_accel_send_(accel_header hdr, zmq::multipart_t&& req)
{ 
  std::cout << "entering ... : " << __func__ << std::endl;
  std::string key = generate_random_str();
  auto& dsr  = _data_steering_reg;
  auto& pmr  = _pending_msgs_reg;
  auto  rid  = hdr.requesting_id.value();
  auto& adss = dsr.find_adss( rid ).value().get();
  ////////////////////////////////////////////////////////////////////
  std::string claimId = (req.poptyp<ushort>(), 
                         req.poptyp<ulong>(),
                         req.popstr() );

  auto AccId = adss.get_accel_id( claimId );
  auto nId   = adss.get_nex_id( claimId );
   
  std::cout << "ClaimId = " << claimId << std::endl <<
               "rid = "     << rid << std::endl << 
               "key = "     << key << std::endl << 
               "AccId = " << AccId << std::endl << 
               "nId   = " << nId   << std::endl;
  /////////////////////////////////////////////////////////////////////
  //send to pending messaged registry
  send_pending_msg pmsg( hdr.tag, rid );
  {  
    std::lock_guard guard( pmr );
    pmr.add_pending_message( key, std::move(pmsg) );
  }
  ///////////////////add new header/////////////////////////////////////  
  auto zmsgb = zmsg_builder<std::string, std::string> ( std::move(req) );
  zmsgb.add_arbitrary_data_top( AccId )
      .add_raw_data_top( key, 
                         nexus_utils::nexus_rolodex.at(nexus_utils::nexus_ctrl::nex_snd)); 
  ////////////////////////////////////////////////////////////////////////
  std::cout << "send msg format " << zmsgb.get_zmsg() << std::endl;
  _node_manager.send_data( nId, std::move( zmsgb.get_zmsg() ) );

  return {};
}

std::vector<zmq::multipart_t> 
accel_service::_accel_updt_manifest_(accel_header hdr, zmq::multipart_t&& req)
{ 
  std::cout << "entering ... : " << __func__ << std::endl;

  //_node_manager.update_resource_list(nid,  );
  
  
  return {};
}

std::vector<zmq::multipart_t> 
accel_service::_accel_finalize_(accel_header hdr, zmq::multipart_t&& req)
{ 
  std::cout << "entering ... : " << __func__ << std::endl;
  //this will be the init for the ADS processes
  using ctrl = client_utils::client_ctrl;
  auto rid  = hdr.requesting_id.value();
  auto& dsr = _data_steering_reg;
  auto& pmr = _pending_msgs_reg;
  std::vector<zmq::multipart_t> out;

  zmsg_viewer<> zmsgv( req );
  ulong rank = zmsgv.get_section<ulong>( 0 ).front();
  //needs to check ALL the ADS threads to VERIFY that all of them have reached
  //finalize
  auto& adss = dsr.find_adss( rid ).value().get();
  adss.finalized();
  
  std::lock_guard guard( pmr );
  if(  dsr.is_all_ads_finalized() ) 
  {
    //send sentinal to all the clients indicate last packet sent
    //before shutdown
    auto final_msgs = pmr.get_final_msgs(false);
    printf(" how many messages in pmr : %i \n", final_msgs.size() );
    for( const auto&  msg : final_msgs )
    {
      auto [l_key, l_rid] = msg.get_ids();
      std::cout<< "Xcelerate: sending finalizing  " << l_rid << std::endl;
      auto client_msg = client_utils::start_routed_message(
                        ctrl::resp, {},  l_rid, l_key );
      client_msg.finalize();
      out.push_back( std::move( client_msg.get_zmsg() ) ); 
    }
    //add the last finalization
    auto client_msg = client_utils::start_routed_message(
                      ctrl::resp, {},  rid, hdr.tag );

    client_msg.finalize();
    out.push_back( std::move( client_msg.get_zmsg() ) ); 

    return out;
  }
  else
  {
    //save it into pending request for later transmission
    printf( "Xcelerate: sending rank %i (%s) finalize to pending : %i\n ", rank, rid.c_str(), dsr.size());
    //send to pending messaged registry
    final_pending_msg fpmsg( hdr.tag, rid );
    pmr.add_pending_message( generate_random_str(), std::move(fpmsg) );
    return {};
  }

}

std::vector<zmq::multipart_t> 
accel_service::_accel_shutdown_(accel_header hdr, zmq::multipart_t&& req)
{ 
  std::cout << "entering ... : " << __func__ << std::endl;
  //shutsdown runtime service
  _stop.store( true );
  return {};
}

std::vector<zmq::multipart_t> 
accel_service::_accel_claim_(accel_header hdr, zmq::multipart_t&& req)
{
  std::cout << "entering ... : " << __func__ << std::endl;
  std::list<std::string> overrides;
  auto& dsr = _data_steering_reg;
  auto& pmr = _pending_msgs_reg;

  bool bExists =  false;
  zmsg_viewer<> zmsgv( req );
  std::string falias = zmsgv.get_section<std::string>(0).front();
  //auto overrides     = zmsgv.get_section<std::string>(1);
  auto claimId       = hdr.tag;
  auto claim_ovr     = std::map<std::string, std::string>();
  auto AccId         = generate_random_str();

  std::cout << "Generate AccId = " << AccId << "--" << hdr.tag << std::endl;
  
  for(auto item : overrides ) 
  {
    auto sub_item = split_str<'='>( item );
    claim_ovr.emplace( sub_item[0] , sub_item[1] );
  }
  
  //get header definition
  std::string fe_header = _config_file->get_function_header( falias );

  //set pending message
  {
    std::lock_guard< decltype(pmr) > lk( pmr );

    claim_pending_msg cpm(hdr.tag, hdr.requesting_id.value() );
    pmr.add_pending_message( AccId, std::move( cpm ) );  
  } 
  //find the ADS state 
  auto& adss = dsr.find_create_adss( hdr.requesting_id.value(), claimId, AccId, bExists, true);
  //find and make request to node
  _node_manager.make_claim( AccId, fe_header, claim_ovr);

  return {};
}

std::vector<zmq::multipart_t> 
accel_service::_accel_claim_resp_(accel_header hdr, zmq::multipart_t&& req)
{
  std::cout << "entering ... : " << __func__ << std::endl;
  using ctrl = client_utils::client_ctrl;
  auto& dsr = _data_steering_reg;
  auto& pmr = _pending_msgs_reg;
  ulong nid = hdr.get_nexus_id().value();
  std::vector<zmq::multipart_t> out;
  //cast input 
  zmsg_viewer<> zmsgv( req );
  bool activate = zmsgv.get_section<bool>(0).front();
  //get pending request by hdr.tag
  //fetch the LibId, and requester ID
  std::lock_guard guard( pmr );
  auto[libKey, rid ] = pmr.get_ids( hdr.tag );

  auto& adss = dsr.find_adss( rid ).value().get();
  //activate or deactivate claim
  adss.update_claim( libKey, activate, nid );
  //create client message
  auto client_msg = client_utils::start_routed_message(
                    ctrl::resp, {},  rid, libKey );
  client_msg.add_arbitrary_data( activate );
  client_msg.finalize();

  out.push_back( std::move( client_msg.get_zmsg() ) ); 

  //clean up pending message
  pmr.remove_message( hdr.tag );

  return out;
}
//////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////
std::optional< configfile >
accel_service::_import_configfile( std::string host_file )
{
  std::string file_content;
  configfile cfg;
  if( !host_file.empty() )
  {
      cfg.import( host_file );
      return std::move(cfg);
  }

  return {};
}
