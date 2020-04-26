#include <zmq.h>
#include <zmq_addon.hpp>
#include <list>
#include <string>
#include <set>
#include <zmsg_builder.h>
#include <nexus_perfmon.h>
#include <memory>
#include <algorithm>
#include <config_components.h>

#ifndef HWNEXCACHE
#define HWNEXCACHE

#define NEX_CACHE_PARTS 11
typedef struct _nex_cache_data{

 //constructors
 _nex_cache_data(std::string, std::string );

 //methods
 void add_cache_data(std::list< std::string>);

 const std::list<std::string>& get_raw_cachelines() const;

 //generates connstring string with tcp:// in front of it
 std::string conn_str();
 
 const std::string get_hostname() const { return host_name; };

 const std::string get_hostaddr() const { return host_addr; };
 //members
 zmq::context_t ctx;
 zmq::socket_t  sock;
 std::string    host_name; //ex. moore-1
 std::string    host_addr; //ex. moore-1.eng.asu.edu:8000
 bool           bActive;
 bool           bValid;
 float          timestamp;
 std::list<std::string> as_is;

} nex_cache_data;

typedef struct _hw_nex_cache : nex_cache_data{

  using nex_msg_type = zmsg_builder<std::string, std::string, std::string>;
 
  //first: identity,  hostaddr: tcp://...,
  _hw_nex_cache(std::string, std::string);

  bool is_local(){ return host_name == get_hostname(); } 
 
  const std::map<valid_function_header, std::string>
        transform_nexcache_view( std::string ) const;

  //send the data
  int send(zmq::multipart_t& mp);
  //init job 
  int recv(zmq::multipart_t& mp);
  //send the data
  int sendrecv(zmq::multipart_t& mp);
  
  int _add_header(zmq::multipart_t& mp);

  bool contains_function( const function_entry& ) const;

  const std::list<std::string>
  get_pico_service_list(const function_entry& fe) const;

  //std overrides

  friend bool operator==(std::shared_ptr<_hw_nex_cache>&, 
                         std::string&);

  friend bool operator==(std::shared_ptr<_hw_nex_cache>&, 
                         std::shared_ptr<_hw_nex_cache>&);

  //member variables
  std::unique_ptr<nexus_perfmon> nex_perfmon_ptr;

} hw_nex_cache;

typedef std::shared_ptr<hw_nex_cache> nex_cache_ptr;
typedef std::shared_ptr<const hw_nex_cache> cnex_cache_ptr;





#endif 
