#include <utils.h>
#include <zmsg_builder.h>
#include <string>
#include <vector>
#include <tuple>
#include <utility>

#ifndef NEXUSUTILS
#define NEXUSUTILS

namespace nexus_utils
{
  enum struct nexus_ctrl { ping, rem_manifest, init, rem_jinit, rem_claim, rem_cclaim, rem_rsync, 
                           rem_dealloc, rem_setparm, hw_rollcall, hw_id, hw_qry, 
                           hw_reg, hw_upt, hw_init, nex_upt, nex_uptd, nex_gid, nex_conn, 
                           nex_snd, nex_recv, nex_def };

  const std::map< nexus_ctrl, std::string> nexus_rolodex = 
  {
    {nexus_ctrl::ping,          "PING"               },
    {nexus_ctrl::rem_manifest,  "__REM_MANIFEST__"   },
    {nexus_ctrl::rem_jinit,     "__REM_JINIT__"      },
    {nexus_ctrl::rem_claim,     "__REM_CLAIM__"      },
    {nexus_ctrl::rem_cclaim,    "__REM_CCLAIM__"     },
    {nexus_ctrl::rem_rsync,     "__REM_RSYNC__"      },
    {nexus_ctrl::rem_dealloc,   "__REM_DEALLOC__"    },
    {nexus_ctrl::rem_setparm,   "__REM_SETPARAMS__"  },
    {nexus_ctrl::hw_rollcall,   "__HW_ROLLCALL__"    },
    {nexus_ctrl::hw_id,         "__HW_IDENTITY__"    },
    {nexus_ctrl::hw_qry,        "__HW_QUERYRES__"    },
    {nexus_ctrl::hw_reg,        "__HW_REGISTER__"    },
    {nexus_ctrl::hw_upt,        "__HW_UPDATE__"      },
    {nexus_ctrl::hw_init,       "__NEX_INITJOB__"    },
    {nexus_ctrl::nex_upt,       "__NEX_UPDATECACHE__"},
    {nexus_ctrl::nex_gid,       "__NEX_GID_REQ__"    },
    {nexus_ctrl::nex_conn,      "__NEX_CONN__"       },
    {nexus_ctrl::nex_snd,       "__NEX_SNDDATA__"    },
    {nexus_ctrl::nex_recv,      "__NEX_RECVDATA__"   },
    {nexus_ctrl::nex_uptd,      "__NEX_UPTDATA__"    },
    {nexus_ctrl::nex_def,       "DEFAULT"            }

  };
  
  nexus_ctrl reverse_lookup( std::string );
 
  zmsg_builder<std::string, std::string> 
    start_request_message( nexus_ctrl, std::string); 

  zmsg_builder<std::string, std::string, std::string> 
    start_routed_message( nexus_ctrl, std::string, std::string); 

}

#endif

