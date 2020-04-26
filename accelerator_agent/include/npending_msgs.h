#include <string>
#include <utility>
#include <variant>
#include <map>
#include <tuple>
#include <mutex>
#include <list>
#include <pending_msg_reg.h>

#ifndef NEXUS_PENDMSGS
#define NEXUS_PENDMSGS

struct nsend_pending_msg : _base_pending_msg
{
  nsend_pending_msg(std::string AccId, std::string xid)
  : _base_pending_msg(AccId, xid ) {};

};

struct nrecv_pending_msg : _base_pending_msg
{
  nrecv_pending_msg(std::string AccId, std::string xid)
  : _base_pending_msg(AccId, xid ) {};

};

#define NPM nsend_pending_msg, nrecv_pending_msg

class nexus_pending_msg_registry : public
      pending_msg_registry< NPM > 
{
  public:


  private:


};



#endif
