#include <ads_state.h>
#include <boost/range/algorithm/for_each.hpp>

ads_state::ads_state() 
{
  _active = false;
  _final  = false;
}

ads_state::ads_state(std::string LibId, std::string AccId) 
{
  _active = true;
  _final  = false;
  add_claim( LibId, AccId );
}

std::optional< std::reference_wrapper<ads_state::claim_header> >
ads_state::_get_claim_header( std::string LibId)
{
  auto claim = _claim_covr_lookup.find(LibId);
  
  if( claim != _claim_covr_lookup.end() )
  {
    return claim->second;

  }
  else std::cout << "Could not get_claim_header " << std::endl;
  return {};

}



std::string ads_state::get_accel_id( std::string LibId )
{
  std::cout << " entering ... " << __func__ << std::endl;
  auto claim = _get_claim_header( LibId );

  if( claim  )
  {
    auto[ _active, _AccId, _nodeId ] = claim.value().get();
    return _AccId;

  }
  else return "";

             
}
  
ulong ads_state::get_nex_id( std::string LibId )
{
  auto claim = _get_claim_header( LibId );

  if( claim )
  {
    auto[ _active, _AccId, _nodeId ] = claim.value().get();
    return _nodeId;

  }
  else return 0;
             
}

void ads_state::add_claim( std::string LibId, std::string AccId )
{
  _claim_covr_lookup.insert( {LibId, { false, AccId, 0 }}  );
}
 
void ads_state::update_claim( std::string LibId,
                           bool active, ulong nid)
{
  auto claim = _get_claim_header( LibId );

  if( claim )
  {
    auto&[ _active, _AccId, _nodeId ] = claim.value().get();
    _active = active;
    _nodeId = nid;

  }
  else std::cout << "Could not find claim update " << std::endl;

  
}

void ads_state::activate_claim( std::string LibId)
{
  auto claim = _get_claim_header( LibId );

  if( claim )
  {
    auto&[ _active, _AccId, _nodeId ] = claim.value().get();
    _active = true;

  }
  else std::cout << "Could not activate claim, claim not found " << std::endl;


}

void ads_state::deactivate_claim(std::string LibId )
{
  auto claim = _get_claim_header( LibId );

  if( claim )
  {
    auto&[ _active, _AccId, _nodeId ] = claim.value().get();
    _active = false;

  }
  else std::cout << "Claim not found " << std::endl;

}
        
bool ads_state::isCActive( std::string LibId )
{
  auto claim = _get_claim_header( LibId );

  if( claim )
  {
    auto&[ _active, _AccId, _nodeId ] = claim.value().get();
    return _active;
  }
  else std::cout << "Claim not found " << std::endl;

  return false;
}

bool ads_state::isCPending( std::string LidId )
{

  return false;
}

bool ads_state::isCClaimed( std::string LidId)
{

  return false;
}

ulong ads_state::get_node_index( std::string LidId)
{

  return 0;
}

std::string ads_state::get_accel_claim( std::string LidId)
{
  return "";
}
