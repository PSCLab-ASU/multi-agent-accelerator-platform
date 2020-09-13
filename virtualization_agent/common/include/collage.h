#include <ranges>
#include "pico_utils.h"
#include "device_services.h"
#include "system_services.h"
#include "shmem_store.h"
#include "global_allocator.h"

#ifndef COLLAGE
#define COLLAGE

//#define PICO_IN_TYPE zmsg_viewer<> & request
#define PICO_IN_TYPE  zmq::multipart_t && zrequest
#define PICO_OUT_TYPE ulong tid, zmq::multipart_t& resp

using namespace std::ranges::views;

class collage 
{

  public: 

    using req_action_func  = std::function<pico_return(PICO_IN_TYPE)>;
    using resp_action_func = std::function<pico_return(PICO_OUT_TYPE)>;

    collage();
    collage( ulong );

    void print_header();

    pico_return set_external_address( std::string);

    pico_return set_owner( std::string);

    pico_return set_repo(  std::string repo_path, bool bAppend=false );

    pico_return set_filters( std::string);

    pico_return shutdown_services();

    pico_return init_services();
    //MAIN QUEUE entry/exit points processing function
    pico_return submit_request(target_sock_type, zmq::multipart_t& ); 
    pico_return process_q_resp( zmq::multipart_t &, target_sock_type&);

  private:    

    template<typename T>
    std::shared_ptr<T>& _get_entry( const ulong& tid, pico_return pret)
    {
      auto& v_entry = _pktgen->get_entry(tid, pret);
      auto& entry   = std::get< std::shared_ptr<T> >( v_entry );
      return entry;
    }

    std::shared_ptr<system_response_ll>&
    _get_system_response(const ulong&, pico_return& );

    const base_req_resp::var_bdata_t& _get_base_data( const ulong& tid, bool& data_exists);

    std::pair<req_action_func, resp_action_func> _action_ctrl(std::string );

    template<typename T>
    const T& _get_misc_payload( const ulong& tid, bool& data_exists);

    const misc_string_payload& _get_misc_string_payload ( const ulong& tid, bool& data_exists );

    template<ushort stage = 0>
    pico_return _send_downstream ( auto& );
    ////////////////////////////////////////////////////////////////////////////////
    //COMMAND SET
    ///////////////////////////////////////////////////////////////////////////////
    pico_return _create_transaction_resp( zmq::multipart_t & );   

    //request handlers
    pico_return _col_reg_resource_req(PICO_IN_TYPE);
    pico_return _col_idn_resource_req(PICO_IN_TYPE);
    pico_return _col_send_req(PICO_IN_TYPE);
    //response handlers
    pico_return _col_reg_resource_resp(PICO_OUT_TYPE);
    pico_return _col_idn_resource_resp(PICO_OUT_TYPE);
    pico_return _col_send_resp(PICO_OUT_TYPE);
    pico_return _col_recv_resp(PICO_OUT_TYPE);
    //send data

    void _deallocate_shmem_store();

    ////////////////////////////////////////////////////////////////////////////////

    ///////MEMBERS//////////////////////////////
    std::atomic_ulong running_transaction_cnt;   

    header_info _crh;

    std::map<pico_utils::pico_ctrl, 
             std::pair<req_action_func, resp_action_func> > command_set;

    //shared queues
    q_arry  _req_qs;

    q_arry _resp_qs;

    //system services for pico services functionality non-device related 
    std::pair<std::thread, system_services> _sys_serv;
    //devices related functionality
    std::pair<std::thread, device_services> _dev_serv;
    //shmem store
    std::shared_ptr<shmem_store> _shmem_store;
    //pico configuration
    std::shared_ptr<pico_cfg> _pcfg;
    //pktgen interface
    std::shared_ptr<shmem_store::pkt_gen_ts> _pktgen;
    //global allocator
    std::shared_ptr<global_allocator> _galloc;
};




#endif
