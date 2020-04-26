#include <hw_nex_cache.h>
#include <list>
#include <utility>
#include <accel_def_lut.h>
#include <map>
#include <memory>
#include <tuple>
#include <algorithm>
#include <functional>
#include <vector>
#include <optional>
#include <config_components.h>

#ifndef JOBDETAILS
#define JOBDETAILS

struct rank_interface{
  //default ctor
  rank_interface();
  rank_interface(std::string, std::string);
  //copy operator
  rank_interface& operator=(const rank_interface& );
  //set nex and picoID
  void set_alloc_addr(std::string, std::string);
  //set the runtime pico addr
  void set_pico_addr(std::optional<std::string>);
  //set optional interfaces
  void set_intf_option(std::string, std::string);
  //get data 
  std::pair<std::string, std::string> get_owner_pair() const;
  //remote address types
  //bool is an activation flag
  //uint is the priority
  //string is the IP address, or mac address..etc and ports
  bool activation;
  uint priority;
  //nex_addr/picoID
  std::pair<std::string, std::string> alloc_addr;
  std::optional<std::string>          pico_addr;
  std::string                         hw_addr;
  std::string                         intf;
};

struct rank_status {
   uint error_code;
   std::string error_msg;
   rank_status(std::optional<uint> err={}, 
               std::optional<std::string> msg={} )
   {
     error_code = err.value_or(0);
     error_msg  = msg.value_or("Success");
     
   };
   operator bool() const { return (error_code == 0); };
};

class rank_desc : public rank_interface
{
  public:
  //rank type
  enum rank_types { ACCEL, CPU };
  //this search returns a list
  enum rank_search_types { BYFNAME, BYALIAS, BYLID, BYGID, BYNEXCACHE };
  //rank phase consist of different phases a rank can be in
  //PENDING means no global Id, just temporary Id (for tracking)
  //REQUESTED  means there is a global_id, but no claim Id (SYNCHRONOUS)
  //CLAIMED means there is a claim ID, and remote address
  //TRANSFERING means ranks in a state of transference
  //LOST means connection can no longer be established
  //DEACTIVATED means the rank is no longer valid 
  enum struct rank_phase { PENDING, REQUESTED, 
                           CLAIMED, TRANSFERING, LOST, DEACTIVATED}; 
  //keeps track if the rank has
  //inited from init_job
  ulong          owning_rank;   //
  ulong          GlobalId;      //Global Rank ID
  ulong          Type;          //CPU or ACCEL
  rank_phase     Phase;         //Phases of the rank

  std::optional<ulong> 
                 ClaimID;       //Claim Number given by pico service
  std::optional<ulong>
                 GroupID;       //Manage rank groupings (future)

  std::optional<function_entry>
                 func_details;  //configuration file details  
  
  //hold meta data consistent to this rank
  //meta data should be optional and at the descreation of the pico service
  //Allows the pico service to do optional optimizations
  //includes constraints like leasing times
  std::map<std::string, std::string> gen_overrides;
  //default construct
  rank_desc();
  rank_desc(ulong owner, rank_types rank_t, std::optional<function_entry> fe_entry={});

  int set_phase( const rank_phase );

  //public methods
  rank_status set_global_id( std::string id ) 
              { 
                GlobalId = stol(id); 
                return rank_status();
              };
  //import a resource string 
  static rank_desc init_from_nex_cache( std::string, rank_status& );

  auto get_config_data() { return func_details; };
  
  std::pair<ulong, ulong> get_ggid() const;

  void update_globalId( ulong, ulong);
 
  const std::string get_func_alias() const;

  const std::array<std::list<std::string>, 3>  serialize ();

  //operator
  rank_desc&  operator=(const struct rank_desc&);
  //operator friends
  friend bool operator==(const struct rank_desc&, const struct rank_desc&);

  friend bool operator==(struct rank_desc&,
                         std::pair<const rank_search_types, const std::string>);


  private:

};

using rank_list           =  std::list<rank_desc>;
using jd_registry_type    =  std::tuple<bool, nex_cache_ptr, rank_list>;
using const_registry_type =  std::tuple<const bool, cnex_cache_ptr, const rank_list>; 

class job_details
{

  public:
  //keys to traverse the rank_registry
  enum registry_search_types { FIND_NEX, FIND_RANK }; 
  enum registry_index { REG_CACHES, REG_NEX, REG_RANKL};

  //friend function
  friend bool operator==(jd_registry_type&, 
                         std::pair<registry_search_types, std::string> );

  template<bool>
  bool is_nexus_cached( std::string& );

  //return const registry
  std::list<const_registry_type> get_c_registry_ptr();
  //get mutable registry
  std::list<jd_registry_type>& get_registry_ptr();

  //
  function_entry get_function_template(std::string);

  //get a list of host addresses
  std::list<std::string> get_hostaddrs_from_config();

  int set_current_rank ( std::string );
  ulong get_owner ( ) const;

  protected:
  std::string jobID;
  std::string current_rank_id;
  //rank tracker
  //second bool represents if this nex has 
  //bool keeps track on whether the nexus update has been called
  //from the local nexus targetting the remote one
  //registry
  std::list< jd_registry_type > rank_registry;
  //std::list< std::tuple<bool, nex_cache_ptr, rank_list> > rank_registry;
  //acceleration lookup table from configuration file
  std::list< accel_def_lut > accelerator_definitions;
  //total number of CPU ranks for job
  //ulong total_cpu_ranks;
  //hold the rank zero entry of the socket
  std::optional< std::shared_ptr<hw_nex_cache> > rank_zero;
  //hold the local nex cache for this CPU rank
  std::shared_ptr<hw_nex_cache> local_nex_cache;
  //hold the local_rank_list
  std::list<rank_desc>* local_rank_list;
  //support funtions
  bool is_nexus_cached(std::string inp);
  //ctor
  job_details();
  job_details(std::string);
  job_details(std::string, ulong);

  //_config_components config_file; 
  configfile config_file;

  private: 

};

using jd_search_types  =  job_details::registry_search_types;


#endif
