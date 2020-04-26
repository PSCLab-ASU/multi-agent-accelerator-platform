#include <fstream>
#include <functional>
#include <map>
#include <thread>
#include <nexus.h>
#include <utils.h>
#include <unistd.h>

using namespace std;

int main(const int argc, const char *argv[])
{

  cout << "Welcome to Nexusctl"<<endl;
  std::string inter_addr ="external_port";
  std::string method     ="action";
  std::string host_list  ="hosts";
  std::string remainder  ="remainder"; //parameter to get the rest of the params
  //modifies original variables
  get_input_params(argc, argv, true, inter_addr, method, host_list, remainder);
  auto methods = split_str<','>(method);

  //generate context
  zmq::context_t context;

 
  InputType inp; 
  for(auto action : methods)
  {
    //Start the proxy
    //need to let resourced connect to nexus
    //resources are added through the init function
    nexus nexusctl(&context, inter_addr, remainder);
    int status = nexusctl.action_ctrl(action, host_list)(inp);
    usleep(5e6);
  }
  return 0;

}
