#include <zmq.hpp>
#include <zmq_addon.hpp>
#include <ncache_registry.h>
#include <client_utils.h> 
#include <string>
#include <vector>
#include <mutex>
#include <atomic>
#include <map>
#include <shared_mutex>
#include <optional>
#include <boost/thread/shared_mutex.hpp>
#include <functional>
#include <nexus_utils.h>

#ifndef ANODEOBJ
#define ANODEOBJ

class anode 
{
  public:

    anode( std::string, zmq::context_t& , std::string ); 
    anode( anode&& );
 
    ~anode()
    {
      _zsock.close();
    }

    int _pre_init();

    int request_manifest();

    //returns the claim_id
    int make_claim( const std::string&, const std::string&, 
                    const std::map<std::string, std::string>& );
    int send( zmq::multipart_t& );

    std::optional<zmq::multipart_t> try_recv( );
   
    void lock_shared(){
      _mu.lock_shared(); 
    }

    void unlock_shared(){
      _mu.unlock_shared();
    }

    void lock(){
      _mu.lock();
    }

    void unlock(){
      _mu.unlock();
    }

    bool isActive( ) const
    { return _bActive; }
  
    std::string get_address()
    { return _nexus_address; }

    std::string get_tx_id()
    { return _tx_id; }
      
    std::string get_rx_id()
    { return _rx_id; }

    void set_rx_id( std::string rid)
    { _rx_id = rid; }

    zmq::socket_t&& move_zsock()
    { return std::move( _zsock); }

    boost::upgrade_mutex&& move_mutex()
    { return std::move(_mu); }

    ncache_registry&& move_nxcache()
    { return std::move(_nxcache); }

    std::vector<std::string>&& move_claim_history()
    { return std::move(_claim_history); }
   
    void activate() { _bActive = true; }
    void deactivate() { _bActive = false; }
 
    void set_nxcache_entries( const std::list<std::string>& );
   
  private:

    bool                  _node_exists; //true if ping returned
    bool                  _bActive;     //true if post_init occured
    std::string           _job_id;
    std::string           _nexus_address;
    std::string           _tx_id; //this is the id for this socket
    std::string           _rx_id; //this is the id for response from nex
    zmq::socket_t         _zsock;
    std::mutex            _zsock_lock;
    boost::upgrade_mutex  _mu;

    //holds the reousrces from the nexus
    ncache_registry       _nxcache;
    //        rank   acceleratorId
    std::vector< std::string > _claim_history;
   
};



#endif
