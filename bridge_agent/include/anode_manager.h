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

class anode_manager
{

  public:
    enum recommendation_strat { round_robin };

    anode_manager( ) {}; 
                   //home nex   remote nex
    anode_manager( std::string, std::string, std::vector<std::string> ); 

    anode_manager& operator= ( anode_manager&& );
 
    std::vector<zmq::multipart_t> recv_all();

    bool is_fully_inited( ) const;
    void add_anodes( std::vector<std::string> );
    void add_anode( std::string );
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

    std::optional<ulong> 
    _recommend_node( recommendation_strat, 
                    std::string, std::string, 
                    const std::map<std::string, std::string>& );      

    ulong _recommend_accel(std::string, std::map<std::string, std::string> );


    std::string         _job_id;
    zmq::context_t      _zctx;
    std::vector<anode>  _remote_anodes;
    //////////////////////////////////////////
    //////////////////HACK////////////////////
    //////////////////////////////////////////
    ulong               _last_nex;   
};

