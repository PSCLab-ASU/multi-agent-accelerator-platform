#include <client_utils.h>
#include <mpi_pv_interface.h>
#include <mpi_computeobj.h>
#include <mpi_proc_utils.h>
#include <mpi.h>
#include <atomic>

#ifndef MPIPROC_IMPL
#define MPIPROC_IMPL

#define ACTION_REG_SIGNATURE mpi_proc_utils::mpi_proc_pkt_viewer& input,  \
                                   std::optional<mpi_proc_utils::mpi_proc_pkt_viewer>& output

class mpi_proc_impl : public mpi_pv_interface
{
  friend class mpi_mix_impl;

  public:
    
    using action_func = std::function< mpi_return( mpi_proc_utils::mpi_proc_pkt_viewer& , 
                                                  std::optional<mpi_proc_utils::mpi_proc_pkt_viewer>& ) >;   

    mpi_proc_impl(std::shared_ptr<std::mutex> &, 
                  std::shared_ptr<std::condition_variable> &,  
                  std::unique_lock<std::mutex>&,
                  bool async = false);  

    action_func action_ctrl( mpi_proc_utils::mpi_proc_mid );

    mpi_return operator()( std::integral_constant<api_tags, mpi_init>, int *, char ***, metadata&) final;
    mpi_return operator()( std::integral_constant<api_tags, mpi_finalize>, metadata& ) final;
    mpi_return operator()( std::integral_constant<api_tags, mpi_claim>, 
                           const char *, 
                           MPI_ComputeObj::callback, 
                           int, ulong *, metadata& ) final;
    mpi_return operator()( std::integral_constant<api_tags, mpi_send>, const void *, int, uint, int, int, int, metadata& ) final;
    mpi_return operator()( std::integral_constant<api_tags, mpi_recv>, void *, int, uint, int, int, int, MPI_Status*, metadata& ) final;
    mpi_return operator()( std::integral_constant<api_tags, mpi_test>, metadata& ) final;
 
    void system_update();
    void init_thread_components();
    bool is_initialized(){ return (_is_mpi_initialized != 0); }

    const ulong& get_current_rank() const;
    const ulong& get_world_size() const;
    int get_app_tag_ub();

    std::map<std::string, std::string> get_minfo_keyval( MPI_Info );
    //get a new global group Id
    ulong request_new_rank(  );

    void all_rank_barrier() { MPI_Barrier(MPI_COMM_WORLD); } 

  protected:
    bool is_processor_rank( const ulong& rank) const;
    std::map<std::string, std::string> _get_minfo_keyval( MPI_Info );
    std::optional<mpi_proc_utils::mpi_proc_pkt_viewer> try_pop_caq_itm();

  private:
    std::optional<mpi_proc_utils::mpi_proc_pkt_viewer> _get_next_message_info( );
    int _get_tag_upper_bound();

    void _reset_mailbox();
 
    template<typename... Ts>
    void _wait_for_message(Ts ...);

    mpi_return _default_action( ACTION_REG_SIGNATURE );
    mpi_return _response_action( ACTION_REG_SIGNATURE );
    mpi_return _get_groupid( ACTION_REG_SIGNATURE );
   
   
    ulong _world_size;
    ulong _current_rank;
    const int _rank_zero = 0;
    int   _system_tag;
    int   _is_mpi_initialized;
    std::atomic_bool _atomic_mpi_initialized=false;
    mpi_proc_utils::rank_zero_vars zrglobals;
    
    //mailbox to cross threading domain
    std::pair<std::optional<mpi_proc_utils::proc_header>,
              std::optional<mpi_proc_utils::mpi_proc_pkt_viewer> > _ct_mailbox;
    //hold the mix mutex
    std::shared_ptr<std::mutex>                          _mix_mutex;
    //holds the mic conditional variable
    std::shared_ptr<std::condition_variable>             _mix_cv;
    //shared unique lock
    std::unique_lock<std::mutex>&                        _mix_lk;
    //hold the actions set for the asynchronous capability
    std::map<mpi_proc_utils::mpi_proc_mid, action_func>  _action_registry;
    //hold the keys to all the MPI_Infos for recall
    std::map<MPI_Info, std::list<std::string> >          _mpi_info_kregy; 
    //queue to connect to mix (TBD)
    std::queue< mpi_proc_utils::mpi_proc_pkt_viewer >    _cross_action_q;
};

#endif

