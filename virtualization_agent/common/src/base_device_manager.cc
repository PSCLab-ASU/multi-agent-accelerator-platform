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

void base_device_manager::add_kernel_desc( std::string res)
{
  _cache.add_resource( res );
}

std::list<std::string> base_device_manager::get_manifest() const 
{
  auto out = _cache.dump_manifest();
  return out;
}
