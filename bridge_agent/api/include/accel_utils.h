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
  enum struct accel_ctrl {ping, ninit, rinit, send, recv, claim, shutdown,
                          test, finalize, nexus_rd, updt_manifest, claim_response };

  const std::map< accel_ctrl, std::string> accel_rolodex = 
  {
    {accel_ctrl::ping,           "PING"                 },
    {accel_ctrl::ninit,          "__ACCEL_NINIT"        },
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

#endif
