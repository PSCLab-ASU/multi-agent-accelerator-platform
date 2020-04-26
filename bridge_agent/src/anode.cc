#include <anode.h>
#include <nexus_utils.h>

anode::anode(std::string jobId, zmq::context_t& zctx, std::string nex_addr )
: _bActive( false ), _job_id(jobId), _nexus_address( nex_addr ),
  _zsock( zctx, ZMQ_DEALER ) 
{
  _bActive     = false;
  _node_exists = false;
  if( zmq_ping( ZPING_TYPE::NONE, nex_addr ) ) 
  {
    std::string id = "anode-" + generate_random_str();
    _zsock.setsockopt(ZMQ_IDENTITY, id.c_str(), id.length());
    _zsock.connect( nex_addr );
    _node_exists = true;
    _pre_init();
  }

}

anode::anode( anode&& rhs)
: _bActive( rhs.isActive() ),   _nexus_address( rhs.get_address() ),
  _zsock( rhs.move_zsock() ),   _tx_id( rhs.get_tx_id() ), _rx_id( rhs.get_rx_id() ),
  _nxcache(rhs.move_nxcache()), _claim_history( rhs.move_claim_history() ) 
{}

int anode::_pre_init()
{
  auto method     = nexus_utils::nexus_ctrl::rem_jinit;
  std::string key = generate_random_str();

  auto zb = nexus_utils::start_request_message( method, key );
  zb.add_arbitrary_data( _job_id );
  zb.finalize();

  send( zb.get_zmsg() );

  return 0;
}

int anode::request_manifest()
{
  auto method     = nexus_utils::nexus_ctrl::rem_manifest;
  std::string key = generate_random_str();

  auto zb = nexus_utils::start_request_message( method, key );
  zb.finalize();

  send( zb.get_zmsg() );

  return 0;
}

void anode::set_nxcache_entries( const std::list<std::string>& )
{
  std::cout << "Entering "<< __func__ << std::endl;
}

int anode::make_claim( const std::string& key,  const std::string& claim, const std::map<std::string, std::string>& clovr)
{
  std::cout << "Entering "<< __func__ << std::endl;
  auto method     = nexus_utils::nexus_ctrl::rem_claim;
  auto zb = nexus_utils::start_request_message( method, key );
  zb.add_arbitrary_data( claim );
  zb.add_arbitrary_data( std::string("") );
  zb.finalize();
  //send claim  
  send( zb.get_zmsg() );

  return 0;
}

int anode::send( zmq::multipart_t& msg )
{
  msg.send( _zsock );

  return 0;
}

std::optional<zmq::multipart_t> anode::try_recv( )
{
  zmq::multipart_t mp;
  std::optional<zmq::multipart_t> omsg;

  mp.recv( _zsock, ZMQ_NOBLOCK);
   
  omsg = std::move( mp );
  
  return std::move(omsg);
}
