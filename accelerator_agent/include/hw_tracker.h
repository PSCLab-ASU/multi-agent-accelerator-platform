#include <map>
#include <zmq.hpp>
#include <zmq_addon.hpp>
#include <functional>
#include <iostream>
#include <list>
#include <memory>
#include <zmsg_builder.h>
#include <ncache_registry.h>

#ifndef HWTRACK
#define HWTRACK

class hw_tracker {
  public: 
  hw_tracker();
  hw_tracker(std::string, std::string, zmq::context_t *);
  ~hw_tracker();
 
  friend bool operator==(const hw_tracker &, const std::string );

  //generate envelope for target HW service
  int generate_header( std::string, zmq::multipart_t & );
  //address for the local hardware resourcea
  //each struct points to a nexus addr : zmq_identifier
  std::string id;
  std::string addr;

  //get id
  std::string get_id() const
  {
    return id;
  }
  
  ulong get_congestion() const
  {
    return _congestion;
  }

  bool can_support( std::string claim) const
  {
    return _caches.can_completely_support( claim ); 
  }

  //mthods for hw_tracker
  int request_rank(std::string, std::string,               
                   std::tuple<ulong, ulong, std::string>,
                   std::list<std::string> );

  ncache_registry& edit_caches() { return _caches;}
  const ncache_registry& get_caches() const { return _caches;}

  private: 
    int _send_msg( const zmq::multipart_t& ) const;
    int _recv_msg( zmq::multipart_t& );

    //connection to router
    std::shared_ptr<zmq::context_t> _ctx;
    std::shared_ptr<zmq::socket_t>  _zlink;
    ncache_registry  _caches;
    ulong _congestion;
};

#endif
