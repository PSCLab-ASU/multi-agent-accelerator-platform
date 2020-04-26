#include <map>
#include <zmq.hpp>
#include <zmq_addon.hpp>
#include <functional>
#include <iostream>
#include <alloc_tracker.h>
#include <utility>

_alloc_tracker::_alloc_tracker()
{}

_alloc_tracker::_alloc_tracker(std::string rank_id) : rank_id(rank_id)
{}

_alloc_tracker::_alloc_tracker( std::initializer_list<std::string> init)
{
  std::vector< std::string > vec (init);

  hw_tr_id = vec[0];
  home_nex = vec[1];
  job_nex  = vec[2];
  job_id   = vec[3];
  rank_id  = vec[4];
  func_id  = vec[5]; 

}

std::ostream& operator<<(std::ostream& os, const _alloc_tracker & item)
{
  return os << std::endl    <<
               "hw_tr_id: " << item.hw_tr_id <<std::endl <<
               "home_nex: " << item.home_nex <<std::endl <<
               "job_nex: "  << item.job_nex  <<std::endl <<
               "job_id: "   << item.job_id   <<std::endl <<
               "rank_id: "  << item.rank_id  <<std::endl <<
               "fun_id: "   << item.func_id  <<std::endl;
}

bool operator==(const _alloc_tracker & lhs,  
                const std::pair<std::string, std::string > & id)
{
   return (lhs.job_id == id.first) && (lhs.rank_id == id.second);
}

//used to find the entry with the root job
bool operator==(const _alloc_tracker & lhs,  
                const std::pair<std::string, FIND_ROOT > id)
{
   return (lhs.job_id == id.first) && (lhs.rank_id == std::to_string(0) );
}

int _alloc_tracker::set_function_list( std::vector<std::string> func_list)
{
  //move function to allocation
  function_list = func_list;

  return 0;
}

