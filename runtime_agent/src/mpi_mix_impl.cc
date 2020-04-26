#include <mpi_mix_impl.h>

mpi_mix_impl::mpi_mix_impl(std::string service_addr, std::string jobId, 
                           std::string hf, std::string repos, bool async)
: mpi_pv_interface( async )
{
  
  _cv = std::make_shared<std::condition_variable>();
  //pointer to legacy mpi processor interface
  _mpi_proc_impl  = std::make_unique<mpi_proc_impl>( get_shared_mutex(), _cv, get_lock() );
  //pointer to accelerator servvices interface
  _mpi_accel_impl = std::make_unique<mpi_accel_impl>(service_addr, jobId, hf, repos,
                                                     get_shared_mutex(), _cv, get_lock() );
  //check if service is running
  _is_service_up  = _mpi_accel_impl->check_service();

  //if async available kick off thread
  if( is_async() ) 
    own_thread( std::thread(&mpi_mix_impl::thread_main, this) );
}

void mpi_mix_impl::init_thread_components()
{
  _mpi_proc_impl->init_thread_components();
}

void mpi_mix_impl::system_update()
{
  //update internal proc state
  _mpi_proc_impl->system_update();
  //update internal accel state
  _mpi_accel_impl->system_update();
}

mpi_return mpi_mix_impl::operator()(std::integral_constant<api_tags, mpi_init> tag, int * argc, char *** argv, metadata& md )
{
  //initialize mpi_process
  _mpi_proc_impl->operator()(tag, argc, argv, md );

  {
    //this lock must come after since
    std::lock_guard<std::mutex> guard( *get_shared_mutex() );
    //get rank of current process
    ulong rank = _mpi_proc_impl->get_current_rank();
    ulong world_sz = _mpi_proc_impl->get_world_size();
    //initializing acceleration service
    std::cout << "is service up : " << _is_service_up << std::endl;
    if( _is_service_up ) _mpi_accel_impl->accel_init( rank, world_sz );
 
  }

  //wait for all clients to reach Finalize
  _mpi_proc_impl->all_rank_barrier();

  return mpi_return{};
}

mpi_return mpi_mix_impl::operator()(std::integral_constant<api_tags, mpi_finalize> tag, metadata& md )
{
  ulong rank = _mpi_proc_impl->get_current_rank();
  //wait for all clients to reach Finalize
  _mpi_accel_impl->finalize();
  //at this point the rutime agent indicated that there will be no
  //more messages
  printf("client %i: completed runtime shutdown procedure\n", rank);
  if( rank == 0)  _mpi_accel_impl->shutdown_runtime_agent();
  //now stop internal thread
  stop_driver();
  //join wait until all the thread have completed
  check_wait();
  //call proc finalize
  //call MPI finalize to shutdown MPI engine
  _mpi_accel_impl->close_sockets();
  //NO MPI CALLS PASSED THIS POINT
  std::cout << "client " << rank << " : finalizing MPI" << std::endl;
  _mpi_proc_impl->operator()(tag, md);
  //deallocate socket
  std::cout << "client " << rank << " : shutingdown" << std::endl;
  
  return mpi_return{};
}

mpi_return mpi_mix_impl::operator()(std::integral_constant<api_tags, mpi_send>, 
                                    const void * buf, int cnt, uint datatype, int dest, int tag, int comm, metadata& md )
{
  std::lock_guard<std::mutex> guard( *get_shared_mutex() );
  std::cout << "entering ... mix_mpi_send" << std::endl;
  //get claimID 
  //TODO CHECK IF CLAIM is active if not go to backup//////
  auto claimId = _claim_lookup.at( dest );
  int dst = _mpi_proc_impl->get_current_rank();

  if( datatype == MPIX_COMPOBJ )
  {
    _mpi_accel_impl->accel_send(dest, claimId, (const MPI_ComputeObj*) buf, cnt, tag );
  }
  else
  {
    _mpi_accel_impl->accel_send(dest, claimId, buf, cnt, datatype, tag );
  }

  return mpi_return{};
}

mpi_return mpi_mix_impl::operator()(std::integral_constant<api_tags, mpi_recv>, 
                                    void * buf, int cnt, uint datatype, int source, int tag, int comm, MPI_Status * status, metadata& md )
{
  std::lock_guard<std::mutex> guard( *get_shared_mutex() );
  std::cout << "entering ... mix_mpi_recv" << std::endl;
  if( datatype == MPIX_COMPOBJ )
  {
    std::cout << "Recieving COMPOBJ" << std::endl;
    _mpi_accel_impl->accel_recv(( MPI_ComputeObj*) buf, source, tag );
  }
  else
  {
    std::cout << "Recieving " << datatype << " array" << std::endl;
    _mpi_accel_impl->accel_recv(buf, cnt, datatype, source, tag );
  }
  return mpi_return{};
}

mpi_return mpi_mix_impl::operator()(std::integral_constant<api_tags, mpi_test>, metadata& md )
{
  std::lock_guard<std::mutex> guard( *get_shared_mutex() );
  return mpi_return{};
}

mpi_return mpi_mix_impl::operator()(std::integral_constant<api_tags, mpi_claim>, 
                                  const char * falias,
                                  MPI_ComputeObj::callback cb,
                                  int minfo, ulong * new_rank, metadata& md )
{
  
  get_lock().lock();
  {
    std::string claimId;

    std::cout << "entering ... mix_mpi_claim" << std::endl;
    std::string func_alias(falias); 
    ulong _rank=0;
    //get overrides from mpi_info object
    auto claim_ovr = _mpi_proc_impl->get_minfo_keyval( minfo );
    //get next global Id
    *new_rank = _mpi_proc_impl->request_new_rank();
    //register claim in the registry
    _claim_callback.insert( {falias, cb}  );
    //send request to the accelerator network
    _mpi_accel_impl->accel_claim( func_alias, claim_ovr, claimId ); 
    //add claim to registry
    _claim_lookup.insert({ *new_rank, claimId }); 
   
  }
  get_lock().unlock();

  return mpi_return{};
}

