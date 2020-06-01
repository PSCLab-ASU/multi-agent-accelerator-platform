#include <argparser.h>
#include <zmq.hpp>
#include <zmq_addon.hpp>
#include <cstdlib>
#include <iterator>     // std::iterator, std::input_iterator_tag
#include <unistd.h>
#include <utils.h>
#include <accel_service.h>
#include <zmsg_builder.h>
#include <zmsg_viewer.h>
#include <client_utils.h>
#include <boost/range/algorithm/for_each.hpp>

using namespace std::ranges::views;

int main(int argc, char ** argv)
{
  mpi_return status;
  bool options_exists=false, default_accel=false;
  int org_argc = argc;
  std::string new_exec_str="";
  const std::string nex_addr   = std::string("ipc:///home_nexus");
  ////////////////////////////////////////////////////////////////////////
  auto[asa, jobId, host_file, 
       repo,  async, spawn_bridges,
       src_bridge_addr, src_bridge_port] = get_init_parms( &argc, &argv);
  //needs to line up with the input args
  /////////////////////////////////////////////////////////////////////////
  const std::string accel_ext_addr = std::string("tcp://*:"+src_bridge_port);
  const std::string accel_addr = std::string("ipc:///home_accel-") + jobId;
  /////////////////////////////////////////////////////////////////////////
  std::cout << "Accel_addr = " << accel_addr << std::endl;
  if( asa.empty() ) { default_accel=true; asa = accel_addr; }

  for(auto i : iota(1, org_argc) )
  {
    std::string parm = argv[i];
    new_exec_str += parm + std::string(" ");
    if( parm == ACCEL_INPUT_PIVOT ) options_exists = true;
  }

  //add the accelerator section 
  if( !options_exists ) new_exec_str += std::string(" ") 
                                     + ACCEL_INPUT_PIVOT 
                                     + std::string(" ");
 
  new_exec_str += "--accel_address=" + asa + std::string(" ");
  new_exec_str += "--accel_job_id=" + jobId + std::string(" ");

  //router path
  //nexus is a hardcoded address
  //generate context
  zmq::context_t context;
  //Socket facing clients router
  zmq::socket_t frontend (context, ZMQ_ROUTER);
  //frontend router to the CPU ranks
  frontend.bind( accel_addr );
  frontend.bind( accel_ext_addr );
  //home nexus connections are in the accel_service interface
  //all async recieving channel happen throught the router
  auto bridge_dst = std::make_pair(src_bridge_addr, src_bridge_port);
  accel_service xcelerate_intf = accel_service(jobId, nex_addr, host_file, 
                                               repo, spawn_bridges  );
  //generate a bridge  
  auto bridge_init_msg = accel_service::generate_bridge_init( jobId );

  xcelerate_intf.add_bridge_init( [&bridge_init_msg, &bridge_dst]()
  {
    //sending init to bridge 
    send_adhoc_zmsgs( zmq_transport_t::EXTERNAL, bridge_dst, 
                      std::move(bridge_init_msg) );
  });

  while(!xcelerate_intf.check_stop())
  {
    zmq::multipart_t request;

    int stat = request.recv(frontend, ZMQ_NOBLOCK);
    if( !request.empty() ) std::cout << "Xcelerate got msg: " << request << std::endl;

    status = xcelerate_intf.submit( std::move(request) );

    auto responses = xcelerate_intf.get_messages();
    boost::for_each( responses, [&]( auto& response )
    {
      if( response )
      {
         std::cout << "Xcelerate: Sending msg :  " << response.value() << std::endl;
         response.value().send( frontend );
      }
   
    });
  }
  std::cout <<"Xcelerate: router socket" << std::endl;
  frontend.close();
  std::cout << "Xcelerate: Closing Xcelerate..."<<std::endl;

  return 0;
}

