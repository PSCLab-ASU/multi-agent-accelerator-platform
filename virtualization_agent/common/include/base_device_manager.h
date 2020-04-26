#include <queue>
#include <functional>
#include "utils.h"
#include <mpi.h>
#include "resource_logic.h"
#include "payload_details.h"
#include "execution_engine.h"


#ifndef BASEDEVMGR
#define BASEDEVMGR

struct devmgr_ret
{
  uint err;
  std::string msg;
};

struct base_device_manager_ees{

  using ExecInputType   = std::pair<ulong, std::reference_wrapper<const send_payload> >;
  using ExecOutputType  = recv_payload; 
  using Header          = ExecInputType::second_type::type::header;
  using ExecCommandType = std::function<std::vector<ExecOutputType>( Header, ExecInputType ) >; 

};

class base_device_manager : public execution_engine<base_device_manager_ees, 64>
{
  
  public:

    base_device_manager();

    devmgr_ret enumerate_devices();

    devmgr_ret list_devices( std::string, std::vector<std::string>& );

    devmgr_ret get_file_metadata( const std::pair<std::string, std::string>&,
                                  std::string&, std::string& );

    const std::vector<std::string> get_valid_file_exts() const;

  private:

};

#endif

