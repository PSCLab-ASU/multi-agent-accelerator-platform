#include <array>
#include <thread>
#include <mutex>
#include <string>
#include <type_traits>
#include <memory>
#include <tuple>
#include <utility>
#include <atomic>
#include <optional>
#include <variant>
#include <map>
#include <mutex>
#include <iostream>
#include <vector>
#include <list>
#include <algorithm>
#include <any>
#include "payload_details.h"
#include <pico_utils.h>
#include <boost/range.hpp>
#include <boost/range/algorithm/copy.hpp>
#include <boost/assign.hpp>
#include <boost/lockfree/queue.hpp>

#ifndef BASEUTILS
#define BASEUTILS

#define NUM_OF_Q 2

enum struct target_sock_type { TST_NONE=0, TST_RTR, TST_DEALER };
enum struct PKT_TYPE {REQ=0, RESP};

//need pointer to dodge the trivial construction for boost lockfree ques
//need shared pointer to do automatic deallocation
using fixed_size = boost::lockfree::fixed_sized<true>;
using single_q   = std::shared_ptr< boost::lockfree::queue<ulong, fixed_size> >;

//////////////////////////////////////////////////////////////////////////
using q_arry     = std::array<single_q, NUM_OF_Q >;

//////////////////////////////////////////////////////////////////////////

struct pico_return
{
  enum { SUCC=0, ERR };

  pico_return(std::optional<int> val={}, std::optional<std::string> msg={})
  {
    ret_val   = val.value_or(0);
    error_msg = msg.value_or("Success");
  };

  operator bool() { return ret_val != 0; }

  pico_return& update()
  {
    char msg_ptr;
    int len;
    return *this;
  };
 
  void set_error_msg( int err, std::string msg="")
  {
    ret_val   = err;
    error_msg = msg;    
  }

  int ret_val;
  std::string error_msg;
};

struct header_info
{
  pico_utils::pico_ctrl method;
  target_sock_type      source;
  ulong                 transaction_id;
  std::string           requester;
  std::string           job_id;
  std::string           owning_rank;
  std::string           claim_id;
  std::string           _key;
  /////////////////////////////////////////////
  /////////////Operator////////////////////////
  header_info& operator =(const header_info& rhs) = default; 
  
  /////////////////////////////////////////////
  std::tuple<std::string, std::string, std::string>
  get_zheader() { return std::make_tuple(requester, job_id, owning_rank ); }

  void set_key( std::string key )
  {
    _key = key;
  }

  void  set_method( pico_utils::pico_ctrl meth )
  {
    method = meth; 
  }

  const std::string get_key()              const { return _key; }
  const pico_utils::pico_ctrl get_method() const { return method; }
  const std::string get_str_method()       const { return pico_utils::pico_rolodex.at(method); }
  const target_sock_type get_source()      const { return source; }
};

template <typename T>
struct ownership{

  using type = T;
 
  ownership( T* o) : owner(o) {}
 
 ownership( std::shared_ptr<T>& o) : owner(o) {}

  ownership& operator=( ownership&& source_o){ 
    return std::move(source_o); 
  }

  ownership& get_shared_ptr(){ return owner; }

  std::shared_ptr<T> owner;
};

struct base_req_resp
{
  using var_bdata_t = 
        std::variant<misc_string_payload,
                     send_payload,
                     recv_payload,
                     identify_payload,
                     register_payload>; 

  base_req_resp( ulong self_id ) : _self_tid(self_id){}

  const ulong get_self_tid() const { return _self_tid; }
 
  const std::optional<ulong> get_predecesor_tid() const { return _predecesor_tid; }
  
  ulong get_refcount() {return _successor_count.load(); } 
  void incr_refcount() { _successor_count++; }
  void decr_refcount() { _successor_count--; }

  void link( auto& orig_pkt )
  {
    //pass transaction id to new packet
    _predecesor_tid = orig_pkt->get_self_tid();   
    //increment dependency in original packet
    orig_pkt->incr_refcount();
  }
  bool data_exists ()                                 { return (bool) _base_data; };
  //void set_data (  const var_bdata_t& data )          { _base_data = data; };
  void set_data (  var_bdata_t&& data )               { _base_data = std::forward<var_bdata_t>(data); };
  void set_method( const pico_utils::pico_ctrl& meth) { _header.set_method( meth ); };
  void set_header( const header_info& hdr)            { _header = hdr; };
  
  const var_bdata_t&           get_data()       const { return _base_data.value(); }
  const header_info&           get_header()     const { return _header; }
  const std::string            get_str_method() const { return _header.get_str_method(); }
  const pico_utils::pico_ctrl  get_method()     const { return _header.get_method(); }
  const target_sock_type       get_source()     const { return _header.get_source(); }

  //data accessors
  template<typename T>
  misc_payload<T> get_misc_payload() { return data_accessor<misc_payload<T> >(); }; 

  template <typename T>
  T data_accessor() const 
  {
    return std::get<T>( _base_data.value_or( var_bdata_t(T{}) ) );
  }

  void forward_data() { _defer_to_predecesor = true; }
  const bool is_deferred() { return _defer_to_predecesor; }

  bool                 _defer_to_predecesor;
  const ulong          _self_tid;
  std::atomic_ulong    _successor_count;
  std::optional<ulong> _predecesor_tid;
  header_info          _header;
  std::optional<var_bdata_t> _base_data;
 
};

#endif
