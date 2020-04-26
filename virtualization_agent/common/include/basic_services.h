#include "pico_utils.h"
#include "shmem_store.h"

#ifndef BASICSERV
#define BASICSERV

enum command_set_t { REQUEST_SET, RESPONSE_SET };

class basic_services{
  public :
    basic_services();

    using action_func = std::function<pico_return(ulong)>;

    virtual action_func action_ctrl( pico_utils::pico_ctrl ) {};  
   
    void operator()( ulong );

    void set_shmem_store ( std::shared_ptr<shmem_store>& );

    //g_method_index get_method_from_tid( ulong tid );

    void set_io_queues( std::pair<std::optional<single_q>, 
                                  std::optional<single_q> > );
  
    void shutdown(){ _bShutdown = true; }
    const bool& is_shutdown(){ return _bShutdown; }

    void set_command_set_t( command_set_t cst) {_action_set = cst; } 
    const command_set_t& get_command_set_t( ) const { return _action_set; } 

    std::shared_ptr<pico_pending_msg_registry>& get_pmsg()
    { return _pmsg; } 
 
    std::shared_ptr<pico_cfg>& get_pcfg()
    { return _pcfg; } 

    std::shared_ptr<shmem_store::pkt_gen_ts>& get_pktgen()
    { return _pktgen; } 

    std::shared_ptr<kernel_interface_kdb >& get_kintf()
    { return _kernel_db; }

    std::shared_ptr<shmem_store>& get_shmem_store()
    { return _shmem_store; } 

    void send_upstream( const ulong& );
    void send_downstream( const ulong& );

    void forward_sys_resp( ulong );
    void forward_dev_req ( ulong );

    template<typename T>
    std::shared_ptr<T>& get_entry( const ulong& tid, pico_return& pret )
    {
      auto& v_entry = _pktgen->get_entry(tid, pret);
      auto& entry   = std::get< std::shared_ptr<T> >( v_entry );
      return entry;
    }

    template<typename T>
    const T& get_misc_payload( const ulong& tid, bool& data_exists);

    const misc_string_payload& get_misc_string_payload( const ulong& tid, bool& data_exists );
    const identify_payload& get_idn_payload( const ulong& tid, bool& data_exists );
    const send_payload& get_send_payload( const ulong& tid, bool& data_exists );

  private:

    const base_req_resp::var_bdata_t& _get_base_data( const ulong& tid, bool& data_exists);

    template<bool>
    pico_return _send_data( const ulong& );

    bool _bShutdown;
    std::shared_ptr<shmem_store> _shmem_store; 
    std::shared_ptr<pico_pending_msg_registry> _pmsg;
    std::shared_ptr<shmem_store::pkt_gen_ts> _pktgen;
    std::shared_ptr<kernel_interface_kdb > _kernel_db;
    std::shared_ptr<pico_cfg> _pcfg;

    std::pair<std::optional<single_q>, 
              std::optional<single_q> > _main_io_queue;
    command_set_t _action_set;

};



#endif
