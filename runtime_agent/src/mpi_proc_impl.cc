#include <mpi_proc_impl.h>

mpi_proc_impl::mpi_proc_impl(std::shared_ptr<std::mutex>& mix_mu,
                             std::shared_ptr<std::condition_variable> & mix_cv,
                             std::unique_lock<std::mutex>& mix_lk,
                             bool async)
: mpi_pv_interface( async ), _mix_lk( mix_lk ),
  _mix_mutex( mix_mu ), _mix_cv( mix_cv )  
{
  auto bind_action = [&](auto func)
  {
    return std::bind(func, this, std::placeholders::_1, std::placeholders::_2);
  };
  
  _action_registry[mpi_proc_utils::mpi_proc_mid::response] = bind_action( &mpi_proc_impl::_response_action );
  _action_registry[mpi_proc_utils::mpi_proc_mid::passthrough] = bind_action( &mpi_proc_impl::_default_action );
  _action_registry[mpi_proc_utils::mpi_proc_mid::get_groupid] = bind_action( &mpi_proc_impl::_get_groupid );
   
}

std::optional< mpi_proc_utils::mpi_proc_pkt_viewer >
mpi_proc_impl::_get_next_message_info( )
{
  int flag, msg_sz;
  MPI_Status status;
  MPI_Request request;

  MPI_Iprobe(MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &flag, &status);

  if( flag ) 
  {
    std::cout << "MPI Request Pending..." << std::endl;
    MPI_Get_count(&status, MPI_BYTE, &msg_sz);
    void * raw_ptr = malloc(msg_sz);
    MPI_Recv(raw_ptr, msg_sz, MPI_BYTE, MPI_ANY_SOURCE, 
             MPI_ANY_TAG, MPI_COMM_WORLD, &status);

    auto data_ptr = mpi_data_t( raw_ptr, mpi_pack_deleter );

    auto mppv = mpi_proc_utils::mpi_proc_pkt_viewer( std::move(data_ptr), 
                                                     (status.MPI_TAG != _system_tag),
                                                     msg_sz );

    mppv.set_src_and_tag( status.MPI_SOURCE, status.MPI_TAG );
    return std::move(mppv);

  }

  else return {};
}

std::optional<mpi_proc_utils::mpi_proc_pkt_viewer > mpi_proc_impl::try_pop_caq_itm( )
{
  std::optional<mpi_proc_utils::mpi_proc_pkt_viewer> oret;
  if( _cross_action_q.empty() ) return {};
  else{
    oret = std::move(_cross_action_q.front()); 
    _cross_action_q.pop();
    return std::move(oret);   
  }
}

mpi_proc_impl::action_func mpi_proc_impl::action_ctrl( mpi_proc_utils::mpi_proc_mid method )
{
  //TBD: default is a mix pass-through function
  if( _action_registry.find( method ) != _action_registry.end() )
  {
    return _action_registry.at(method);
  }
  else
  {
    std::cout << "Forwarding data to mix" << std::endl;
    return _action_registry.at( mpi_proc_utils::mpi_proc_mid::passthrough );
  }

}

void mpi_proc_impl::init_thread_components()
{
  std::cout << "init_thread_components... " << std::endl;
  while( !_atomic_mpi_initialized.load() );
}

void mpi_proc_impl::system_update()
{
  //step 1: get next message
  mpi_return mret;
  auto opt_hdr_data = _get_next_message_info( );
  std::optional<mpi_proc_utils::mpi_proc_pkt_viewer> mix_request;
 
  //check if there is a message 
  if( opt_hdr_data ) 
  {
    //check if the message is for the system or for the client
    if( opt_hdr_data->get_msg_tag() == _system_tag )
    {
      std::cout << "Recieved System msg " << std::endl;
      //if there is a message in the queue process it
      auto method = opt_hdr_data->get_method();

      //call function back action
      mret = action_ctrl( method )( opt_hdr_data.value(), mix_request );

    } //end of system_msg
    else
    {
      //call function back action
      auto key = mpi_proc_utils::mpi_proc_mid::response;
      auto mret = action_ctrl( key )( opt_hdr_data.value(), mix_request );
      std::cout << "Native packet Recv..." << std::endl;
    }

    if( mret && mix_request )
    {
      //if successful and there is a mix request
      //forward request to mix queue
      _cross_action_q.emplace( std::move( mix_request.value() ) );
    }
  } //optional message 

}

int mpi_proc_impl::_get_tag_upper_bound()
{
  void *v;
  int flag;
  int vval;

  MPI_Comm_get_attr( MPI_COMM_WORLD, MPI_TAG_UB, &v, &flag );
  if( !flag ) 
  {
    std::cout << "Could not get upper tag bound" << std::endl;
    vval = *(int*)v;
  }
  else vval = *(int*)v;
  
  return vval;
}

int mpi_proc_impl::get_app_tag_ub()
{
  return _get_tag_upper_bound() - 2;
}


bool mpi_proc_impl::is_processor_rank( const ulong& rank) const
{
  return ( (rank == 0) || (rank < _world_size) );
}

const ulong& mpi_proc_impl::get_global_rank() const
{
  return _global_rank;
}

const ulong& mpi_proc_impl::get_local_rank() const
{


  return _local_rank;
}


const ulong& mpi_proc_impl::get_world_size() const
{
  return _world_size;
}

std::map<std::string, std::string> mpi_proc_impl::_get_minfo_keyval( MPI_Info mi_handle)
{
  //TBD
  return {};
}

std::map<std::string, std::string> mpi_proc_impl::get_minfo_keyval( MPI_Info mi_handle)
{
  return _get_minfo_keyval( mi_handle );
}

mpi_return mpi_proc_impl::operator()(std::integral_constant<api_tags, mpi_init>, int * argc, char *** argv, metadata& md)
{
  int provided=0;
  std::lock_guard<std::mutex> guard( *_mix_mutex );
  std::cout << "entering mpi_proc_impl::mpi_init" << std::endl;
  _world_size = 1;
  _global_rank = 0;

  //error can't pass the pointers in seg fault results
  MPI_Init_thread( argc, argv, MPI_THREAD_MULTIPLE, &provided );
  if( provided != MPI_THREAD_MULTIPLE )
  {
    std::cout << "Does not support Multithreaded MPI! " << std::endl;
  }
  //setting system tag
  _system_tag = get_app_tag_ub() + 1;
  //check if the MPI subsystem has been inititalized
  MPI_Initialized( &_is_mpi_initialized );
  if( _is_mpi_initialized ) 
  {
    std::cout << "MPI is initialized : " << provided << std::endl;
    int wrld_sz, cur_rank;
    MPI_Comm_size(MPI_COMM_WORLD, &wrld_sz);
    MPI_Comm_rank(MPI_COMM_WORLD, &cur_rank);
    _world_size = wrld_sz;
    _global_rank = cur_rank;

    MPI_Comm shmcomm;
    MPI_Comm_split_type(MPI_COMM_WORLD, MPI_COMM_TYPE_SHARED, 0,
                        MPI_INFO_NULL, &shmcomm);

    int l_rank;
    MPI_Comm_rank(shmcomm, &l_rank);
    _local_rank = l_rank;
    //deallocating communicator
    MPI_Comm_free(&shmcomm);
    //set initial rank value for child ranks
    zrglobals.init_rank(_world_size);
  }
 
  //hold MPI thread until the MPI runtime is intialized
  _atomic_mpi_initialized.store( true );
  return mpi_return{};
}
mpi_return mpi_proc_impl::operator()(std::integral_constant<api_tags, mpi_finalize>, metadata& md)
{




  int ret = MPI_Finalize();

  return mpi_return{};
}

mpi_return mpi_proc_impl::operator()(std::integral_constant<api_tags, mpi_send>, 
                                     const void * buf, int cnt, uint datatype, int dest, int tag, int comm, metadata& md )
{
  std::lock_guard<std::mutex> guard( *_mix_mutex );
  std::cout << "entering ... proc_mpi_send" << std::endl;
  return mpi_return{};
}

mpi_return mpi_proc_impl::operator()(std::integral_constant<api_tags, mpi_recv>, 
                                     void * buf, int cnt, uint datatype, int source, int tag, int comm, MPI_Status * status, metadata& md)
{
  std::lock_guard<std::mutex> guard( *_mix_mutex );
  std::cout << "entering ... proc_mpi_recv" << std::endl;
  return mpi_return{};
}

mpi_return mpi_proc_impl::operator()(std::integral_constant<api_tags, mpi_claim>, 
                                   const char * falias,
                                   MPI_ComputeObj::callback cb,
                                   int minfo, ulong * new_rank, metadata& md)
{
  std::lock_guard<std::mutex> guard( *_mix_mutex );
  std::cout << "entering ... proc_mpi_claim" << std::endl;
  return mpi_return{};
}

mpi_return mpi_proc_impl::operator()(std::integral_constant<api_tags, mpi_test>, metadata& md)
{
  std::lock_guard<std::mutex> guard( *_mix_mutex );
  return mpi_return{};
}

mpi_return mpi_proc_impl::operator()(std::integral_constant<api_tags, mpi_alloc_mem>, int, int, void **, metadata&)
{
  std::lock_guard<std::mutex> guard( *_mix_mutex );
  return mpi_return{};
}

mpi_return mpi_proc_impl::operator()(std::integral_constant<api_tags, mpi_free_mem>, void **, metadata&)
{
  std::lock_guard<std::mutex> guard( *_mix_mutex );
  return mpi_return{};
}

mpi_return mpi_proc_impl::_default_action ( ACTION_REG_SIGNATURE )
{
  std::cout << "entering _default_action... " << std::endl;
  return mpi_return{};
}

mpi_return mpi_proc_impl::_response_action ( ACTION_REG_SIGNATURE )
{
  std::cout << "entering _response_action... " << std::endl;
  //section to forward to the mailbox 
  if( _ct_mailbox.first )
  {
    //check if the mailbox request with the current request
    //for requests that produce many sys call, keep header active
    std::cout << "checking mailbox..." << std::endl;
    if( input.get_proc_header() == _ct_mailbox.first.value() ) 
    {
      std::cout << "Matched header information to pending..." << std::endl;
      //fill the mailbox

      _ct_mailbox.second = std::move( input );
      //notify the waiting thread
      _mix_cv->notify_one();

    }
    else
    {
      //there is a messgae pending but it wasn't a match
      std::cout << "Current mesg doesn't matching pending msg" << std::endl;
    }
     
  }
  else
  {
    //no pending message but message comes in
    //a message came in that is NOT a system message, 
    //and this thread wasn't expecting any message
    std::cout << "(err) No pending message... " << std::endl;
  }

  return mpi_return{};
}

mpi_return mpi_proc_impl::_get_groupid( ACTION_REG_SIGNATURE )
{
  std::cout << "entering _get_groupid.... " << std::endl;
  std::cout << input.get_proc_header() << std::endl;

  MPI_Request request;
  int data_bytes = sizeof(int); 
  auto[hdr_bytes, hdr_vals] = mpi_proc_utils::form_header( {MPI_INT}, {1} );
  int total_bytes = hdr_bytes + data_bytes;
  std::cout << "hdr_bytes : " << hdr_bytes 
            << "data_bytes : " << data_bytes << std::endl;
  //////////////////////////////////////////////////////////////////////////
  int claim_rank = zrglobals.claim_rank();
  /////////////////////////////////////////////////////////////////////////

  mpi_proc_utils::mpi_proc_pkt_builder 
  mproc_builder ( mpi_proc_utils::mpi_proc_mid::response,
                  total_bytes, input.get_sys_tag() );

  mproc_builder.push_back_hdr( hdr_vals.first , hdr_vals.second ); 
  mproc_builder.push_back_data( &claim_rank );

  auto data = mproc_builder.finalize();
  //send data
  MPI_Isend(data.get(), mproc_builder.get_byte_size(), 
            MPI_BYTE, input.get_src_rank(), 
            input.get_msg_tag(), MPI_COMM_WORLD, &request);

  std::cout << "waiting for message... " << std::endl;

  return mpi_return{};
}

ulong mpi_proc_impl::request_new_rank( )
{
  MPI_Request request;
  int _id = rand();
  ulong rank_id;
  std::cout << "building group_id request " << std::endl;
  
  if( _global_rank ) 
  {
    mpi_proc_utils::mpi_proc_pkt_builder 
    mproc_builder ( mpi_proc_utils::mpi_proc_mid::get_groupid,
                    0, _id );
    auto data = mproc_builder.finalize();
    //send data
    MPI_Isend(data.get(), mproc_builder.get_byte_size(), 
              MPI_BYTE, _rank_zero, 
              _system_tag, MPI_COMM_WORLD, &request);

    std::cout << "waiting for message... " << std::endl;
    //wait for message (this unlocks the lock) 
    _wait_for_message( _id, _system_tag );
    std::cout << "mailbox filled... " << std::endl;
    //get data from mailbox
    auto& mail = _ct_mailbox.second; 
    rank_id    = mail->get_single_data<int>(0);
    _reset_mailbox();
    return rank_id;
    
  }
  else
  {
    std::cout << "This rank is the zero'th rank " << std::endl;
    return (ulong) zrglobals.claim_rank();
  }
  return 0;
}

void mpi_proc_impl::_reset_mailbox()
{
  _ct_mailbox.first.reset();
  _ct_mailbox.second.reset();
}

template<typename ...Ts>  
void mpi_proc_impl::_wait_for_message(Ts... parms)
{
  //fill mailbox
  _ct_mailbox.first = mpi_proc_utils::proc_header{ parms... };
  //wait for global Id
  _mix_cv->wait( _mix_lk );  

}


template void mpi_proc_impl::_wait_for_message<int, int>( int, int );
