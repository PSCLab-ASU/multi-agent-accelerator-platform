#include "base_device_manager.h"

#define NUM_THREADS 0

base_device_manager::base_device_manager()
: execution_engine(NUM_THREADS) 
{


}

devmgr_ret base_device_manager::enumerate_devices()
{
  return devmgr_ret{};
}

devmgr_ret base_device_manager::list_devices( std::string, std::vector<std::string>&)
{

  return devmgr_ret{};
}

devmgr_ret base_device_manager::get_file_metadata( const std::pair<std::string, std::string>&,
                                                   std::string&, std::string&)
{

  return devmgr_ret{};
}

const std::vector<std::string> base_device_manager::get_valid_file_exts() const
{
  return {};
}

