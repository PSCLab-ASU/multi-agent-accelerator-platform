#include <string>
#include <utility>
#include <variant>
#include <map>
#include <tuple>
#include <mutex>
#include <list>
#include <pending_msg_reg.h>

#ifndef ACCEL_PENDMSGS
#define ACCEL_PENDMSGS

struct final_pending_msg  : _base_pending_msg
{
  final_pending_msg ( std::string libId, std::string rid)
  : _base_pending_msg( libId, rid ) {};
  
};

struct init_pending_msg  : _base_pending_msg
{
  init_pending_msg ( std::string libId, std::string rid)
  : _base_pending_msg( libId, rid ) {};

  
};

struct claim_pending_msg : _base_pending_msg
{
  claim_pending_msg( std::string libId, std::string rid) 
  : _base_pending_msg( libId, rid){};
  
  void set_claimId( std::string cid ) { _claimId = cid; }   
  std::string get_claimId( ) { return _claimId; }   

  std::string _claimId;
};

struct send_pending_msg : _base_pending_msg
{
  send_pending_msg( std::string libId, std::string rid) 
  : _base_pending_msg( libId, rid){};

};

#define APM init_pending_msg, claim_pending_msg, send_pending_msg, other_pending_msg, final_pending_msg

class accel_pending_msg_registry : public
      pending_msg_registry< APM >
{

  public:

    std::list< init_pending_msg> get_init_msgs( bool bremove )
    {
      return _get_all_msgs_by_type<init_pending_msg>( bremove);
    }

    std::list< final_pending_msg> get_final_msgs( bool bremove )
    {
      return _get_all_msgs_by_type<final_pending_msg>( bremove);
    }

  private:

};

#endif

