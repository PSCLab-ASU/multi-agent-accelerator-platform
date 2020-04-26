#include <type_traits>
#include <job_details.h>

#ifndef NEXPRED
#define NEXPRED

enum struct OrderNexBy {VALID_NEX, LEAST_BUSY, LEAST_OCCUPIED, LEAST_BUSY_PICO}; 

template <OrderNexBy order>
struct SortNexBy{

  SortNexBy ( rank_desc res ) 
  : resource(res){};

  bool Compare( const std::string cache_entry);

  bool operator()(jd_registry_type jd_reg);

  rank_desc resource;

};

#endif
