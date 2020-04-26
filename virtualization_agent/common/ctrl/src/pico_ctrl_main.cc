#include <fstream>
#include <functional>
#include <map>
#include <thread>
#include <pico_utils.h>
#include <pico_ctrl.h>
#include <utils.h>
#include <unistd.h>

using namespace std;

int main(const int argc, const char *argv[])
{

  cout << "Welcome to Picoctl"<<endl;
  std::string method    ="action";
  std::string claim_id  ="claim";
  std::string n_args    ="nargs";
  std::string stim_file ="input_file";
  int in_args           = 1;
  int status            = -1;
  bool repeat           = true;

  //modifies original variables
  get_input_params(argc, argv, true, method, claim_id, stim_file, n_args);

  //generate context
  zmq::context_t context(1);
  zmq::socket_t router(context, ZMQ_ROUTER);
  router.setsockopt(ZMQ_IDENTITY, "pico_ctrl_rtr", 13);
  zmq::socket_t pub( context, ZMQ_PUB );
  pub.setsockopt(ZMQ_IDENTITY, "pico_ctrl_pub", 13);

  std::string rtr_addr = "ipc:///home_nexus";
  std::cout << "address : " << rtr_addr << std::endl;
  router.bind(rtr_addr);

  std::string pub_addr = "ipc:///prouting";
  std::cout << "pub_address : " << pub_addr << std::endl;
  pub.bind(pub_addr);
  
  in_args = atoi( n_args.c_str() );

  std::cout << "Launch pico service, then press any key to continue..." << std::endl;

  std::cin.ignore();
  pico_ctrl picctl( &router, &pub, claim_id, stim_file, in_args);

  std::cout << "starting main loop" << std::endl;
  const char escape = 27;
  while ( 1 ) 
  {
    status = picctl.action_ctrl(method)();
    std::cout << "ESC, then [Enter] to QUIT, [Enter] to REPEAT" << std::endl;
    if(std::cin.get() == escape)
    {
      std::cout << "Exiting pico_ctrl..." << std::endl;
      break;
    }
    
  }

  return 0;

}
