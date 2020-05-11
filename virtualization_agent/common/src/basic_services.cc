#include "basic_services.h"

basic_services::basic_services()
: _bShutdown(false)
{

}

void basic_services::forward_dev_req( ulong tid) //this function foward to device manager
{
  auto& dev_request = get_pktgen()->opt_generate<ulong, device_request_ll>(tid);
  //forwaring data to system responsea
  dev_request->forward_data();
  //send payload to device services
  send_downstream( dev_request->get_self_tid() );
}

void basic_services::forward_sys_resp( ulong tid) //this function forward to collage
{
  using out_type = system_response_ll;
  auto& sys_resp = get_pktgen()->opt_generate<ulong, out_type>(tid);
  //forwaring data to system responsea
  sys_resp->forward_data();
  /////////////////////////////////////////
  send_upstream( sys_resp->get_self_tid() );

}

void basic_services::set_shmem_store ( std::shared_ptr<shmem_store>& shmem)
{
  //_shmem_store = shmem;
  _shmem_store = shmem;
  _pktgen      = shmem->get_item<item_key::PKT_GEN>();
  _pcfg        = shmem->get_item<item_key::PICO_CFG>();
  _kernel_db   = shmem->get_item<item_key::KERNEL_INTF>();
  _pmsg        = shmem->get_item<item_key::PMSG>();

}

void basic_services::set_io_queues( std::pair<std::optional<single_q>, 
                                              std::optional<single_q> > qs)
{
  _main_io_queue = qs;
}

void basic_services::operator()( ulong tid )
{
  pico_return p_ret;  

  auto method_id = _pktgen->get_method_from_tid(tid);

  if( method_id.first != pico_utils::pico_ctrl::sys_shutdown)
  {
    auto action    = action_ctrl( method_id.first );
  
    auto p_return  = action( tid );
  }
  else
  { 
    shutdown();
  }

  //the only time a chain is deleted is when the last action
  //did not lead to an additional dependency
  //meaning, if the dependency chain wasn't altered,
  //the request is considered complete
  //A request can complete or a response can complete
  //this should recursively remove all the dependents
  _pktgen->try_remove( tid ); 
    
}

template< bool direction >
pico_return basic_services::_send_data( const ulong& tid)
{
  if( direction )
  { 
    if( _main_io_queue.second )
    {
      (*_main_io_queue.second)->push( tid );
    }
    else
      std::cout <<"Queue unavailable..." << std::endl;
  }
  else
  {
    if( _main_io_queue.first )
      (*_main_io_queue.first)->push( tid );
    else
      std::cout <<"Queue unavailable..." << std::endl;
  }
  return pico_return{};
}

void basic_services::send_upstream( const ulong& tid )
{
  _send_data<true>( tid );
}

void basic_services::send_downstream( const ulong& tid )
{ 
  _send_data<false>( tid );
}

const base_req_resp::var_bdata_t& basic_services::_get_base_data( const ulong& tid, bool& data_exists) 
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
const T& basic_services::get_misc_payload( const ulong& tid, bool& data_exists)
{
  auto& entry = _get_base_data( tid, data_exists); 
  return std::get<T>( entry );
}

const misc_string_payload& basic_services::get_misc_string_payload( const ulong& tid, bool& data_exists )
{
  return get_misc_payload<misc_string_payload>(tid, data_exists);
}

const identify_payload& basic_services::get_idn_payload( const ulong& tid, bool& data_exists )
{
  return get_misc_payload<identify_payload>(tid, data_exists);
}

const send_payload& basic_services::get_send_payload( const ulong& tid, bool& data_exists )
{
  return get_misc_payload<send_payload>(tid, data_exists);
}
