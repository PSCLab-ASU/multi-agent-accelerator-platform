#include <map>
#include <zmq.hpp>
#include <zmq_addon.hpp>
#include <functional>
#include <iostream>

#ifndef ALLOCTRACK
#define ALLOCTRACK

struct FIND_ROOT {};

typedef struct _alloc_tracker{

  _alloc_tracker();
  _alloc_tracker( std::string );
  _alloc_tracker( std::initializer_list<std::string> );
  friend std::ostream& operator<<(std::ostream& os, const _alloc_tracker & item);
  friend bool operator==(const _alloc_tracker &, 
                         const std::pair<std::string, std::string>&);
  friend bool operator==(const _alloc_tracker &, 
                         const std::pair<std::string, FIND_ROOT>);
  int set_function_list( std::vector<std::string> );
  //tracks function list
  std::vector< std::string > function_list;
  //hardware tracker id
  std::string hw_tr_id;
  //home nexus (defines the nexus that owns the process)
  //format is tcp://hostname:port
  std::string home_nex;
  //job nexus (defines the local nexus for the accel doing the job
  //format is tcp://hostname:port
  std::string job_nex;
  //job id per mpirun
  std::string job_id;
  //allocated rank number
  std::string rank_id;
  //function id
  std::string func_id; 

} alloc_tracker;


#endif
