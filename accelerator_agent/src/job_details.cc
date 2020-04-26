#include <job_details.h>
/////////////////////////////////////////////////////////////////////////////////
///////////////////////////rank_interface///////////////////////////////////////
rank_interface::rank_interface()
{}

rank_interface::rank_interface(std::string nex_addr, std::string picoID)
{
  alloc_addr = std::make_pair(nex_addr, picoID);
}

rank_interface& rank_interface::operator=(const rank_interface& rhs)
{
  activation  = rhs.activation;
  priority    = rhs.priority;
  alloc_addr  = rhs.alloc_addr;
  pico_addr   = rhs.pico_addr;
  hw_addr     = rhs.hw_addr;
  intf        = rhs.intf;
  return *this;
}

void rank_interface::set_alloc_addr( std::string nex_addr, std::string picoID )
{
  alloc_addr = std::make_pair(nex_addr, picoID);
}

std::pair< std::string, std::string>
rank_interface::get_owner_pair() const
{
  return alloc_addr;
}

void rank_interface::set_pico_addr( std::optional<std::string> picoAddr={} )
{ 
   pico_addr = picoAddr;
}

void rank_interface::set_intf_option(std::string key, std::string val)
{
  //intf.insert( std::make_pair(key, val) );
}

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////job_detail//////////////////////////////////////
job_details::job_details()
{

}

job_details::job_details(std::string jobId)
: jobID(jobId)
{
  
}

job_details::job_details(std::string jobId, ulong total_cpu)
{

}

std::list<const_registry_type> job_details::get_c_registry_ptr()
{
   std::list<const_registry_type> temp;
   return temp;
}

std::list<jd_registry_type>& job_details::get_registry_ptr()
{
  return rank_registry;
}

template < bool toggle >
bool job_details::is_nexus_cached(std::string& inp )
{
  std::list<jd_registry_type>::iterator iter;

  using find_iterator   = decltype(iter);
  using find_lookupval  = std::pair<registry_search_types, std::string>;
  auto registry_search  = std::bind(
                                    std::find<find_iterator, find_lookupval>, 
                                    rank_registry.begin(),
                                    rank_registry.end(), 
                                    std::placeholders::_1);


  if( toggle )
    iter = registry_search(std::make_pair(job_details::FIND_NEX,  inp));
  else
    iter = registry_search(std::make_pair(job_details::FIND_RANK, inp));

  return std::get<job_details::REG_CACHES>(*iter);
}

function_entry job_details::get_function_template(std::string alias)
{

  auto fe = config_file.get_function_entry(alias);
  return fe;
  //lookup configuration entry
}

std::list<std::string> job_details::get_hostaddrs_from_config() 
{
  return config_file.get_all_host_addrs();
}

int job_details::set_current_rank(std::string cur_rank)
{
  current_rank_id = cur_rank;
  return 0;
}


ulong job_details::get_owner() const
{
  return std::stoul(current_rank_id);
}

/////////////////////////////////////////////////////////////////////////
////////////////////////////rank_desc////////////////////////////////////
rank_desc::rank_desc()
{
  Phase       = rank_phase::PENDING;
}

rank_desc::rank_desc(ulong owner, rank_types rank_t, std::optional<function_entry> f_entry )
{
  Phase        = rank_phase::PENDING;
  Type         = rank_t;
  owning_rank  = owner;

  func_details = f_entry;

}

rank_desc rank_desc::init_from_nex_cache( std::string nex_cache_line, rank_status& rstatus )
{
  rstatus = rank_status();
  return rank_desc();
}

void rank_desc::update_globalId(ulong groupId, ulong globalId)
{
  GroupID  = groupId;
  GlobalId = globalId;
}

std::pair<ulong, ulong> rank_desc::get_ggid() const
{
  return std::make_pair(GroupID.value(), GlobalId );
}

const std::string rank_desc::get_func_alias() const
{
  if( func_details )
    return func_details.value().get_func_alias();
  else return "";
}

rank_desc& rank_desc::operator=(const rank_desc& rhs)
{
  if( this != &rhs)
  { 
    GlobalId      = rhs.GlobalId;
    Type          = rhs.Type;
    ClaimID       = rhs.ClaimID;
    GroupID       = rhs.GroupID;
    Phase         = rhs.Phase;
  }   
  return *this;
}

int rank_desc::set_phase( const rank_desc::rank_phase rp) 
{
  Phase = rp;
  return 0;
}

const std::array< std::list< std::string >, 3> rank_desc::serialize() 
{
  std::list<std::string> rank_desc;
  
  auto insert_var = [&] ( std::string key, std::string val ) 
                    {
                      rank_desc.push_back(key + "=" + val );
                    };
  //rank_interface variables
  insert_var( "alloc_addr" ,   alloc_addr.first + "," + alloc_addr.second );
  if( pico_addr )
    insert_var( "pico_addr" ,  pico_addr.value()                );
  insert_var( "hw_addr" ,      hw_addr                          );
  insert_var( "intf" ,         intf                             );
  insert_var( "activation" ,   std::to_string(activation)       );
  insert_var( "priority" ,     std::to_string(priority)         );
  //rank_desc variables
  insert_var( "owner_rank" ,   std::to_string(owning_rank)      );
  insert_var( "GlobalId" ,     std::to_string(GlobalId)         );
  insert_var( "Type" ,         std::to_string(Type)             );
  insert_var( "rank_phase" ,   std::to_string((ushort) Phase)   );
  if( GroupID )
    insert_var( "GroupID",     std::to_string(GroupID.value())  );
  if( ClaimID ) 
    insert_var( "ClaimID",     std::to_string(ClaimID.value())  );

  std::cout << "serialized all rd" << std::endl;
  //get base_config
  auto [base, derived] = func_details->serialize();

  return {rank_desc, base, derived};
} 
////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////HELPER functions//////////////////////////////////////
bool operator==(const rank_desc& lhs, const rank_desc& rhs)
{
    return ( (lhs.GlobalId      == rhs.GlobalId)       &&
             (lhs.Type          == rhs.Type)           &&
             (lhs.ClaimID       == rhs.ClaimID)        &&
             (lhs.GroupID       == rhs.GroupID)        &&
             (lhs.Phase         == rhs.Phase) );
}

bool operator==(rank_desc& entry, 
                std::pair<const rank_desc::rank_search_types,const std::string> lookup)
{
  bool results = false;

  if( lookup.first == rank_desc::BYFNAME )
    results = (lookup.second == entry.get_func_alias());
  else if( lookup.first == rank_desc::BYGID)
    results = (lookup.second == std::to_string(entry.GlobalId));
  else if( lookup.first == rank_desc::BYNEXCACHE)
  {
    rank_status rstatus;
    rank_desc rd = rank_desc::init_from_nex_cache(lookup.second, rstatus);
    if( rstatus )
      results = (entry == rd );
    else 
      std::cout << "Failed to init rank" << std::endl;
  }
  return false;
}

bool operator==(jd_registry_type& entry, std::pair<jd_search_types, std::string> lookup)
{
  if( lookup.first == job_details::FIND_NEX )
  {
    //compares nexus hostnames
    return (std::get<job_details::REG_NEX>(entry) == lookup.second);
  }
  else if( lookup.first == job_details::FIND_RANK )
  {
    //get rank list
    rank_list& rlist = std::get<job_details::REG_RANKL>(entry);
    //look for the rank
    auto item  = std::find(rlist.begin(), rlist.end(),
                                 std::make_pair(rank_desc::BYGID, lookup.second) );
    return ( item != rlist.end() );
  }
  return false;
}

