#include "pico_cfg.h"
pico_cfg::pico_cfg()
{
}

const std::string pico_cfg::get_external_address()
{
  std::lock_guard lk(mu);
  return _external_address;
}

pico_return pico_cfg::set_external_address(std::string addr)
{
  std::lock_guard lk(mu);

  _external_address = addr;

  return pico_return{};
}

const std::string pico_cfg::get_owner() 
{
  std::lock_guard lk(mu);
  
  return _service_owner;
}

pico_return pico_cfg::set_owner( std::string owner)
{
  std::lock_guard lk(mu);
  _service_owner = owner;
  return pico_return{};
}

const std::string pico_cfg::get_repos() 
{
  std::lock_guard lk(mu);
  std::string merge_repos;
  merge_repos = std::accumulate(std::begin(_repos), 
                               std::end(_repos),
                               std::string(""), 
                               [](std::string a, std::string b)
                               {
                                 return a + std::string(":") + b;
                               } ); 

  return merge_repos;
}

pico_return pico_cfg::set_repos(std::string repo_path, bool bAppend)
{
  std::lock_guard lk(mu);
  auto new_repos = split_str<':'>(repo_path);
  
  if( !bAppend ) _repos.clear();

  _repos.insert(_repos.begin(), new_repos.begin(), new_repos.end() );

  return pico_return{};
}

const std::string pico_cfg::get_filters( ) 
{
  std::lock_guard lk(mu);
  std::string id;
  std::stringstream merge_ids;

  boost::copy( _filters, std::ostream_iterator<std::string>(merge_ids, ":") );
  id = merge_ids.str();
  id.pop_back();
  return id;
}

pico_return pico_cfg::set_filters( std::string filts )
{
  std::lock_guard lk(mu);
  _filters.clear();

  _filters = split_str<':'>(filts);
  
  return pico_return{};
}

