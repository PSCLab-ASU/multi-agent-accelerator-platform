#include "ncache_registry.h"
#include <boost/range/algorithm.hpp>

///////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////cacged resource desc//////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////

void cached_resource_desc::_insert( int indx, std::string val )
{
  auto entry = std::next(_func_desc.begin(), indx);
  entry->second = val;  
}
  
void cached_resource_desc::_insert( std::string key, std::string val)
{
  auto vfh_key = reverse_map_find(g_func_map, key); 
  _func_desc.at( vfh_key ) = val;
}
  
void cached_resource_desc::_fill_wildcards()
{
  for(auto globals : g_func_map )
  {
    if( globals.second == _func_desc.at(globals.first))
      _func_desc.at(globals.first) = "*";
  }
}

std::string cached_resource_desc::stringify() const
{
  return g_func_map.at(vfh::HW_VID)    + "=" + _func_desc.at(vfh::HW_VID)    + ":" +
         g_func_map.at(vfh::HW_PID)    + "=" + _func_desc.at(vfh::HW_PID)    + ":" +
         g_func_map.at(vfh::HW_SS_VID) + "=" + _func_desc.at(vfh::HW_SS_VID) + ":" +
         g_func_map.at(vfh::HW_SS_PID) + "=" + _func_desc.at(vfh::HW_SS_PID) + ":" +
         g_func_map.at(vfh::SW_VID)    + "=" + _func_desc.at(vfh::SW_VID)    + ":" +
         g_func_map.at(vfh::SW_PID)    + "=" + _func_desc.at(vfh::SW_PID)    + ":" +
         g_func_map.at(vfh::SW_FID)    + "=" + _func_desc.at(vfh::SW_FID)    + ":" +
         g_func_map.at(vfh::SW_CLID)   + "=" + _func_desc.at(vfh::SW_CLID)   + ":" +
         g_func_map.at(vfh::SW_VERID)  + "=" + _func_desc.at(vfh::SW_VERID); 
}
///////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////ncache regsitry///////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////

//functional existense implies you can execute
//a kernel on which only applies to FID and CLID
uint ncache_registry::functional_exists( std::string clid, std::optional<std::string> fid ) const
{

  bool clid_match=false, fid_match=false;

  clid_match = std::any_of( _caches.cbegin(), _caches.cend(), 
                            cached_resource_desc::compare<vfh::SW_CLID>(clid));

  if( fid ) 
  fid_match = std::any_of( _caches.cbegin(), _caches.cend(), 
                           cached_resource_desc::compare<vfh::SW_FID>(fid.value()));
                            
  if( fid_match ) return 1; 
  else if( clid_match ) return 2;
  else return 0;
  
}

//equal to the equality operator
bool ncache_registry::functional_exists( cached_resource_desc res) const
{
  return std::any_of( _caches.cbegin(), _caches.cend(), 
                      cached_resource_desc::compare_functional(res.get_sw_clid(), res.get_sw_fid()) );
}

bool ncache_registry::can_execute( cached_resource_desc res) const
{
  return std::any_of( _caches.cbegin(), _caches.cend(), 
                      cached_resource_desc::compare_execute(res.get_sw_clid(), res.get_sw_fid()) );
}

//equal to the equality operator
bool ncache_registry::exact_exists( cached_resource_desc res) const
{
  return std::any_of( _caches.cbegin(), _caches.cend(), 
                      [&](auto entry){ return (res == entry); } );
}

bool ncache_registry::can_completely_support( std::string res_str) const
{
  
  cached_resource_desc res( res_str);
  
  bool functional = functional_exists( res.get_sw_clid(), res.get_sw_fid() );

  bool hw_support = can_execute( res );

  return functional && hw_support;
}

uint ncache_registry::can_support( std::string hw_vid, std::string hw_pid,
                                   std::optional<std::string> hw_ss_vid, 
                                   std::optional<std::string> hw_ss_pid) const
{
  bool exact_match=false;

  exact_match = std::any_of( _caches.cbegin(), _caches.cend(), 
                             cached_resource_desc::compare_supportable(hw_vid, hw_pid, 
                                                                       hw_ss_vid, hw_ss_pid) );

  if( exact_match ) return 1; 
  else return 0;
  
}
   
//find full line
std::optional<cached_resource_desc>
ncache_registry::find_functional(std::string clid, std::optional<std::string> fid)
{
  auto entry = std::find_if( _caches.begin(), _caches.end(),
                             [&]( const cached_resource_desc& input )
                             {
                               return (clid == input.get_sw_clid() ) || 
                                      (fid && (fid.value() == input.get_sw_fid()) );
                             });
  if( entry == _caches.end() ) return {};
  else return *entry; 
}

std::list<std::string> ncache_registry::dump_manifest() const
{
  std::list<std::string> out;
  boost::range::for_each(_caches, [&]( auto res ) {
                         out.emplace_back( res.stringify() );
                        });
  return out;

}
