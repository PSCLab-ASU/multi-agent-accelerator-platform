#include <nex_predicates.h>
template<OrderNexBy eql>
bool SortNexBy<eql>::Compare( const std::string cache_entry) 
{
  return false;
  //return resource == std::make_pair(lookup, cache_entry);
}

template<> 
bool SortNexBy<OrderNexBy::VALID_NEX>::operator()(jd_registry_type jd_reg)
{
  bool ret=false;
  //hw_nex_cache type
  auto nex = std::get<job_details::REG_NEX>(jd_reg);
  //std::optional function_entry
  auto config_data = resource.func_details;
  if( config_data )
  {
    //function_entry
    auto fe = config_data.value();
    std::cout << "contains_function : " << fe << std::endl;
    ret     = nex->contains_function(fe);
    std::cout << "ret = " << ret << std::endl; 

  }
  else ret = false;

  return ret;

}

template<>
bool SortNexBy<OrderNexBy::LEAST_BUSY>::operator()(jd_registry_type jd_reg)
{

  return false;
}
template<>
bool SortNexBy<OrderNexBy::LEAST_OCCUPIED>::operator()(jd_registry_type jd_reg)
{

  return false;
}

template<>
bool SortNexBy<OrderNexBy::LEAST_BUSY_PICO>::operator()(jd_registry_type jd_reg)
{

  return false;
}
