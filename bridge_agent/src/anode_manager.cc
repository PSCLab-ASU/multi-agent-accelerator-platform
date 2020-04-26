#include <anode_manager.h>
#include <type_traits>
#include <boost/range/algorithm/for_each.hpp>
#ifndef ANODEMAN
#define ANODEMAN

anode_manager::anode_manager( std::string jobId, std::string hnex, std::vector<std::string> rnex)
: _zctx( zmq::context_t() ) , _remote_anodes()
{
  //pass in jobId
  _job_id = jobId;
  //adding the home node
  add_anode( hnex );
  //add the remote nodes
  add_anodes( rnex );
  
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

void anode_manager::add_anodes( std::vector<std::string> remote_anodes)
{
  for( auto anode : remote_anodes )
    add_anode( anode );
}

void anode_manager::add_anode( std::string nex )
{ 
  anode an = anode( _job_id, _zctx, nex);
  _remote_anodes.push_back( std::move(an) );

}

void anode_manager::remove_anode( std::string remote_address )
{

}

void anode_manager::remove_anodes( std::vector<std::string> rnexs )
{

}

int anode_manager::make_claim( const std::string& AccId, 
                               const std::string& claim_req, 
                               const std::map<std::string, std::string>& claim_ovr )
{
  ulong nsize  = _remote_anodes.size();
  ulong nodeId = (_last_nex++ % nsize); //zero is home nexa

  //nodeId = _recommend_accel( claim_req, claim_ovr );
  if( nodeId < _remote_anodes.size() )
  {
    std::cout << "Recommending : " << nodeId << " for "<< claim_req << std::endl;
    _remote_anodes[nodeId].make_claim( AccId, claim_req, claim_ovr);
  }
  else  std::cout << "Recommendation out of bounds..." << std::endl;

  return 0;
}
                 


#endif

