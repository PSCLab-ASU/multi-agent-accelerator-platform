#include <utils.h>
#include <zmsg_builder.h>
#include <string>
#include <vector>
#include <tuple>
#include <utility>
#include <optional>


#ifndef ACCLUTIL
#define ACCLUTIL


namespace accel_utils
{
  enum struct accel_ctrl {ping, sinit, ninit, rinit, send, recv, claim, shutdown,
                          test, finalize, nexus_rd, updt_manifest, claim_response };

  const std::map< accel_ctrl, std::string> accel_rolodex = 
  {
    {accel_ctrl::ping,           "PING"                 },
    {accel_ctrl::ninit,          "__ACCEL_NINIT"        },
    {accel_ctrl::sinit,          "__ACCEL_INIT_SA__"    },
    {accel_ctrl::rinit,          "__ACCEL_RINIT__"      },
    {accel_ctrl::updt_manifest,  "__UPDATE_MANIFEST__"  },
    {accel_ctrl::claim,          "__ACCEL_CLAIM__"      },
    {accel_ctrl::claim_response, "__ACCEL_CLAIM_RESP__" },
    {accel_ctrl::test,           "__ACCEL_TEST__"       },
    {accel_ctrl::nexus_rd,       "__ACCEL_REDIRECT__"   },
    {accel_ctrl::send,           "__ACCEL_SEND__"       },
    {accel_ctrl::recv,           "__ACCEL_RECV__"       },
    {accel_ctrl::shutdown,       "__ACCEL_SHUTDOWN__"   },
    {accel_ctrl::finalize,       "__ACCEL_FINALIZE__"   }
 
  };
 
  std::optional<accel_ctrl> reverse_lookup( std::string val ); 


  zmsg_builder<std::string, std::string, std::string> 
    start_request_message( accel_ctrl, std::string); 

  zmsg_builder<std::string, std::string, std::string, std::string> 
    start_routed_message( accel_ctrl, std::string, std::string); 

}

struct accel_header
{
  std::optional<std::string> requesting_id;
  accel_utils::accel_ctrl method;
  std::optional<std::string> requesting_nex;
  std::string tag;

  std::optional<ulong> get_nexus_id()
  {
 
    if( requesting_id ) return std::stoul( requesting_nex.value() );
    else return {};
  }

};

struct status_agent
{

  enum struct app_state {NOTSTARTED, STARTED, INITIALIZING, INITIALIZED, RUNNING};

  std::string to_string( app_state state )
  { 
    switch(state)
    {
      case app_state::NOTSTARTED:   return "NOTSTARTED";
      case app_state::STARTED:      return "STARTED";
      case app_state::INITIALIZING: return "INITIALIZING";
      case app_state::INITIALIZED:  return "INITIALIZED";
      case app_state::RUNNING:      return "RUNNING";
      default: return "NA";
    }
  }

  void update_status( app_state state)
  {
    if( state > _state )
    {
      _state = state;
      _state_updated = true;
    }
  }

  std::pair<bool, zmq::multipart_t> generate_status_msg()
  {
    bool state_chg = _state_updated;
    _state_updated = false;
    if( addr )
    { 
      zmsg_builder msg( addr.value(), to_string(_state), 
                        generate_random_str() );
      msg.finalize();
      return std::make_pair<bool, zmq::multipart_t>
             (std::move(state_chg), std::move(msg.get_zmsg()) );
    } else return { false, zmq::multipart_t{} };
 
  }
  std::optional<std::string> addr;
  app_state _state = app_state::NOTSTARTED;
  bool _state_updated = false;

};

#endif
