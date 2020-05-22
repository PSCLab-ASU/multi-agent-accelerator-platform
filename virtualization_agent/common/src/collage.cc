#include "collage.h"
#include <thread>
#include <nexus_utils.h>
#include <type_traits>

collage::collage() 
{
  
  using ctrl = pico_utils::pico_ctrl;

  auto bind_action = [&](auto req_func, auto resp_func)
  {
    auto first  = std::bind(req_func,  this, std::placeholders::_1);
    auto second = std::bind(resp_func, this, std::placeholders::_1, std::placeholders::_2);
    return std::make_pair(first, second);
  };

  command_set[ctrl::reg_resource]   = bind_action(&collage::_col_reg_resource_req, 
                                                  &collage::_col_reg_resource_resp);
  command_set[ctrl::ident_resource] = bind_action(&collage::_col_idn_resource_req,
                                                  &collage::_col_idn_resource_resp);
  command_set[ctrl::send]           = bind_action(&collage::_col_send_req,
                                                  &collage::_col_send_resp);
  //command_set[ctrl::recv]           = bind_action(nullptr,
  //                                                &collage::_col_recv_resp);
}

void collage::print_header()
{
  std::cout << std::endl << "requester : "   << _crh.requester; 
  std::cout << std::endl << "job_id : "      << _crh.job_id; 
  std::cout << std::endl << "owning_rank : " << _crh.owning_rank; 
  std::cout << std::endl << "method : "      << _crh.get_str_method() << std::endl; 
}

collage::collage(const ulong q_size )
: collage()
{
  std::cout << "collage::collage(" << q_size <<")" << std::endl;

  _req_qs[0]  = std::make_shared<single_q::element_type >(q_size);
  _req_qs[1]  = std::make_shared<single_q::element_type >(q_size);
  _resp_qs[0] = std::make_shared<single_q::element_type >(q_size);
  _resp_qs[1] = std::make_shared<single_q::element_type >(q_size);
  
  _sys_serv.second = std::move(system_services());
  _dev_serv.second = std::move(device_services());
  _shmem_store     = std::make_shared<shmem_store>();
  _pktgen          = _shmem_store->get_item<item_key::PKT_GEN>();
  _pcfg            = _shmem_store->get_item<item_key::PICO_CFG>();
}

pico_return collage::init_services()
{

  std::cout << "calling function " << __func__ << std::endl;
  //kick off functor form each service
  //collage to system writer is _req_qs[0]
  //system writer to collage is _resp_qs[0]
  //system writer to device is _req_qs[1]
  //device to system is _resp_qs[1]
  //note: system class need both queue ptr; one for down stream and upstream
  _sys_serv.first = std::move( std::thread(std::ref(_sys_serv.second),
                                           std::ref(_req_qs), 
                                           std::ref(_resp_qs),
                                           std::ref(_shmem_store)) );
  
  _dev_serv.first = std::move( std::thread(std::ref(_dev_serv.second ), 
                                           std::ref(_req_qs[1]), 
                                           std::ref(_resp_qs[1]),
                                           std::ref(_shmem_store)) );
  
  return pico_return{};
}


pico_return collage::set_external_address( std::string ext_addr)
{ 
  std::cout << "calling function " << __func__ << std::endl;
  return _pcfg->set_external_address( ext_addr );
}

pico_return collage::set_owner( std::string owner)
{
  std::cout << "calling function " << __func__ << std::endl;
  return _pcfg->set_owner( owner );
}

pico_return collage::set_repo( std::string repo_path, bool bAppend)
{
  std::cout << "calling function " << __func__ << std::endl;
  return _pcfg->set_repos( repo_path, bAppend  );
}

pico_return collage::set_filters( std::string vidpids)
{
  std::cout << "calling function " << __func__ << std::endl;
  return _pcfg->set_filters( vidpids );
}

pico_return collage::_create_transaction_resp( zmq::multipart_t & rep)
{
  zmsg_builder zb(_crh.requester, _crh.job_id, _crh.owning_rank, _crh.get_str_method() );

  zb.add_arbitrary_data( _crh.claim_id)
    .add_arbitrary_data( _crh.transaction_id)
    .finalize();

  rep = std::move( zb.get_zmsg() );

  return pico_return{};
}

std::pair<collage::req_action_func, collage::resp_action_func> 
collage::_action_ctrl( std::string method_str )
{
  auto method = pico_utils::reverse_lookup( method_str );

  return command_set.at(method);
}

template< ushort stage = 0>
pico_return collage::_send_downstream( auto& req  )
{
  using namespace std::chrono_literals;
  pico_return pret{}; 
  auto& q       = _req_qs[stage];
  ulong tid     = req->get_self_tid();
  
  q->push( tid ); 
  return pret;
}

const base_req_resp::var_bdata_t& collage::_get_base_data( const ulong& tid, bool& data_exists)
{
   pico_return pret;
   data_exists = false;
   const base_req_resp::var_bdata_t * pdata=nullptr;

   auto& v_entry = _pktgen->get_entry( tid, pret );

   std::visit([&](auto& entry){

     if( entry->is_deferred() )
     {
       ulong pred = entry->get_predecesor_tid().value();
       std::cout << "entry is deferred : " << pred <<  std::endl;
       auto& v_pred_entry = _pktgen->get_entry( pred, pret );
       std::visit([&](auto& pred_entry)
       {
         data_exists = pred_entry->data_exists();
         pdata = &( pred_entry->get_data() );

       }, v_pred_entry);

     }
     else
     {
       std::cout << "entry is NOT deferred : " << tid <<  std::endl;
       data_exists = entry->data_exists();
       pdata = &( entry->get_data() );
     }

   }, v_entry);

  return *pdata;
}

template<typename T>
const T& collage::_get_misc_payload( const ulong& tid, bool& data_exists)
{
  auto& entry = _get_base_data( tid, data_exists);
  return std::get<T>( entry );
}

std::shared_ptr<system_response_ll>&
collage::_get_system_response( const ulong& tid, pico_return& pret )
{
  std::cout << "collage : " << __func__ << std::endl;
  return _get_entry<system_response_ll>( tid, pret);
}

const misc_string_payload& collage::_get_misc_string_payload ( const ulong& tid, bool& data_exists )
{
  return _get_misc_payload<misc_string_payload>(tid, data_exists);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////submit & process queues/////////////////////////////
pico_return collage::submit_request(target_sock_type ts, zmq::multipart_t& req )
{
  std::cout << "--calling function " << __func__ << " : " << req.empty()<< std::endl;
  pico_return p_status;
  std::string s_method;
  //move data to member varaibles
  if( !req.empty() ) 
  {
    std::cout << "submitting : " << req << std::endl;
    _crh.source      = ts;
    _crh.set_method ( pico_utils::reverse_lookup( req.popstr() ) );
    _crh.set_key    ( req.popstr() );
  
    
    std::cout << "_crh.method : " << _crh.get_str_method() << std::endl;
    std::cout << "_crh.key : "    << _crh.get_key() << std::endl;
  
    //get & process action
    p_status = _action_ctrl( _crh.get_str_method() ).first(std::move(req) );
  }
  else
  {
    std::cout << "Incompatible msg, Ignoring msg!" << std::endl;
  }
  return p_status;
}

pico_return collage::process_q_resp( zmq::multipart_t & resp, target_sock_type& ts)
{
  //std::cout << "calling function " << __func__ << std::endl;
  ulong tid;
  pico_return p_status{};
  ts = target_sock_type::TST_NONE;
  
  bool succ = _resp_qs[0]->pop( tid );
  if ( succ )
  {
    std::string method = _pktgen->get_method_from_tid( tid ).second; 
    ts                 = _pktgen->get_source_from_tid( tid );
    p_status           = _action_ctrl( method ).second(tid, resp);
    //removing entry from shared memory
    _pktgen->try_remove( tid );
  }
  
  return p_status;
}

pico_return collage::shutdown_services()
{
  std::cout << "calling function " << __func__ << std::endl;
  ////////////////////////////////////////////////////////////////////////////
  auto& req = _pktgen->opt_generate<system_request_ll>(); 
  req->set_method( pico_utils::pico_ctrl::sys_shutdown ); 
  _send_downstream( req );
  ////////////////////////////////////////////////////////////////////////////
  ////////////////////////////////////////////////////////////////////////////
  req = _pktgen->opt_generate<system_request_ll>(); 
  req->set_method( pico_utils::pico_ctrl::sys_shutdown ); 
  _send_downstream<1>( req);
  ////////////////////////////////////////////////////////////////////////////

   _sys_serv.first.join();
   _dev_serv.first.join();
   
  std::cout << "_deallocate shared_mem store" << std::endl;
   _deallocate_shmem_store();

  return pico_return{};
}

void collage::_deallocate_shmem_store()
{
  //NEED TO DEALLOCATE SHMEM STORE TBD
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
//command set
pico_return collage::_col_send_req( PICO_IN_TYPE ) //vars are request, and reply
{
  std::cout << "entering " << __func__ << std::endl;
  zmsg_viewer<>  request(zrequest);
  send_payload::base_type_t     base_msgs;
  send_payload::arg_headers_t   argh;

  auto& req = _pktgen->opt_generate<system_request_ll>();  //generate shared_ptr<DestT>&
  ////////////////////////////////////////////////////////
  auto resource_str = request.get_section<std::string>(0).front();
  auto nargs        = request.get_section<ulong>(1).front();
  ////////////////////////////////////////////////////////
  //set the header for the request
  req->set_header( _crh );

  //The two is based on the header section ( resource_str and nargs )
  for(auto i : iota((ulong)2,nargs+1) )
  {
    auto[b_sign, b_type, type_size, v_size, msg] = 
      request.get_memblk(i); 
 
    //push header
    argh.emplace_back( (arg_sign_t) b_sign, (arg_data_t) b_type, 
                       type_size, v_size );
    //push message
    base_msgs.push_back( std::move( msg ) );
  }
  //set the data
  send_payload data( std::move(argh), std::move(base_msgs) );
  //set the resource
  data.set_resource ( resource_str );
  //move it to the data
  req->set_data( std::move(data) );
  ////////////////////////////////////////////////////////
  //send data to system services
  ////////////////////////////////////////////////////////
  _send_downstream( req );

  return pico_return{};
}

pico_return collage::_col_send_resp( PICO_OUT_TYPE ) //vars are request, and reply
{
  std::cout << "entering " << __func__ << std::endl;
  using in_type = std::shared_ptr<system_response_ll>; 

  pico_return pret;
  bool data_exists = false;
  
  auto& sys_resp = _get_system_response(tid, pret);
  auto& payload  = _get_misc_payload<recv_payload>(tid, data_exists);
  auto header    = sys_resp->get_header();

  if( data_exists ) 
  {
    auto nargs  = payload.get_size();

    std::string key  = header.get_key();
    
    auto mbuilder = nexus_utils::start_request_message( nexus_utils::nexus_ctrl::nex_recv, key);
  
    //add number of args to return
    mbuilder.add_arbitrary_data((ulong) nargs);  
    
    for(auto i : iota((size_t)0, nargs) )
    {
      auto [header, data] = payload.pop_output();
      auto[sign_v, d_type, type_size, vec_size] = header;
      mbuilder.add_memblk(sign_v, d_type, type_size,
                          data, vec_size );
    }
    mbuilder.finalize();  
  
    resp = std::move(mbuilder.get_zmsg() );
    
  }
   
  return {};

}

pico_return collage::_col_recv_resp( PICO_OUT_TYPE ) //vars are request, and reply
{
  std::cout << "entering " << __func__ << std::endl;
  return {};

}

pico_return collage::_col_reg_resource_req( PICO_IN_TYPE ) //vars are request, and reply
{
  std::cout << "entering " << __func__ << std::endl;
  zmsg_viewer<>  request(zrequest);

  auto& req = _pktgen->opt_generate<system_request_ll>();  //generate shared_ptr<DestT>&
  ////////////////////////////////////////////////////////////////////////////
  //FILL IN REQUEST HERE!  
  req->set_header( _crh );

  ////////////////////////////////////////////////////////////////////////////
  //send data to system services  
  _send_downstream( req );

  return pico_return{};
}

pico_return collage::_col_reg_resource_resp( PICO_OUT_TYPE ) // tid, zmq::multipart_t& resp
{
  using in_type = std::shared_ptr<system_response_ll>; 

  pico_return pret;
  bool data_exists = false;
  
  std::cout << "entering " << __func__ << std::endl;
  auto& sys_resp = _get_system_response(tid, pret);
  auto& payload  = _get_misc_string_payload(tid, data_exists);
  auto header    = sys_resp->get_header();
  ////////////////////////////////////////////////////////////////////////////
  //FILL IN REQUEST HERE!  
  if( data_exists ) 
  {
    //auto [requester, job_id, rank_id] = header.get_zheader();
    std::string key  = header.get_key();
    auto raw_data = payload.get_data();
    auto ext_addr = raw_data[0];
    //remove the ext address from the list
    raw_data.erase(raw_data.begin());
    
    auto mbuilder = nexus_utils::start_request_message( nexus_utils::nexus_ctrl::hw_reg, key);

    std::list<std::string> ll;
    ll.insert(ll.begin(), raw_data.begin(), raw_data.end() );

    mbuilder.add_arbitrary_data( ext_addr )
            .add_sections( ll )
            .finalize();

    resp = std::move(mbuilder.get_zmsg() );

    std::cout << "payload exists" << std::endl;
    
  }
   
  return pico_return{};
}

pico_return collage::_col_idn_resource_req( PICO_IN_TYPE ) //vars are request, and reply
{

  std::cout << "entering " << __func__ << std::endl;
  zmsg_viewer<>  request(zrequest);

  auto& sys_req = _pktgen->opt_generate<system_request_ll>();  //generate shared_ptr<DestT>&
  ////////////////////////////////////////////////////////////////////////////
  //FILL IN REQUEST HERE!  
  sys_req->set_header( _crh );
  auto append    = request.get_section<bool>(0);
  auto repo_dirs = request.get_section<std::string>(1);

  std::vector<std::string> repo_dir_vec(repo_dirs.begin(), repo_dirs.end() );

  identify_payload data(append.front(), repo_dir_vec );

  sys_req->set_data( data );
  ////////////////////////////////////////////////////////////////////////////
  //send data to system services  
  _send_downstream( sys_req );

  return pico_return{};

}

pico_return collage::_col_idn_resource_resp( PICO_OUT_TYPE ) //vars are request, and reply
{
  using in_type = std::shared_ptr<system_response_ll>; 
  pico_return pret;
  bool data_exists = false;

  std::cout << "entering " << __func__ << std::endl;
  auto& sys_resp = _get_system_response(tid, pret);
  auto& payload  = _get_misc_string_payload(tid, data_exists);
  auto header    = sys_resp->get_header();

  ////////////////////////////////////////////////////////////////////////////
  //FILL IN REQUEST HERE!  
  if( data_exists ) 
  {
    auto [requester, job_id, rank_id] = header.get_zheader();
    auto raw_data = payload.get_data();
    std::list<std::string> l_raw_data(raw_data.begin(), raw_data.end() );

    zmsg_builder mbuilder( job_id, rank_id, 
                           std::string("__HW_UPDATE__") );
   
    mbuilder.add_sections( l_raw_data ).finalize();

    resp = std::move(mbuilder.get_zmsg() );

    std::cout << "payload exists" << std::endl;
    
  }

  return pico_return{};
}
