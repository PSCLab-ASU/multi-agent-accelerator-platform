#include <list>
#include <string>
#include <map>
#include <job_details.h>
#include <type_traits>
#include <memory>
#include <algorithm>
#include <nex_predicates.h>

#ifndef CLAIMSUBSYS
#define CLAIMSUBSYS

struct claim_ret {
  int error;
  std::string err_msg;
};

class claim_subsys_base{

  public:
    claim_subsys_base(){};
   
    //initialize the rank_registry
    claim_ret init( std::list<jd_registry_type>* );

    //will make final recommendation
    virtual claim_ret make_recommendation( std::list<rank_desc>& );
    
    std::string ID;
  private:
    //will do the in initial partitioning 
    claim_ret _init_recommendation(const rank_desc& rreq);

    ////////////////////////////////////////////////////////////
    //these functions manipulate the list with:
    //step 1: partitioning, Step 2: Sorting
    //given a function ID get least busy pico service
    claim_ret _orderby_least_busy_pico_service(){ return claim_ret{};}; //TBD
    //get nexus with the least amount claims
    claim_ret _orderby_least_occupied_nex() { return claim_ret{}; };     //TBD
    //get nexus with the least amount 
    claim_ret _orderby_least_busy_nex() { return claim_ret{}; };         //TBD
    ////////////////////////////////////////////////////////////
    //get const registry_ptr
    std::pair< std::list<jd_registry_type>::iterator,
               std::list<jd_registry_type>::iterator> 
               _get_iterator_pair() 
               {
                 return std::make_pair(iter[0], iter[1]);
               };

    //if function id exists
    bool is_claimable( );

    bool is_inited; 

    //used to keep track of the ordering
    std::list<jd_registry_type>::iterator iter[2];

    std::list<jd_registry_type > * registry_ptr;
};

// linked to claim_subsys_base;
extern std::map<std::string, std::unique_ptr<claim_subsys_base> > 
            g_recommd_registry;

class round_robin_scatter : public claim_subsys_base 
{
  public:
  
    round_robin_scatter() : claim_subsys_base() 
    { 
      this->ID = std::string("RR_SCATTER");
      g_recommd_registry.emplace( this->ID, this );
    } 
};


#endif

