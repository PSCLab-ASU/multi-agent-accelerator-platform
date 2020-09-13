#include <client_utils.h>
#include <config_components.h>
#include <mpi_computeobj.h>
#include <zmsg_viewer.h>
#include <zmsg_builder.h>
#include <mpi_accel_utils.h>
#include <accel_utils.h>
#include <mpi_pending_msg.h>
#include <global_allocator.h>

#ifndef MPIACCEL_IMPL
#define MPIACCEL_IMPL

struct zmqreq_header {};

#define ACTION_ACCEL_SIG zmsg_viewer<std::string, std::string, std::string>& input, \
                         std::optional<zmq::multipart_t >& output

class mpi_accel_impl 
{
  public:

    using action_func = std::function< mpi_return(ACTION_ACCEL_SIG) >;

    mpi_accel_impl(std::string, std::string, std::string, std::string, 
                   std::shared_ptr<std::mutex>&,
                   std::shared_ptr<std::condition_variable>&, 
                   std::unique_lock<std::mutex>&,
                   bool async=false);

    action_func action_ctrl(std::string); 

    mpi_return finalize();

    mpi_return accel_init(const ulong&, const ulong& );

    mpi_return accel_claim( std::string, std::map<std::string, std::string>, std::string& );

    mpi_return accel_send(int, std::string, const MPI_ComputeObj*, int, int);
    mpi_return accel_send(int, std::string, const void*, int, int, int);

    mpi_return accel_recv(MPI_ComputeObj*, int, int );
    mpi_return accel_recv(void*, int, int, int, int );

    bool import_configfile( std::string );
 
    bool check_service() { return _is_service_up; };

    void system_update();

    std::optional<zmq::multipart_t > try_pop_caq_itm();
  
    void shutdown_runtime_agent();
 
    void close_sockets() 
    {
      _zsock.close();
      _zctx.close();
    } 

  private:

   void _process_recv_message( mpi_recv_pmsg&, MPI_ComputeObj* );

   std::optional<zmq::multipart_t > _get_next_message_info( );

   auto _start_message( accel_utils::accel_ctrl method, std::string& key)
   {
     key = generate_random_str();
     zmsg_builder zBuilder = accel_utils::start_request_message( method, key); 
     return zBuilder;
   }

   void _send_zmsg( auto& zmsgb) 
   {
     std::cout << "accel_send : " << zmsgb.get_zmsg() << std::endl;
     zmsgb.get_zmsg().send( _zsock ); 
   }

   auto _recv_zmsg( zmq::multipart_t& zmsg )
   {
     zmsg.recv( _zsock );
     zmsg_viewer<std::string, std::string, 
                 std::string> zmsgv( zmsg );
     return zmsgv; 
   }

   auto _sendrecv_zmsg( auto& zmsgb)
   {
     _send_zmsg( zmsgb );
     return _recv_zmsg( zmsgb.get_zmsg() );
   }
   
   void _reset_mailbox() { _ct_mailbox.reset(); } 
 
   bool _check_service( );  
   ///////////////////////////////////////////////////////////////////
   //////////////////////////system registry//////////////////////////
   ///////////////////////////////////////////////////////////////////
   mpi_return _response_action( ACTION_ACCEL_SIG );
   mpi_return _default_action( ACTION_ACCEL_SIG );
   mpi_return _update_claim( ACTION_ACCEL_SIG );
   mpi_return _recv( ACTION_ACCEL_SIG );

   ///////////////////////////////////////////////////////////////////
   ///////////////////////////////////////////////////////////////////
   ///////////////////////////////////////////////////////////////////
   //mailbox
   std::optional< 
        std::pair<std::string, zmq::multipart_t> > _ct_mailbox;  
   //hold the mix mutex
   std::shared_ptr<std::mutex>              _mix_mutex;
   //holds the mic conditional variable
   std::shared_ptr<std::condition_variable> _mix_cv;
   //extra lock
   std::unique_lock<std::mutex>&            _mix_lk;
   //is service up
   bool _is_service_up;
   //job identification
   std::string _jobId;
   //repo directories
   std::string _repos;
   //store rank information
   ulong _this_rank;
   //service address
   std::string _service_address;
   //host file address
   std::string _host_file;
   std::optional<configfile> _config_file;
   //zmq socket to xcelerate
   zmq::context_t _zctx = zmq::context_t();
   zmq::socket_t  _zsock;
   std::mutex     _zmtx;
   //function registry
   std::map<client_utils::client_ctrl, action_func> _action_registry;
   std::queue< zmq::multipart_t > _cross_action_q;
   //claim registry
   std::vector<claim_info> _claim_registry;
   //pending messages registry
   mpi_pending_msg_registry _pmsg_registry;
   //global allocator
   std::shared_ptr<global_allocator> _galloc;
};

#endif

