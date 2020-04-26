#include "system_services.h"


system_services::system_services()
: basic_services(){}

void system_services::build_command_set()
{
  using ctrl = pico_utils::pico_ctrl;
  auto bind_action = [&]( auto req_func, auto resp_func) 
  {
    auto first   = std::bind(req_func,  this, std::placeholders::_1);
    auto second  = std::bind(resp_func,  this, std::placeholders::_1);
    return std::make_pair(first, second);
  };
 
  command_set[ctrl::reg_resource]   = bind_action(&system_services::_register_resource_req,
                                                &system_services::_register_resource_resp);
  command_set[ctrl::ident_resource] = bind_action(&system_services::_identify_resource_req,
                                                &system_services::_identify_resource_resp);
  command_set[ctrl::alloc_resource] = bind_action(&system_services::_allocate_resource_req,
                                                &system_services::_allocate_resource_resp);
  command_set[ctrl::send]           = bind_action(&system_services::_send_req,
                                                &system_services::_send_resp);
}

basic_services::action_func system_services::action_ctrl(pico_utils::pico_ctrl method)
{
  if( get_command_set_t() == REQUEST_SET )
    return command_set.at( method ).first;
  else
    return command_set.at( method ).second;
}


void system_services::operator()(q_arry& req_qs, q_arry& resp_qs, std::shared_ptr<shmem_store>& _sh_store)
{
  std::cout << "calling sys-function " << __func__ << std::endl;
  main_system_qs = std::make_pair(req_qs[0], resp_qs[0]);
  main_device_qs = std::make_pair(req_qs[1], resp_qs[1]);
  //set queues for the base methods
  //send upstream and downstream
  set_io_queues( std::make_pair(req_qs[1], resp_qs[0]) );

  set_shmem_store( _sh_store );
 
  build_command_set();

  ulong tid=0;
  while(true)
  {
    //1) read request from main input queue
    set_command_set_t(REQUEST_SET);
    bool succ = main_system_qs.first->consume_one(*this);
    if ( is_shutdown() ) break; 

    //2) read response from device service queue
    set_command_set_t(RESPONSE_SET);
    main_device_qs.second->consume_one(*this);
   
  } 
  std::cout << "Shutting down system_services" << std::endl;

}

std::shared_ptr<system_request_ll>& 
system_services::get_system_request( const ulong& tid, pico_return& pret )
{
  return get_entry<system_request_ll>( tid, pret);
}

std::shared_ptr<device_response_ll>&
system_services::get_device_response( const ulong& tid, pico_return& pret )
{
  return get_entry<device_response_ll>( tid, pret);
}
///////////////////////////////////////////////////////////////////////////
//////////////////////COMMAND SETS////////////////////////////////////////
pico_return system_services::_send_req( ulong tid )
{
  std::cout << "system_services : " << __func__ << std::endl;
  pico_return pret;
  //forwarding packets from collage to device services
  forward_dev_req( tid );

  return pret;
}

pico_return system_services::_send_resp( ulong tid )
{
  std::cout << "system_services : " << __func__ << std::endl;
  pico_return pret;
  //forwards packet from device services to collage
  forward_sys_resp( tid );
  
  return pret;
}

pico_return system_services::_register_resource_req( ulong tid )
{
  std::cout << "system_services : " << __func__ << std::endl;
  auto& dev_request = get_pktgen()->opt_generate<ulong, device_request_ll>(tid);
  send_downstream( dev_request->get_self_tid() );
  return pico_return{};
}

pico_return system_services::_register_resource_resp( ulong tid )
{
  std::cout << "system_services : " << __func__ << std::endl;
  using out_type = system_response_ll; 

  pico_return pret;
  auto& sys_resp = get_pktgen()->opt_generate<ulong, out_type>(tid);
  //forwaring data to system responsea
  sys_resp->forward_data();
  //////////////////////////////////////////
  send_upstream( sys_resp->get_self_tid() );

  return pico_return{};
}

pico_return system_services::_identify_resource_req( ulong tid )
{
  std::cout << "system_services : " << __func__ << std::endl;
  auto& dev_request = get_pktgen()->opt_generate<ulong, device_request_ll>(tid);
  dev_request->forward_data(); 
  send_downstream( dev_request->get_self_tid() );

  return pico_return{};
}

pico_return system_services::_identify_resource_resp( ulong tid )
{
  std::cout << "system_services : " << __func__ << std::endl;
  using out_type = system_response_ll; 

  pico_return pret;
  bool data_exists;

  std::vector<std::string> out_payload;
  auto& sys_resp = get_pktgen()->opt_generate<ulong, out_type>(tid);
  auto& dev_resp = get_device_response( tid, pret );
  /////////////////////////////////////////////////////////////////
  auto& payload =  dev_resp->get_dev_data<dev_kernel_payload>(); 
  for( auto entry : payload ) //one entry per file
  {
    auto fid = entry.fid;
    auto func_ids = entry.get_func_data(); //vector_pair (func_id, num_dev)
    for(auto func_id : func_ids)
    {
      std::string cline;
      get_kintf()->get_kernel_cline( fid, func_id.first, cline);
      out_payload.push_back( cline );
      //std::cout << "(" << fid << "," << func_id.first << ") = " << cline << std::endl;

    }
  }
  //move data to system response
  misc_string_payload msp( out_payload );
  sys_resp->set_data( msp );   

  /////////////////////////////////////////////////////////////////
  send_upstream( sys_resp->get_self_tid() );

  return pico_return{};
}

pico_return system_services::_allocate_resource_req( ulong tid )
{
  std::cout << __func__ << std::endl;
  return pico_return{};
}

pico_return system_services::_allocate_resource_resp( ulong tid )
{
  std::cout << __func__ << std::endl;
  return pico_return{};
}
