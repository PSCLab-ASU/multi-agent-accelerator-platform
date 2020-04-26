#include <client_utils.h>
#include <mpi_pv_interface.h>
#include <mpi_proc_impl.h>
#include <mpi_accel_impl.h>

#ifndef MPIMIX_IMPL
#define MPIMIX_IMPL

struct claim_data
{
  bool active;
  std::string func_alias;
  MPI_ComputeObj::callback cb;
  const bool& is_active() const { return active; }
};

class mpi_mix_impl : public mpi_pv_interface
{
  public:
   
    mpi_mix_impl(std::string, std::string, std::string, std::string, bool async=false);
   
    template<typename ... Ts>
    mpi_return router( metadata& md, Ts... Targs)
    {
      bool proc_rank=true, dst_exists= md.dst_rank_exists();
      mpi_return mret;
      //convert Targs to a touple
      //and call operator()
      if( dst_exists ) 
       proc_rank = _mpi_proc_impl->is_processor_rank( md.dst_rank.value());

      if( proc_rank && !md.force_mix_route )
      {
        //call legacy mpi interface
        mret = _mpi_proc_impl->operator()(Targs..., md);
      }
      else
      {
        //call the mix operators
        mret = this->operator()(Targs..., md);
      }

      //if updated are synchronous
      if( !is_async() ) system_update();
     
      //wait for child to stop
      //if( is_async() ) check_wait();

      return mret;
    }
   
    void init_thread_components() final;
    void system_update() final;

    mpi_return operator()( std::integral_constant<api_tags, mpi_init>, int *, char ***, metadata& ) final;
    mpi_return operator()( std::integral_constant<api_tags, mpi_finalize>, metadata& ) final;

    mpi_return operator()( std::integral_constant<api_tags, mpi_claim>, const char *, MPI_ComputeObj::callback, int, ulong *, metadata& ) final;

    mpi_return operator()( std::integral_constant<api_tags, mpi_send>, const void *, int, uint, int, int, int, metadata& ) final;
    mpi_return operator()( std::integral_constant<api_tags, mpi_recv>, void *, int, uint, int, int, int, MPI_Status*, metadata& ) final;
    mpi_return operator()( std::integral_constant<api_tags, mpi_test>, metadata& ) final;

  private:
    
    //connect the data from MPI to accel services and vice-versa
    void _execute_mixture();
   
    //conditional variable for proc
    std::shared_ptr<std::condition_variable> _cv;
    //connection to the legacy MPI
    std::unique_ptr<mpi_proc_impl> _mpi_proc_impl;
    //connection to the accel MPI
    std::unique_ptr<mpi_accel_impl> _mpi_accel_impl;
    //is acceleration services up?
    bool _is_service_up;
    //callback registry for the RPC callsa
    std::map<std::string, MPI_ComputeObj::callback> _claim_callback;
    //claimer tracker key: rank_id
    std::map<ulong, std::string> _claim_lookup;
};

#endif
