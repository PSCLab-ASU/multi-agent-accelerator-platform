#include "config_components.h"
#include "resource_logic.h"
#include "utils.h"
#include <map>

resource_logic::resource_logic( std::string res )
{
  if( !res.empty() ) this->set_rsign( res );
}

void resource_logic::set_rsign( std::string res )
{
  resource    = res;
  auto& fmap  = g_func_map; 
  auto ventry = split_str<':'>( resource );

  resource_desc[fmap.at(vfh::HW_VID)]    = "*"; //vendor id
  resource_desc[fmap.at(vfh::HW_PID)]    = "*"; //product id
  resource_desc[fmap.at(vfh::HW_SS_VID)] = "*"; //subsystem vendor id  
  resource_desc[fmap.at(vfh::HW_SS_PID)] = "*"; //subsystem product id  
  resource_desc[fmap.at(vfh::SW_VID)]    = "*"; //software vendor id
  resource_desc[fmap.at(vfh::SW_PID)]    = "*"; //software product id  
  resource_desc[fmap.at(vfh::SW_CLID)]   = "*"; //software class id
  resource_desc[fmap.at(vfh::SW_FID)]    = "*"; //software function id
  resource_desc[fmap.at(vfh::SW_VERID)]  = "*"; //software version id
  
  if( !resource.empty() )
  {
    for(auto entry : ventry)
    {  
      auto keyval = split_str<'='>( entry );
      resource_desc.at( keyval[0] ) = keyval[1];
    }
  }else std::cout << "No Resource ... " << std::endl;

}

bool resource_logic::operator== ( const resource_logic& rhs ) const
{
  bool result = true;
  auto& fmap  = g_func_map; 
  for( auto entry : rhs.resource_desc )
  {
    auto[key, value] = entry;
    if ( (this->resource_desc.at(key) != "*") && (value != "*") ) 
      result &= (this->resource_desc.at(key) == value);   
  }
  //need either function id or class id to recall a function 
  if(resource_desc.at(fmap.at(vfh::SW_FID))== "*" || 
     rhs.resource_desc.at(fmap.at(vfh::SW_FID)) == "*" )
  {
    if(resource_desc.at(fmap.at(vfh::SW_CLID))== "*" || 
      rhs.resource_desc.at(fmap.at(vfh::SW_CLID)) == "*" )
        result &= false;
  }
 
  return result;
}

bool resource_logic::operator< ( const resource_logic& rhs ) const
{
  return !(*this == rhs);
}

