#include <string>
#include <utility>
#include <variant>
#include <map>
#include <tuple>
#include <mutex>
#include <list>
#include <pending_msg_reg.h>


#ifndef PENDINGMSGS
#define PENDINGMSGS

struct send_pending_msg : _base_pending_msg
{
  send_pending_msg( std::string AaId, std::string rid)
  : _base_pending_msg(  AaId, rid ) {};

};

struct recv_pending_msg : _base_pending_msg
{
  recv_pending_msg( std::string AaId, std::string rid)
  : _base_pending_msg(  AaId, rid ) {};

};

#define PPM  send_pending_msg, recv_pending_msg

class pico_pending_msg_registry : public
      pending_msg_registry< PPM > 
{
 
  public:

  private:


};




#endif
