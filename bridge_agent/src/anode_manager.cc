#include <anode_manager.h>
#include <type_traits>
//#include <boost/range/algorithm/for_each.hpp>
#include <boost/range/algorithm.hpp>
#include <boost/algorithm/string.hpp>

#ifndef ANODEMAN
#define ANODEMAN

anode_manager::anode_manager( std::string jobId, std::string hnex, 
                              std::vector<host_entry> rnex_desc,
                              std::optional<std::string> bridge_parms)
: _zctx( zmq::context_t() ) , _remote_anodes()
{
  _bridge_parms = bridge_parms;
  //pass in jobId
  _job_id = jobId;
  //adding the home node
  add_anode( hnex );
  //add the remote nodes
  add_anodes( rnex_desc );
  
}

anode_manager& anode_manager::operator= ( anode_manager&& rhs)
{
  this->_job_id        = std::move( rhs.get_jobId()     );
  this->_zctx          = std::move( rhs.get_zcontext()  );
  this->_remote_anodes = std::move( rhs.get_node_list() );
  this->_last_nex      = std::move( rhs.get_last_nex()  );
  return *this;
}

bool anode_manager::is_fully_inited() const
{
  return std::all_of( _remote_anodes.begin(), _remote_anodes.end(), 
                      []( const anode& an ) {return an.isActive(); } );
}

//
bool anode_manager::_node_exists( ulong nodeIdx )
{
  return (_remote_anodes.size() > nodeIdx );
}

int anode_manager::send_data ( ulong nodeIdx, zmq::multipart_t&& msg)
{
  auto& an = _remote_anodes.at(nodeIdx);
  an.send( msg );

  return 0;
}

//used to recieve initialization from 
int anode_manager::post_init( ulong nodeIdx, std::string nexId, const std::list<std::string>& clines )
{
  std::cout << "  entering " << __func__ << std::endl;

  if( _node_exists( nodeIdx ) )
  {
    auto& an = _remote_anodes.at(nodeIdx);

    std::lock_guard<anode> lk(an);
    an.set_rx_id( nexId );
    an.set_nxcache_entries( clines );
    an.activate();   
  } else std::cout<<"Couldn't find node : " << nodeIdx << std::endl;

  return 0;
}

ulong anode_manager::_recommend_accel( std::string claim_req, 
                                       std::map<std::string, std::string> claim_ovr )
{
  std::cout << "  entering " << __func__ << std::endl;
  return 0;
}

std::vector<zmq::multipart_t> anode_manager::recv_all()
{

  std::vector<zmq::multipart_t> out;

  for( auto& rnode : _remote_anodes )
  {
    auto msg = rnode.try_recv();
    out.push_back( std::move(msg.value() ) );
  }
  return out;

}

void anode_manager::add_anodes( std::vector<host_entry> remote_anodes)
{
  for( auto anode : remote_anodes )
    add_anode( anode );
}

void anode_manager::remove_anode( std::string remote_address )
{

}

void anode_manager::remove_anodes( std::vector<std::string> rnexs )
{

}

//template <recommendation_strat strat> TBD
std::optional<ulong>
anode_manager::_recommend_node( recommendation_strat strat, 
                                std::string key, std::string claim_request, 
                                const std::map<std::string, std::string>& claim_ovr )      
{
  std::vector< std::pair<ulong, ulong> > node_congestion_table;
  //1) get a list of candidate nodes

  auto num_nodes = _remote_anodes.size();

  for(auto i : std::ranges::views::iota((size_t) 0, num_nodes))
  {
    anode& an = _remote_anodes[i];
    bool supported = an.can_completely_support( claim_request );

    if( supported ) 
      node_congestion_table.emplace_back(i, an.get_outstanding_msg_cnt() ); 

  }

  boost::sort( node_congestion_table, [](auto lhs, auto rhs){ return lhs.second < rhs.second; });
  boost::for_each( node_congestion_table, 
   [](auto item) {std::cout << "Recommend : "<< item.first<< ", " << item.second<< std::endl; });
  if( node_congestion_table.empty() )
  {
    std::cout << "No Support for : " << claim_request << std::endl;
    return {};
  }
  else return node_congestion_table[0].first;
}


int anode_manager::make_claim( const std::string& AccId, 
                               const std::string& claim_req, 
                               const std::map<std::string, std::string>& claim_ovr )
{
  //get all candidate nodes
  auto nodeId = _recommend_node( recommendation_strat::round_robin, 
                                 AccId, claim_req, claim_ovr);   

  if( nodeId && nodeId.value() < _remote_anodes.size() )
  {
    std::cout << "Recommending : " << nodeId.value() << " for "<< claim_req << std::endl;
    _remote_anodes[nodeId.value()].make_claim( AccId, claim_req, claim_ovr);
    return 0;
  }
  else  
  {
    std::cout << "Recommendation out of bounds or cannot be fullfilled..." << std::endl;
    return -1;
  }
}
                 


#endif

