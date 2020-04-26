#include <ranges>
#include <mpi_accel_impl.h>
#include <boost/range/algorithm.hpp>

mpi_accel_impl::mpi_accel_impl(std::string serv_addr, std::string jobId, 
                               std::string host_file, std::string repos,
                               std::shared_ptr<std::mutex>& mix_mu,
                               std::shared_ptr<std::condition_variable> & mix_cv,
                               std::unique_lock<std::mutex>& mix_lk, bool async )
: _mix_lk( mix_lk), _mix_mutex( mix_mu ), _mix_cv( mix_cv ), _zsock( _zctx, ZMQ_DEALER)
{

  using ctrl = client_utils::client_ctrl;

  _service_address = serv_addr;
  _jobId           = jobId;
  _host_file       = host_file;
  _repos           = repos;
  _is_service_up   = _check_service();
 
  if( !_host_file.empty() )
    import_configfile( _host_file );
  
  auto bind_action = [&](auto func) 
  {
    return std::bind(func, this, std::placeholders::_1, std::placeholders::_2);
  };

  _action_registry[ctrl::def]   = bind_action( &mpi_accel_impl::_default_action ); 
  _action_registry[ctrl::resp]  = bind_action( &mpi_accel_impl::_response_action ); 
  _action_registry[ctrl::claim] = bind_action( &mpi_accel_impl::_update_claim ); 
  _action_registry[ctrl::recv]  = bind_action( &mpi_accel_impl::_recv ); 

  std::string dealer_id = "ads-" + generate_random_str(); 
  _zsock.setsockopt(ZMQ_IDENTITY, dealer_id.c_str(), dealer_id.length());
  
  _zsock.connect(_service_address);

}

std::optional<zmq::multipart_t>
  mpi_accel_impl::_get_next_message_info( )
{
  zmq::multipart_t zmsg; 
  zmsg.recv( _zsock, ZMQ_NOBLOCK );
  if( !zmsg.empty() ) 
    return std::move( zmsg );
  else return {};

}

mpi_accel_impl::action_func mpi_accel_impl::action_ctrl( std::string method_str )
{
  auto method = client_utils::reverse_lookup( method_str );
  if( _action_registry.find( method ) != _action_registry.end() ) 
    return _action_registry.at(method);
  else 
    return _action_registry.at(client_utils::client_ctrl::def);
}

void mpi_accel_impl::system_update()
{
  auto input = _get_next_message_info();
  std::optional<zmq::multipart_t>  mix_request;

  if( input )
  {
    zmsg_viewer<std::string, std::string, std::string> zmsgv ( input.value() );

    auto[key, smethod, pmethod] = zmsgv.get_header();

    auto aret = action_ctrl( pmethod )( zmsgv, mix_request ); 
    if( aret && mix_request )
    {
      //forward to mix
      _cross_action_q.emplace( std::move( mix_request.value() ) );
    }
  } 

}

void mpi_accel_impl::_process_recv_message(mpi_recv_pmsg& recv,  MPI_ComputeObj* cobj )
{
  auto& data = recv.get_data();
  ulong num_args =0;
  if( data ) 
  {
    //std::cout << "processing recv_data : " << data.value() << std::endl;
    auto& zmsg = data.value();
    
    cobj->num_args = num_args = *(zmsg.at(5).data<ulong>()); 
    
    for(auto i : std::ranges::views::iota(0, num_args) ) 
    {
       cobj->types[i]   = *(zmsg.at(9+i*6).data<ulong>());
       cobj->lengths[i] = *(zmsg.at(7+i*6).data<ulong>());
       cobj->data[i]    = zmsg.at(11+i*6).data();
       //std::cout << "Output " << i << " recieved " << cobj->lengths[i] << std::endl;
       //for(auto j : std::ranges::views::iota(0, cobj->lengths[i] ) )
       //  std::cout << "data[" << j << "] = " << ((float *)cobj->data[i])[j] << std::endl;
    }
 
  
 
  }
  else std::cout << "No data in recv packet ERROR" << std::endl;


}

void mpi_accel_impl::shutdown_runtime_agent()
{
  std::cout << "entering ... " << __func__ << std::endl;
  std::string key;
  auto zBuilder = _start_message(accel_utils::accel_ctrl::shutdown, key);
  zBuilder.finalize();
  //send adn recieve message
  _send_zmsg( zBuilder );

}

std::optional< zmq::multipart_t  >
  mpi_accel_impl::try_pop_caq_itm()
{
  if( !_cross_action_q.empty() )
  {
    zmq::multipart_t zmsg = std::move( _cross_action_q.front() );
    _cross_action_q.pop();

    return std::move( zmsg );
  }
  else return {};
}

mpi_return mpi_accel_impl::accel_init( const ulong& rank, const ulong& world_sz )
{
  std::cout << "entering ... " << __func__ << std::endl;
  std::string key;
  _this_rank = rank;

  auto zBuilder = _start_message(accel_utils::accel_ctrl::rinit, key);
  zBuilder.add_arbitrary_data( rank );
  zBuilder.add_arbitrary_data( world_sz );
  zBuilder.finalize();
  ///////////////////////////////////////////////////////////
  _ct_mailbox->first = key;
  //send adn recieve message
  _send_zmsg( zBuilder );
  //wait until the request is full filled
  _mix_cv->wait( _mix_lk );

  return mpi_return{};
}

mpi_return mpi_accel_impl::finalize()
{
  std::cout << "entering : " << __func__ << std::endl;
  std::string key;
  auto zmsgb = _start_message( accel_utils::accel_ctrl::finalize, key);
  zmsgb.add_arbitrary_data( _this_rank );
  zmsgb.finalize();
  /////////////////////////////////////////////////////////////////////
  _ct_mailbox->first = key;
  //send adn recieve message
  _send_zmsg( zmsgb );
  //wait until the request is full filled
  _mix_cv->wait( _mix_lk );
  printf("--client %i finalized \n", _this_rank);

  return {};
}

mpi_return mpi_accel_impl::accel_claim( std::string falias, 
                                        std::map<std::string, std::string> meta, std::string& claimId )
{
  std::string key;
  auto zmsgb = _start_message( accel_utils::accel_ctrl::claim, key);
  //this will be the claim number as seen by the library
  claimId = key;
  //add the alias
  zmsgb.add_arbitrary_data( falias );
  //reorg the map
  std::list<std::string> keyval;
  for(auto entry : meta )
    keyval.push_back( entry.first + "=" + entry.second);
  //add the overrides
  if( !keyval.empty() )
    zmsgb.add_sections( keyval );
  //finalize message
  zmsgb.finalize();
  ///////////////////////////////////////////////////////////
  _claim_registry.emplace_back( key, falias );
  _claim_registry.back().set_metadata( meta );
  ///////////////////////////////////////////////////////////
  _ct_mailbox->first = key;
  //send adn recieve message
  _send_zmsg( zmsgb );
  //wait until the request is full filled
  _mix_cv->wait( _mix_lk );
  ///////////////////////////////////////////////////////////
  zmsg_viewer<std::string, std::string, std::string> 
    zmsgv ( _ct_mailbox->second );
  std::optional< zmq::multipart_t >  temp;

  auto[temp1, smethod, temp2] = zmsgv.get_header();

  auto aret = action_ctrl( smethod )( zmsgv, temp ); 
  ///////////////////////////////////////////////////////////
  ///////////////////////////////////////////////////////////
  //reset mailbox
  _reset_mailbox();

  return mpi_return{};
}
 
mpi_return mpi_accel_impl::accel_send(int dst, std::string claimId, const MPI_ComputeObj* cobj, int cnt, int tag)
{
  std::cout << "entering ... " << __func__ << std::endl;
  std::string key;
  int type_size=0;
  ushort nargs = cobj->get_nargs();
  uint   *  types;
  size_t *  lens;
  void  **  data; 

  if( cnt == 1 )
  {
    ushort nargs = cobj->get_nargs();
    types = cobj->get_types();
    lens  = cobj->get_lengths();
    data  = cobj->get_data(); 

    auto zmsgb = _start_message( accel_utils::accel_ctrl::send, key);
    //building message
    zmsgb.add_arbitrary_data(claimId);
    //add number of args
    zmsgb.add_arbitrary_data((ulong) nargs);

    for(int i =0; i < nargs; i++)
    {   
      auto[b_signed, b_type] = g_mpi_type_conv.at(types[i]);
      MPI_Type_size(types[i], &type_size);
      zmsgb.add_memblk(b_signed, b_type, type_size, 
                       data[i], lens[i] );
    }

    zmsgb.finalize();
    /////////////////////////////////////////////////////////
    ///////////adding pending message to registry////////////
    _pmsg_registry.add_send_pmsg( key, dst, tag, 
                                  zmsgb.get_zmsg().clone() );
    /////////////////////////////////////////////////////////
    /////////////////////////////////////////////////////////
    //sending message
    _send_zmsg( zmsgb );


  }else std::cout << "cnt is not equal one " << std::endl;
  

  return mpi_return{};
} 

mpi_return mpi_accel_impl::accel_send(int dst, std::string claimId, const void* buf, int cnt, int dt, int tag )
{
  std::cout << "2) entering ... " << __func__ << std::endl;

  return mpi_return{};
}
 
mpi_return mpi_accel_impl::accel_recv(MPI_ComputeObj* cobj, int src, int tag )
{

  std::cout << "Getting into  ... " << __func__ << std::endl;

  auto& pmsgr = _pmsg_registry;
  //first check if the data is already here
  //this will return the lowest seq number
  //machine the src and tag
  //keep in mind this is destructive!
  printf("--client: Looking up message rank=%i, tag=%i\n", src, tag );
  auto rpmsg = pmsgr.edit_recv_pmsg( std::make_pair(src, tag) );
  if( rpmsg && rpmsg->get().is_active() )
  {
    //found a recv message with the lowest sequence number 
    _process_recv_message(rpmsg->get(), cobj ); 
  }
  else
  {
    if( !rpmsg ) 
    {
      std::cout << "Could not find recv pending message" << std::endl;
      return {};
    }

    _ct_mailbox->first = rpmsg->get().get_key();
    //wait until the request is full filled
    _mix_cv->wait( _mix_lk );
    ////////////////////////////////////////////////////////
    //grab the mailbox value
    std::optional<zmq::multipart_t > zmq_o;
    auto zmsgv = zmsg_viewer<std::string, std::string, std::string>( _ct_mailbox->second );
    /////////////////////////////////////////////////////////////////
    //this function will add the recv message to the pending registry
    _recv( zmsgv, zmq_o ); //This function also removes the send counter part
    /////////////////////////////////////////////////////////////////
    rpmsg = pmsgr.edit_recv_pmsg( std::make_pair(src, tag) );
    if( rpmsg && rpmsg->get().is_active() ) 
    {
      _process_recv_message( rpmsg->get(), cobj ); //this process the message and send it back to the app
    }
    else
      std::cout << "Error no recv message was found" << std::endl;
    ///////////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////
    //reset mailbox
    _reset_mailbox();
  }

  return mpi_return{};
}
 
mpi_return mpi_accel_impl::accel_recv(void* buf, int cnt, int dt, int src, int tag )
{
  std::cout << " entering default_action " << std::endl;
  return mpi_return{};
}

mpi_return mpi_accel_impl::_default_action( ACTION_ACCEL_SIG )
{
  std::cout << " entering default_action " << std::endl;
  return {};
}

mpi_return mpi_accel_impl::_recv( ACTION_ACCEL_SIG )
{
  std::cout << " entering _recv " << std::endl;
  auto& pmsgr = _pmsg_registry;
  //step 1: get the key
  auto[key, smethod, pmethod] = input.get_header();
  //step 2: lookup the key in the pending message
  auto spmsg = pmsgr.move_send_pmsg( key );
  //step 3: return the dst, and tag of orignal message
  auto[ rank, tag ] = spmsg->get_rank_tag();
  //step 4: get the empty recv message from pending
  auto rpmsg = pmsgr.edit_recv_pmsg( std::make_pair( rank, tag), key ); 
  //step 5: add recv pending message to registry
  if( rpmsg )
  {
    rpmsg->get().move_data( std::move(input.get_zmsg()) ); //DOES ANOTHER COPY :(
    rpmsg->get().activate();
  }
  else std::cout << "Could not find pmsg recv with rank : "<< 
         rank << ", tag : "<< tag << ", key : " << key << std::endl;

  return {};
}

mpi_return mpi_accel_impl::_response_action( ACTION_ACCEL_SIG )
{
  std::cout << " entering response_action " << std::endl;
  mpi_return aret;

  auto[key, smethod, pmethod] = input.get_header();
  if( _ct_mailbox->first == key )
  {
    _ct_mailbox->second = std::move( input.get_zmsg() );
    _mix_cv->notify_one();
  }
  else
  {
    aret = action_ctrl( smethod )( input, output ); 
  }

  return aret;
}

mpi_return mpi_accel_impl::_update_claim( ACTION_ACCEL_SIG )
{
  std::cout << "updating claim state...." << std::endl;
  auto [claimId,t1,t2] = input.get_header();
  bool bActive   = input.get_section<bool>(0).front(); 
  
  auto claim = boost::find( _claim_registry, claimId);

  if( claim != _claim_registry.end() )
  {
    if( bActive ) claim->activate();
    else claim->deactivate();

  }
  else std::cout << "Could not find claim details " << std::endl;

  return {};
}

bool mpi_accel_impl::_check_service( )
{
  if( _service_address.empty() )
    return false;
  else
    return zmq_ping( ZPING_TYPE::NONE, _service_address );
}

bool mpi_accel_impl::import_configfile( std::string cfg_path)
{
  std::string file_content;
  configfile cfg;
  if( !cfg_path.empty() )
  {
    read_complete_file( cfg_path, file_content );
    if( !file_content.empty() )
    {
      cfg.import( file_content );

      _config_file = std::move(cfg);
    }
  }
 
  return false;
}

