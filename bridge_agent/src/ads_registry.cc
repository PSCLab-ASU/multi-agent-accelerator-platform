#include <ads_registry.h>
#include <algorithm>
#include <functional>
#include <boost/range/algorithm/for_each.hpp>

ads_registry::ads_registry()
{
    //setting up an empty entry
    _ads_registry.emplace(std::piecewise_construct, 
                          std::forward_as_tuple(std::string("EMPTY")), 
                          std::forward_as_tuple(std::string(""), 
                                                std::string("")) );
    auto& adss = _ads_registry["EMPTY"];
    adss.deactivate();
    adss.finalized();
}

const bool& ads_registry::is_all_ads_complete()
{
  _stop =  std::all_of( _ads_registry.begin(), 
                        _ads_registry.end(), 
                        [](const auto& entry )-> bool{
                          return entry.second.isActive();  
                        } );

  return _stop;
}

const bool& ads_registry::is_all_ads_finalized()
{
  _stop =  std::all_of( _ads_registry.begin(), 
                        _ads_registry.end(), 
                        [](const auto& entry )-> bool{
                          return entry.second.isFinal();  
                        } );

  return _stop;
}

ads_state& ads_registry::find_create_adss( std::string Rid, std::string LibId, std::string AccId, 
                                           bool& bExists, bool create)
{
  std::cout << "find_create_adss ... " << Rid << std::endl;

  auto temp = _ads_registry.find( Rid );

  if(create && (temp == _ads_registry.end()) )
  {
    bExists = false;
    auto e = _ads_registry.emplace(std::piecewise_construct, 
                                   std::forward_as_tuple(Rid), 
                                   std::forward_as_tuple(LibId, AccId) ).first;
    return (*e).second;
  }
  else if( !create && (temp == _ads_registry.end()) )
  {
    bExists = false;
    auto& e = _ads_registry["EMPTY"];
    return e;
  }
  else if(create && (temp != _ads_registry.end()) )
  {
    bExists = true;
    (*temp).second.add_claim(LibId, AccId);
    return (*temp).second;

  }
  else 
  {
    std::cout << "Found ADS entry..." <<std::endl; 
    bExists = true;
    return (*temp).second;
  }
}

std::optional<std::reference_wrapper<ads_state> >
ads_registry::find_adss( std::string rid )
{
  bool bExists = false;
  auto& adss = find_create_adss( rid, "", "",  bExists, false);
  
  if( bExists ) return adss;
  else return {};
}
