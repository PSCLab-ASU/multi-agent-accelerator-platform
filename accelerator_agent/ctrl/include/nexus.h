#include <map>
#include <set>
#include <zmq.hpp>
#include <zmq_addon.hpp>
#include <functional>
#include <iostream>
#include <algorithm>
#include <tuple>
#include <utility>
#include <utils.h>
#include <vector>
#include <unistd.h>
#include <optional>
#include <list>
#include <array>

#ifndef NEXCTL_T
#define NEXCTL_T
enum  : ushort {NEXCTL_CHAR=0, NEXCTL_LONG, NEXCTL_SHORT, NEXCTL_ULONG, NEXCTL_FLOAT, NEXCTL_DOUBLE, NEXCTL_BOOL}; 

typedef std::map<std::string, std::string> InputType;
typedef std::optional<std::list<std::string> > extra_params;

class nexus
{
  public: 
    typedef std::function<int(InputType)> action_func;

    nexus();
    ~nexus();
    nexus(zmq::context_t *, std::string, std::string );

    //entry point to all local hardware resources
    //                      method        host_list
    action_func action_ctrl(std::string, std::string);
 
    //claiming rank from resource
    int _nexctl_default_       (InputType);
    int _nexctl_queryres_      (InputType);
    int _nexctl_claim_         (InputType);
    int _nexctl_rollcall_      (InputType);

  private:
    //this variables gets updated everytime a call is made to
    //the frontend
    typedef struct _nxhost {
      
      std::string host_addr;
      zmq::context_t ztx;
      zmq::socket_t z_sock;

      void connect( std::string addr) { 
        host_addr = addr; 
        std::cout << "connecting tcp://" + addr <<std::endl;
        z_sock.connect("tcp://"+addr);
      } //end of connect

      //constructor
      _nxhost(zmq::context_t * ctx, std::string name, std::string host) 
      : ztx(), z_sock(ztx, ZMQ_DEALER) 
      {
          z_sock.setsockopt( ZMQ_IDENTITY, name.c_str(),
                             name.length());
          connect( host );
      } //end of constructor
      bool operator<(const struct _nxhost& h) const { return true; }
      
      void close_socket(){ z_sock.close(); }

      //destructor
      ~_nxhost() { std::cout << "Calling nexhost destruct" << std::endl; 
                   close_socket();
                 } 

    } nxhost;

    std::string nex_name;
    std::set< nxhost > nxhost_set;
    zmq::context_t * zcontext;
    std::string nx_port;
    std::map<std::string, std::string> current_params;
    std::map<std::string, action_func > command_set;

    //private methods
    template <size_t N>
    int _broadcast_gather(std::string, std::array<extra_params, N> );
    int _gather_nex_results();

    template <size_t N>
    int _broadcast_nex_command(std::string, std::array<extra_params,N> );

};

#endif
