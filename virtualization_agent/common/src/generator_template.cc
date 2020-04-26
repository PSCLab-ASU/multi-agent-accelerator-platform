#include "generator_template.h"
#include "device_req_resp.h"
#include "system_req_resp.h"
#include "utils.h" 

////////////////////////////////////////////////////////////
#define GEN_TYPES device_request_ll, device_response_ll, \
                  system_request_ll, system_response_ll
////////////////////////////////////////////////////////////
using pki = pktgen_interface<GEN_TYPES>;
using pg  = persistent_generator<GEN_TYPES>;
////////////////////////////////////////////////////////////

template<typename ... Ts>
unique_generator<Ts...>::unique_generator()
{
  running_id.store(1);
}

template<typename ... Ts> template<typename T>
T unique_generator<Ts...>::generate()
{
  ulong next_tid = _generate_tid();
  T element(next_tid);
  return element;

}

template<typename ... Ts>
ulong unique_generator<Ts...>::_generate_tid()
{
  //transaction of zero 
  return running_id++;
}

template<typename ... Ts>
persistent_generator<Ts...>::persistent_generator()
: unique_generator<Ts...>()
{

}

template<typename ... Ts> template<typename T>
std::shared_ptr<T>& persistent_generator<Ts...>::generate()
{
  //generates a request or response and ties
  //a transaction id
  T element = unique_generator<Ts...>::template generate<T>();
  decltype(auto) el_shptr = _add_entry( element );

  return el_shptr;
 
}

template<typename... Ts> 
typename persistent_generator<Ts...>::gen_types&
persistent_generator<Ts...>::get_entry( ulong tid, pico_return& ret)
{
  std::lock_guard<std::mutex> guard(_alock);
  if ( _holders.find( tid ) != _holders.end() )
  {
    return _holders.at( tid );
  }
  else
  {
    ret.set_error_msg( pico_return::ERR, "Could not find holder");
    return _holders.at( tid );
  }

}


template<typename... Ts> template<typename T>
std::shared_ptr<T>& persistent_generator<Ts...>::_add_entry(const T& el)
{
  std::lock_guard<std::mutex> guard(_alock);
 
  using shT = std::shared_ptr<T>;
  const ulong tid = el.get_self_tid();
 
  shT sh_el = std::make_shared<T>( tid );

  if( _holders.find(tid) != _holders.end() )
  {
    std::cout << "tid = " << tid <<" already exists" <<  std::endl;
  }
  else
  {
    _holders[ tid ] = sh_el;
  }
 
  return std::get<shT>( _holders[tid] );

}


template<typename... Ts>
bool persistent_generator<Ts...>::try_remove(ulong tid)
{
  auto& var_entry = _holders[tid];
  /////// /////////////////////////////////////////////////
  auto recur_decr_rem = [&](ulong ptid)->std::optional<ulong> 
  {
    std::optional<ulong> next_ptid;
    auto& lentry = this->_holders[ptid];

    std::visit([&]( auto& entry ) 
    {
      if(entry->get_refcount() )
      {
        next_ptid.reset();
        entry->decr_refcount();
      }

      if(entry->get_refcount() == 0 )
      {
        next_ptid = entry->get_predecesor_tid();
        this->_holders.erase(ptid);
      }

    }, lentry );

    return next_ptid;

  };
  /////////////////////////////////////////////////////////
  std::visit([&]( auto& entry )
  {
    //if there are successors req or resp
    //skip removal
    if( entry->get_refcount() ==0 );
    else return; //hopefully breaks out of the visit

    //if refcount is equal to zero remove dependence
    std::optional<ulong> pred = entry->get_predecesor_tid();
    //run through the chain of dependend pkts
    //and delete tthem
    std::lock_guard<std::mutex> guard(this->_alock);
    while ( pred )
    {
      if( this->_holders.find(*pred) != this->_holders.end() )
      {
        //check and remove dependencies
        pred = recur_decr_rem( *pred );       
      }
      else
      {
        pred.reset(); //reset the tid
        std::cout << "Could not find dep tid: " << *pred << std::endl;
        break;
      }
    } //end of while loop
    //since we arleady check the ref cound for tid
    //at this point it should be deleted
    _holders.erase(tid);

  }, var_entry); //end of visit
  
  /////////////////////////////////////////////////////////////////////
}

template<typename... Ts>
std::pair<pico_utils::pico_ctrl, std::string> pktgen_interface<Ts...>::get_method_from_tid( ulong tid )
{
  std::string s_method;
  pico_utils::pico_ctrl e_method;

  pico_return pret{};

  auto& entry = pg::get_entry(tid, pret);

  std::visit([&](auto pkt)
  {
    s_method = pkt->get_str_method();
    e_method = pkt->get_method(); 
  }, entry);

  return std::make_pair(e_method, s_method);
}

template<typename... Ts>
target_sock_type pktgen_interface<Ts...>::get_source_from_tid( ulong tid )
{
  target_sock_type source;
  pico_return pret{};

  auto& entry = pg::get_entry(tid, pret);
  std::visit([&](auto pkt){ source = pkt->get_source(); }, entry);
  return source;
}

template<typename... Ts>
template<typename SourceT, typename DestT>
std::shared_ptr<DestT>&
pktgen_interface<Ts...>::opt_generate( std::optional<SourceT>&& source)
{
  pico_return pret{};
  //print_types<DestT>();
  auto& new_req = pg::template generate<DestT>();
  //if there is a source link to new packet
  if ( source )
  {
    if constexpr ( std::is_same_v<SourceT, ulong> )
    { 
      auto& var_entry = get_entry( *source, pret);
      std::visit([&](const auto& entry )
      {
        new_req->set_header(entry->get_header() );
        new_req->link( *entry );

      }, var_entry);
    }
    else
    {
      //set the default header information
      new_req->set_header( source.value().get_header() );
      new_req->link( source.value() );
    }
  }

  return new_req;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
template pg::persistent_generator();
template pki::pktgen_interface();
////////////////////////////////////////////////////////////////////////////////////////////////////
template std::shared_ptr<system_request_ll>&  pg::generate<system_request_ll>();
template std::shared_ptr<system_response_ll>& pg::generate<system_response_ll>();
template std::shared_ptr<device_request_ll>&  pg::generate<device_request_ll>();
template std::shared_ptr<device_response_ll>& pg::generate<device_response_ll>();
////////////////////////////////////////////////////////////////////////////////////////////////////
template bool pg::try_remove(ulong);
////////////////////////////////////////////////////////////////////////////////////////////////////
template pg::gen_types& pg::get_entry(ulong, pico_return&);
////////////////////////////////////////////////////////////////////////////////////////////////////
template std::shared_ptr<system_request_ll>&  pg::_add_entry(const system_request_ll&);
template std::shared_ptr<device_request_ll>&  pg::_add_entry(const device_request_ll&);
template std::shared_ptr<system_response_ll>& pg::_add_entry(const system_response_ll&);
template std::shared_ptr<device_response_ll>& pg::_add_entry(const device_response_ll&);
////////////////////////////////////////////////////////////////////////////////////////////////////
template std::pair<pico_utils::pico_ctrl,std::string> pki::get_method_from_tid( ulong );
////////////////////////////////////////////////////////////////////////////////////////////////////
template target_sock_type  pki::get_source_from_tid( ulong );
////////////////////////////////////////////////////////////////////////////////////////////////////
template std::shared_ptr<device_request_ll>&  pki::opt_generate(std::optional<device_request_ll>&&);
template std::shared_ptr<device_request_ll>&  pki::opt_generate(std::optional<system_request_ll>&&);
template std::shared_ptr<system_request_ll>&  pki::opt_generate(std::optional<system_request_ll>&&);
template std::shared_ptr<system_response_ll>& pki::opt_generate(std::optional<system_request_ll>&&);
template std::shared_ptr<device_response_ll>& pki::opt_generate(std::optional<device_request_ll>&&);
template std::shared_ptr<system_response_ll>& pki::opt_generate(std::optional<device_response_ll>&&);
template std::shared_ptr<device_request_ll>&  pki::opt_generate(std::optional<ulong>&&);
template std::shared_ptr<system_request_ll>&  pki::opt_generate(std::optional<ulong>&&);
template std::shared_ptr<system_response_ll>& pki::opt_generate(std::optional<ulong>&&);
template std::shared_ptr<device_response_ll>& pki::opt_generate(std::optional<ulong>&&);
////////////////////////////////////////////////////////////////////////////////////////////////////
