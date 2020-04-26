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

#ifndef PICO_CTL_T
#define PICO_CTL_T

class pico_ctrl
{
  public: 
    typedef std::function<int()> action_func;

    pico_ctrl();
    ~pico_ctrl();
    pico_ctrl(zmq::socket_t *, zmq::socket_t *, std::string, std::string, int );

    //entry point to all local hardware resources
    //                         method
    action_func action_ctrl(std::string);
 

  private:
   
    int            n_args;
    std::string    pico_name;
    std::string    claim;
    std::string    defintion_file;
    zmq::socket_t  * rtr_zsock;
    zmq::socket_t  * pub_zsock;
    std::map<std::string, action_func > command_set;

    //claiming rank from resource
    int _picctl_default_ ();
    int _picctl_sample_  ();
    int _picctl_fsample_ ();

    //private methods
    int _send(zmq::multipart_t&, bool );
    int _recv(zmq::multipart_t& );
    int _sendrecv_command(zmq::multipart_t&, zmq::multipart_t&, bool );
    int _rollcall();   

};

#endif
