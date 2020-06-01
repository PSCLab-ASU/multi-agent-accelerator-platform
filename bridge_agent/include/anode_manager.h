#include <string>
#include <list>
#include <anode.h>
#include <vector>
#include <mutex>
#include <shared_mutex>
#include <zmq.hpp>
#include <zmq_addon.hpp>
#include <ads_state.h>
#include <client_utils.h>
#include <algorithm>
#include <config_components.h>
#include <boost/algorithm/string.hpp>

class anode_manager
{

  public:
    enum recommendation_strat { round_robin };

    anode_manager( ) {}; 
                   //home nex   remote nex
    anode_manager( std::string, std::string, std::vector<host_entry>, 
                   std::optional<std::string>  ); 

    anode_manager& operator= ( anode_manager&& );
 
    std::vector<zmq::multipart_t> recv_all();

    bool is_fully_inited( ) const;
    void add_anodes( std::vector<host_entry> );

    template <typename T>
    void add_anode(T remote_anode )
    {
      //HACK from home nexus
      bool is_local = (_remote_anodes.size() == 0);
      if( !_should_skip_node( remote_anode ) )
      {
        if( _should_spawn_bridge( remote_anode ) ) 
          _remote_anodes.push_back(anode( _job_id, _zctx, remote_anode, _bridge_parms, is_local));
        else
          _remote_anodes.push_back(anode( _job_id, _zctx, remote_anode, {}, is_local));
      }
      else std::cout << "Skipping node : " << remote_anode << std::endl;
    }


    void remove_anode( std::string );
    void remove_anodes( std::vector<std::string>);

    int post_init( ulong, std::string, const std::list<std::string>& );

    int make_claim( const std::string&, const std::string&, 
                    const std::map<std::string, std::string>& );
 
    int send_data ( ulong, zmq::multipart_t&& );
   
    std::string get_jobId() { return _job_id; }
    zmq::context_t get_zcontext() { return std::move( _zctx ); }
    std::vector<anode> get_node_list(){ return std::move( _remote_anodes ); }
    ulong get_last_nex() { return _last_nex; }


  private:

    bool  _node_exists( ulong );

    template<typename T>
    bool _should_skip_node(const T& anode_desc)
    {
      bool hn_exists = _hnode_exists( anode_desc );
      if constexpr ( !std::is_same_v<std::string, T> )
      {
        auto n_mode = reverse_map_find(g_node_mode_map, 
                                       anode_desc.get_mode() );
        return ( n_mode == node_mode::ADS_ONLY ) || (hn_exists);  
      }
      else return hn_exists;

    }

    template<typename T>
    bool _should_spawn_bridge( const T& anode_desc)
    {
      if constexpr ( !std::is_same_v<std::string, T> )
      {
        auto n_mode = reverse_map_find(g_node_mode_map, 
                                       anode_desc.get_mode() );
        return ( n_mode == node_mode::ADS_ACCEL );  
      }
      else return false; //only happens with root accelerator agent
    }

    template<typename T>
    bool _hnode_exists(const T& anode_desc)
    {
      std::string node_addr;
      if constexpr ( std::is_same_v<std::string, T> )
        node_addr  = anode_desc;
      else
        node_addr  = anode_desc.get_addr();
 
      std::string node_name  = get_hostname();
      auto node_vec = split_str<'.'>(  node_addr );

      if( node_vec.size() > 0 )
      {
        std::cout << "comparing " << node_vec[0] 
                  << " == " << node_name << std::endl;
        return (node_vec[0] == node_name);
      }
      else std::cout << "Could not find address for " << node_name << std::endl;
      

      return false;     
    }

    std::optional<ulong> 
    _recommend_node( recommendation_strat, 
                    std::string, std::string, 
                    const std::map<std::string, std::string>& );      

    ulong _recommend_accel(std::string, std::map<std::string, std::string> );


    std::string                _job_id;
    std::optional<std::string> _bridge_parms;
    zmq::context_t             _zctx;
    std::vector<anode>         _remote_anodes;
    //////////////////////////////////////////
    //////////////////HACK////////////////////
    //////////////////////////////////////////
    ulong               _last_nex;   
};

