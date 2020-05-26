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
#include <config_components.h>

#ifndef ANODEOBJ
#define ANODEOBJ

class anode 
{
  public:

    anode( std::string jid, zmq::context_t& zctx, host_entry he, 
           std::optional<std::string> bridge_parms, bool is_local) :
      anode( jid, zctx , he.get_ext_connstr(), bridge_parms, is_local )
    {
       //adding host entry to anode 
       _host_desc = he; 
       //bridge param
       _bridge_parms = bridge_parms;
    }
      
    anode( std::string, zmq::context_t& , std::string, std::optional<std::string>, bool ); 
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

    std::optional<std::string> get_bp()
    { return _bridge_parms; }

    void set_rx_id( std::string rid)
    { _rx_id = rid; }

    zmq::socket_t&& move_zsock()
    { return std::move( _zsock); }

    boost::upgrade_mutex&& move_mutex()
    { return std::move(_mu); }

    ncache_registry&& move_ncache()
    { return std::move(_ncache); }

    std::vector<std::string>&& move_claim_history()
    { return std::move(_claim_history); }
   
    void activate() { _bActive = true; }
    void deactivate() { _bActive = false; }
 
    void set_nxcache_entries( const std::list<std::string>& );

    bool can_completely_support( std::string resource ) const;

    bool can_support( std::pair<std::string, std::string> ,
                      std::optional<
                        std::pair<std::string, std::string> > ) const;
    size_t get_outstanding_msg_cnt() const;

    std::optional<std::array<std::string, 3> >
      is_fetch_mpi_data();

    bool get_locality() { return _is_local; }

  private:
    bool                  _is_local;
    bool                  _node_exists; //true if ping returned
    bool                  _bActive;     //true if post_init occured
    size_t                _outstanding_msg_cnt=0;
    std::string           _job_id;
    std::string           _nexus_address;
    std::string           _tx_id; //this is the id for this socket
    std::string           _rx_id; //this is the id for response from nex
    //bridge param
    std::optional<std::string> _bridge_parms;
    zmq::socket_t         _zsock;
    std::mutex            _zsock_lock;
    boost::upgrade_mutex  _mu;

    //holds the reousrces from the nexus
    ncache_registry       _ncache;
    //        rank   acceleratorId
    std::vector< std::string > _claim_history;
    host_entry _host_desc;
};



#endif
