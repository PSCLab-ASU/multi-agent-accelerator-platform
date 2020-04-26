#include <map>
#include "utils.h"
#include "zmsg_builder.h"
#include "zmsg_viewer.h"

#ifndef PICOUTILS
#define PICOUTILS

namespace pico_utils
{
  typedef std::function<void(void *)> send_deleter_t;
  enum struct pico_ctrl { ping, rollcall, reg_resource, ident_resource, alloc_resource, sys_shutdown, send, recv };

  const std::map<pico_ctrl, std::string> pico_rolodex = 
  {
    {pico_ctrl::ping,           "PING"                  },
    {pico_ctrl::send,           "__REG_SEND__"          },
    {pico_ctrl::recv,           "__REG_RECV__"          },
    {pico_ctrl::rollcall,       "__REG_ROLLCALL__"      },
    {pico_ctrl::reg_resource,   "__REG_RESOURCE__"      },
    {pico_ctrl::alloc_resource, "__ALLOC_RESOURCE__"    },
    {pico_ctrl::sys_shutdown,   "__SYS_SHUTDOWN__"      },
    {pico_ctrl::ident_resource, "__IDENTIFY_RESOURCE__" }

  };

  pico_ctrl reverse_lookup( std::string );

  zmsg_builder<std::string, std::string>
    start_request_message( pico_ctrl, std::string );
  
  void send_deleter( void * );

}


#endif
