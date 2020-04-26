#include <hw_nex_cache.h>
#include <job_details.h>
#include <algorithm>
#include <list>
#include <utils.h>
#include <functional>
#include <algorithm>
#include <tuple>
#include <rank_arbiter.h>
#include <claim_subsys_base.h>

#ifndef JOBINTF
#define JOBINTF

class job_interface : public job_details
{
  public:

  job_interface();
  //construct a jobID
  job_interface(std::string);
  //construct a jobID
  job_interface(std::string, ulong);
  //this should load all the locals & 
  //convinient member function
  job_interface(std::string, ulong, std::list<std::string>); 
  //add nex and the global cpu rank
  int add_nex_and_rank( bool, nex_cache_ptr&, std::string);
  //add nex and the global cpu rank
  int add_rank_zero( nex_cache_ptr&, ulong);

  friend bool operator==(std::shared_ptr<job_interface>&,
                         std::string&);
  friend bool operator==(jd_registry_type&, const nex_cache_ptr&);

  // ulong=rank
  int sync_job( std::string );
  // ulong rank
  int update_nexus(std::list<std::string>, std::string );
 
  //creates the next global Ids (request -> calls allocate)
  int allocate_global_ids(const ulong, ulong&, std::list<ulong>&);

  //make_claims
  int make_claims( const std::list<function_entry>&,
                   std::list<ulong>& );

  //get rank_desc based on global Id;
  int get_rank_desc(ulong, rank_desc& );
  //get global_ids
  int get_global_ids( ulong, std::list<ulong>& );
  //get primary rank interface
  int get_primary_rt_interface(ulong, std::list<rank_interface>&);
 
  //set configuration file
  template<typename ...T>
  int set_configfile(std::list<T>...);

  private:
  
  //request global ID from rank zero nexusa
  //return is the global Id (request calls allocate)
  int _request_global_id(const ulong, ulong&, std::list<ulong> &);
  //tranmit claims to the pico services
  int _transmit_claims( const std::list<rank_desc>& );
  //register claim to support synchronization 
  int _register_claims( std::list<rank_desc>& ); 

  int load_convinient_locals( ulong );
  //send message to a specific  nexus parm1: host_name
  int _send_zmsg_to_nex( const std::string, const zmq::multipart_t);
  //send message by rank to home nexus rank_id
  int _send_zmsg_by_rank( const std::string, const zmq::multipart_t);
  //broadcast a rank 
  int _broadcast_rank( rank_desc& );
  //broacast zmsg
  int _broadcast_zmsg( const zmq::multipart_t&, bool );
 
  //get recommendatio system
  std::unique_ptr<claim_subsys_base>& _get_recommd_sys(std::string);  

  //global rank counter
  ulong global_rank_cnt;
  ulong global_group_cnt;
  //rank arbitrar - needs to friend rank_perfmon, nexus_perfmon
  rank_arbiter _rank_arb_ptr;

  friend class claim_subsys_base;
};

typedef std::shared_ptr<job_interface> job_interface_ptr;

#endif
