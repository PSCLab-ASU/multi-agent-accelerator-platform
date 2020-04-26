#include <fstream>
#include <functional>
#include <map>
#include <thread>
#include <res_mgr_frontend.h>
#include <utils.h>

using namespace std;

int main(const int argc, const char *argv[])
{

  cout << "Welcome to the HW Nexus"<<endl;
  std::string intra_addr="internal_port";
  std::string inter_addr="external_port";
  std::string pub_addr  ="publisher_port";
  std::string repo = "repo";
  std::string nex_home = "nex_home";
  //modifies original variables
  get_input_params(argc, argv, false, intra_addr, inter_addr, pub_addr, repo, nex_home);

  //generate context
  zmq::context_t context;
  // intranode router for local processes
  zmq::socket_t conn (context, ZMQ_ROUTER);
  zmq::socket_t pub (context, ZMQ_PUB);

  //mpirun connects to intranode 
  std::string interaddr = std::string("tcp://*:") + inter_addr;
  std::string intraaddr = std::string("ipc:///")  + intra_addr;
  std::string pubaddr   = std::string("ipc:///")  + pub_addr;
  std::cout << "External Addr: " << interaddr <<std::endl;
  std::cout << "Internal Addr: " << intraaddr <<std::endl;
  std::cout << "Publisher Addr: " << pubaddr   <<std::endl;

  std::string home = std::string("nexus-") + get_hostname();
  conn.setsockopt(ZMQ_IDENTITY, home.c_str(), home.length());

  //bind all ports
  conn.bind(interaddr); //bind external port
  conn.bind(intraaddr); //bind to internal resources
  pub.bind(pubaddr);    //bind broadcast

  //Start the proxy
  cout << "Stating ZMQ Proxy"<<endl;
  //need to let resourced connect to nexus
  //resources are added through the init function
  resmgr_frontend frontend_intf(&conn, &pub, inter_addr, repo, nex_home);
  //set zmq context to be used for the rest of the nex
  frontend_intf.set_zmq_context( &context ); 
 
  while(1)
  {
    zmq::multipart_t request, reply;
    request.recv(conn);
    std::cout << "Recived req: "<< request <<endl;
    //process_addr_
    std::string proc_addr = request.popstr();
    std::cout << "proc_addr: "<< proc_addr <<endl;
    //method
    std::string method    = request.popstr();
    std::cout << "method: "<< method <<std::endl;
    //process id
    std::string key = request.popstr();
    std::cout << "msg_key: "<< key <<std::endl;
    //communicate to the local HW nexus
    int status_further_action = frontend_intf.action_ctrl(proc_addr, method, key)
                                                         (&request, &reply);
    if( !reply.empty() && (status_further_action >= 0) ) 
    {
      ////////////////////////////////////////////////
      /////////////repond to the requester////////////
      //////////////////////////////////////////////// 
      //push address to mpirun
      //reply.pushstr("");
      if( status_further_action == 0 )
        reply.pushstr(proc_addr);

      std::cout << "sending nex msg: "<< reply << std::endl;
      reply.send(conn);
    }
    else if( !reply.empty() && (status_further_action == 1) ) 
    {
      std::cout <<"Forwarding request..." <<std::endl;
    }
    else if( reply.empty() )
    {
      std::cout <<"No response needed..." <<std::endl;
    }

  }

  conn.close();
  pub.close();
  cout << "Completed ZMQ Proxy"<<endl;

  return 0;

}
