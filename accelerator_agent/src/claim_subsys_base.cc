#include <claim_subsys_base.h>
#include <list>
#include <algorithm>
#include <memory>
#include <nex_predicates.h>
#include <iterator>

//THIS IS THE REAL REGISTRY
std::map<std::string, std::unique_ptr<claim_subsys_base> > g_recommd_registry;

bool claim_subsys_base::is_claimable( )
{
  return ( std::distance(iter[0], iter[1]) > 0 );
}

claim_ret claim_subsys_base::init( std::list<jd_registry_type> * regry_ptr)
{
  registry_ptr = (regry_ptr);
  is_inited    = (false);
  iter[0]      = registry_ptr->begin();
  iter[1]      = registry_ptr->end();
 

  return claim_ret{};
} 

claim_ret claim_subsys_base::_init_recommendation(const rank_desc& rreq)
{

  //shuffle nexuses second parition find the canidates  
  auto It = std::partition( registry_ptr->begin(), 
                            registry_ptr->end(),
                            SortNexBy<OrderNexBy::VALID_NEX>(rreq) );

  iter[0] = registry_ptr->begin();
  iter[1] = It;
  is_inited = true;
  //at this point the list was parititon between nexus that can fill the request
  //versus the ranks that can't. The first group is the rank that can't fill
  //the second group are the ranks that can fill
  return claim_ret{};

}

//default round robin, rotating per nexus
claim_ret claim_subsys_base::make_recommendation( std::list<rank_desc>& rank_reqs )
{
  std::cout << "ROUND_ROBIN Recommendation Engine" << "\n";
  //length of the ranks
  ulong len = rank_reqs.size();
  ulong idx = 0;
  for( auto& rank : rank_reqs)
  {
    _init_recommendation( rank );
    auto [bIter, eIter] = _get_iterator_pair();
    //number of nexuses
    ulong nex_len = std::distance(bIter, eIter);
    std::cout << "distance = " << nex_len << std::endl;
    //check to see if 
    if( is_claimable() )
    {
      std::cout << " is claimable..." << std::endl;
      //found atleast one nexus
      //std::advance(bIter, (idx % nex_len) );
      auto It = std::next(bIter, (idx % nex_len) );
      if( rank.func_details ) 
      {
        std::cout << " func_details..." << std::endl;
        auto& nex_ptr  = std::get<job_details::REG_NEX>(*It);
        //auto nex_addr  = bIter->get_hostaddr();
        auto nex_addr  = nex_ptr->get_hostaddr();
        //get the frist element in the list of pico services
        function_entry fe   = rank.get_config_data().value();
        auto pico_id = nex_ptr->get_pico_service_list( fe ).front();
        std::cout << nex_addr << " : " << pico_id << " : fe = " << fe << std::endl;
        rank.set_alloc_addr(nex_addr, pico_id);
      }
    }
    else
    {
      //found no nexus
      std::cout << "No Nexus's that compare" <<std::endl;
    }   
    idx++;
  }

 


  return claim_ret{};
}

REGISTRY_ENTRY(round_robin_scatter)

