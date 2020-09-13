#include <client_utils.h>
#include <mpi_computeobj.h>
#include <global_allocator.h>
#include <memory>


#ifndef MPIPV_INTF
#define MPIPV_INTF

class mpi_pv_interface
{
  public:
    mpi_pv_interface( );
    mpi_pv_interface( bool );

    virtual void thread_main();
    virtual void system_update() = 0;
    virtual void init_thread_components() = 0;
    virtual mpi_return operator()( std::integral_constant<api_tags, mpi_init>, int *, char ***, metadata& ) = 0;
    virtual mpi_return operator()( std::integral_constant<api_tags, mpi_finalize>, metadata& ) = 0;
    virtual mpi_return operator()( std::integral_constant<api_tags, mpi_send>, const void *, int, uint, int, int, int, metadata& ) = 0;
    virtual mpi_return operator()( std::integral_constant<api_tags, mpi_recv>, void *, int, uint, int, int, int, MPI_Status*, metadata&) = 0;
    virtual mpi_return operator()(std::integral_constant<api_tags, mpi_claim>, const char *, MPI_ComputeObj::callback, int, ulong *, metadata&) = 0;
    virtual mpi_return operator()( std::integral_constant<api_tags, mpi_test>, metadata& ) = 0;
    virtual mpi_return operator()( std::integral_constant<api_tags, mpi_alloc_mem>, int, int, void **, metadata& ) = 0;
    virtual mpi_return operator()( std::integral_constant<api_tags, mpi_free_mem>, void **, metadata& ) = 0;
    
    std::shared_ptr<std::mutex>& get_shared_mutex();
    std::unique_lock<std::mutex>& get_lock();

    void wait();
    void check_wait();
    bool is_async();
    void stop_driver( );
    void enable_async( ); 
    void own_thread( std::thread&& thread );
    void set_current_meta( const metadata& );
    std::shared_ptr<global_allocator> get_global_allocator();
 
  private:

    std::shared_ptr<global_allocator> _global_allocator;
    std::shared_ptr<std::mutex> _mu;
    std::unique_lock<std::mutex> _lk;
    std::atomic_bool _stop_driver;
    bool _async_enabled;
    std::optional<metadata> _meta;
    std::optional<std::thread> _worker_thread;
};

#endif
