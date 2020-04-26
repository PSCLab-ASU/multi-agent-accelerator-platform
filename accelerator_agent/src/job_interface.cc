#include <job_interface.h>
#include <zmsg_builder.h>
#include <tuple>
#include <config_components.h>

using l_strings = std::list<std::string>;

job_interface::job_interface() {}

job_interface::job_interface(std::string jobId) 
: job_details(jobId)
{
  global_rank_cnt  = 0;
  global_group_cnt = 0;
}

job_interface::job_interface(std::string jobId, ulong global_rank)
: job_details(jobId)
{
  global_rank_cnt  = 0;
  global_group_cnt = 0;
}

job_interface::job_interface(std::string jobId, ulong cpu_rank_limit,
                             l_strings extra_params)
: job_details(jobId)
{
  global_rank_cnt  = cpu_rank_limit;
  global_group_cnt = cpu_rank_limit;
}

int job_interface::add_rank_zero( nex_cache_ptr& nptr, ulong max_cpu_ranks)
{
  std::cout << "Adding rank zero information, max_ranks = " << max_cpu_ranks << std::endl;
  rank_zero        = nptr;
  global_rank_cnt  = max_cpu_ranks;
  global_group_cnt = max_cpu_ranks;
  return 0;
}

int job_interface::add_nex_and_rank( bool is_local, nex_cache_ptr& nptr, std::string global_cpuid)
{
 //if is_local is true take both rank and nex and save it
 //otherwise, check if nex exists, if true do nothing
 //else, add nex, else add nex and rank
 auto rank = rank_desc(get_owner(), rank_desc::rank_types::CPU);
 //set global ID;
 rank.set_global_id( global_cpuid );
 //set rank zero
 //if( global_cpuid == "0" ) rank_zero = nptr;
 //find nexus entry
 auto registry_entry = std::find(rank_registry.begin(), rank_registry.end(), nptr); 
 if( registry_entry == rank_registry.end() )
 {
   //means nexus doesn't exists, add it along with rank
   //rank_registry.push_back(std::make_tuple(false, nptr, {} ));
   rank_registry.emplace_back(false, nptr, rank_list{} );
 }
 
 //only add the global_id if its the first nexus on the list
 //this prevents the rank to be added to multiple nex rank_descriptions
 if( is_local )
 {
   //rediscover nexus location
   //find job_interfaces based on nexus
   registry_entry  = std::find(rank_registry.begin(), rank_registry.end(), nptr); 
   //add local_nex_cache for convienence 
   local_nex_cache = std::get<job_details::REG_NEX>(*registry_entry);
   //get rank list
   rank_list& rankList = std::get<job_details::REG_RANKL>(*registry_entry); 

   auto itt = std::find(rankList.begin(), rankList.end(), 
                       std::make_pair( rank_desc::BYGID, global_cpuid ));

   //check if rank exists
   if( itt == rankList.end() )
   {
     //rank NOT found, add rank information
     rankList.push_back(rank);
   }
   else
   {
     //do nothing both nexus and rank exists
     std::cout << "Rank already discovered" << std::endl;
   }
  
 }
 
 return 0;
}

bool operator==(jd_registry_type& regptr, const nex_cache_ptr& nptr)
{
  bool results = (std::get<1>(regptr) ==  nptr);

  return results;
}  

bool operator==(job_interface_ptr& jptr, std::string& jobId)
{
  return (jptr->jobID == jobId);
}


int job_interface::load_convinient_locals(ulong global_rank)
{
  //load rank_0 nexus
  //get hostname from utilities and load local nexus
  //load local rank_list
  return 0;
}

template<typename ...T>
int job_interface::set_configfile(std::list<T>... args)
{
  auto add_to_config = [&](int idx,  std::list<std::string> config ){
            std::for_each(config.begin(), config.end(), 
                          [&](const std::string& entry){
                          //std::cout << "Adding config: " << entry << std::endl;
                          if ( idx == 0 )
                            this->config_file.add_entry(host_entry(entry));
                          else if ( idx == 1) 
                            this->config_file.add_entry(function_entry(entry));
                          else if ( idx == 2) 
                            this->config_file.add_entry(platform_entry(entry));
                         });
  }; 
  
  std::list<std::string> config_section[sizeof...(args)] = {args...};
  //add host section
  add_to_config(0, config_section[0]);  
  //add function section
  add_to_config(1, config_section[1]);  
  //add meta section
  add_to_config(2, config_section[2]);  
  
 
  return 0;
}

//TBD: Need to complete the rest of the format of payload for
//_res_nxinitjob TBD TBD TBD_
int job_interface::sync_job(std::string global_rank)
{
  //manages sending the request
  auto send_update_request = [&](jd_registry_type& entry)
  {
    zmq::multipart_t msg;
    nex_cache_ptr& nptr  = std::get<1>(entry);
    std::cout << "Forwarding sync request to ->" << nptr->host_name
              << "(" << get_hostname() << ")" <<std::endl;
    bool is_this_host = (get_hostname() == nptr->host_name);

    if( !is_this_host )
    {
       std::cout << "found matching host!" <<std::endl;
       zmsg_builder mbuilder(
                this->jobID, 
                (global_rank),
                std::string("__NEX_INITJOB__") );
       //add the indicator to reply to nexus update
       mbuilder.add_arbitrary_data<bool>(false)
               .add_sections<std::string>(this->config_file.get_section<host_entry>())
               .add_sections<std::string>(this->config_file.get_section<function_entry>())
               .add_sections<std::string>(this->config_file.get_section<platform_entry>())
               .finalize();
       
       //////////////////////////////////////////////////////////////
       //send the request to the target nexus
       nptr->send(mbuilder.get_zmsg());
    }

  };
  //process nexus's that have not been updated
  std::for_each(rank_registry.begin(), rank_registry.end(), send_update_request);

 return 0;
}

int job_interface::update_nexus(std::list<std::string> this_cache, std::string global_rank)
{
  //lambda to partition nex in two sets
  //not_updated vs. updated
  auto separate_updated_nex = [](const jd_registry_type entry)->bool
  {
    return std::get<0>(entry);
  };

  //manages sending the request
  auto send_update_request = [&](jd_registry_type& entry)
  {
    zmq::multipart_t msg;
    nex_cache_ptr& nptr  = std::get<1>(entry);
    bool &already_updated = std::get<0>(entry);
    bool is_this_host    = (get_hostname() == nptr->host_name);
    ulong size = nptr->as_is.size();

    if( !(already_updated || is_this_host) )
    {
       std::cout << "sharing nexus kernel support... % " << nptr->host_addr<< std::endl;  
       zmsg_builder mbuilder(
                     this->jobID, global_rank,
                     std::string("__NEX_UPDATECACHE__"));
       //add the indicator to reply to nexus update
       //with payload
       mbuilder.add_arbitrary_data<bool>(true)
               .add_arbitrary_data<std::string>(local_nex_cache->host_addr)
               .add_sections<std::string>(this_cache)
               .finalize();
       
       //////////////////////////////////////////////////////////////
       //send the request to the target nexus
       std::cout << "msg = " << mbuilder.get_zmsg() << std::endl;
       nptr->send(mbuilder.get_zmsg());
       //setting the flag to show that 
       already_updated = true;
    }
    else if( is_this_host )
    {
      //add data to lcoal cache
      local_nex_cache->add_cache_data( this_cache );

    }

  };

  //not updated nex iterator
  //the starting iterator to nexus not updated
  auto notUptNexIt = std::partition(rank_registry.begin(),
                                    rank_registry.end(),
                                    separate_updated_nex ); 

  //process nexus's that have not been updated
  std::for_each(notUptNexIt, rank_registry.end(), send_update_request);
  
  return 0;
}


int job_interface::_send_zmsg_to_nex(const std::string host_name, const zmq::multipart_t msg)
{
  bool sent = false;
  zmq::multipart_t mmsg = msg.clone(); 
  for( auto& rreg_entry : rank_registry )
  {
    auto& nex = std::get<job_details::REG_NEX>(rreg_entry);
    if( nex->host_name == host_name )
    {
      if( !sent )
      {
        nex->send(mmsg);
        sent = true;
      }
      else
      {
        std::cout << "Found multiple nexus with same hostname" <<std::endl;
      }
    }

  }

  return 0;
}

int job_interface::_send_zmsg_by_rank(const std::string rank_id, const zmq::multipart_t msg)
{
  bool sent = false;
  zmq::multipart_t mmsg = msg.clone(); 
  for( auto& rreg_entry : rank_registry )
  {
    auto& nex     = std::get<job_details::REG_NEX>(rreg_entry);
    auto& rank_li = std::get<job_details::REG_RANKL>(rreg_entry);
    auto rank     = std::find(rank_li.begin(), rank_li.end(),
                              std::make_pair(rank_desc::BYGID, rank_id) );

    if( rank != rank_li.cend() )
    {
      //found rank, now send message
      if( !sent ){
        nex->send(mmsg);
      }
      else{
        std::cout << "Duplicate Global Id Found" << std::endl;
      }
      sent = true;
    }

  }
  return 0;
}

int job_interface::_broadcast_rank( rank_desc& rd )
{

  //create request and send to all the nodes
  auto [ rank_desc, base_desc, func_desc ] = rd.serialize();
  //build messages
  zmsg_builder mbuilder( this->jobID, this->current_rank_id, 
                         std::string("__REM_RSYNC__") );

  mbuilder.add_sections(rank_desc)
          .add_sections(base_desc)
          .add_sections(func_desc)
          .finalize();
  _broadcast_zmsg( mbuilder.get_zmsg(), false );

  return 0;
}

int job_interface::_broadcast_zmsg( const zmq::multipart_t& msg, bool include_local)
{
  zmq::multipart_t mmsg = msg.clone(); 
  for( auto& rreg_entry : rank_registry )
  {
    auto& nex = std::get<job_details::REG_NEX>(rreg_entry);
    if( !nex->is_local() || include_local ) nex->send(mmsg);
    else std::cout << "Skipping local nex from broadcasting" << std::endl;
  }

  return 0;
}

int job_interface::allocate_global_ids( const ulong num_of_ids,
                                        ulong& group_id,
                                        std::list<ulong>& global_ids)
{
  for(int i=0; i < num_of_ids; i++) 
    global_ids.push_back(global_rank_cnt++);

  group_id = global_group_cnt++;

  return 0;
}

//return claiming system
std::unique_ptr<claim_subsys_base>&
job_interface::_get_recommd_sys(std::string strategy)
{
  return g_recommd_registry[strategy];
}

//send a request to get the global & groupID
//NEED TO MODIFY TO RETURN GROUP_ID as well TBD
int job_interface::_request_global_id(const ulong num_of_ids,
                                      ulong& group_id, 
                                      std::list<ulong>& global_ids)
{
  std::cout << " entering _request_global_id : " << global_ids.size() <<  std::endl;
  if( rank_zero && !(*rank_zero)->is_local() )
  {
    std::cout << " _request_global_id is not local" << std::endl;
    //create global ID Request
    zmsg_builder mbuilder(jobID, std::string(""), std::string("__NEX_GID_REQ__"));
    //build the rest of the message
    mbuilder.add_arbitrary_data<ulong>(num_of_ids).finalize();

    //send synchronous request for a global ID
    (*rank_zero)->sendrecv( mbuilder.get_zmsg() );
    //moved data to global ids  
    auto mviewer = mbuilder.get_viewer();
    group_id     = *mviewer.get_section<ulong>(0).begin();
    global_ids   = std::move(mviewer.get_section<ulong>(1));
  }
  else{
    //when it is a local request bypass sending a zmq request
    allocate_global_ids(num_of_ids, group_id, global_ids); 
  }

  return 0;
}

int job_interface::_transmit_claims(const std::list<rank_desc>& rd_list)
{
  //std::map -> std::list
  auto transform_attr = []( base_entry::attr_T attrs ) -> auto
  {
    std::list<std::string> ret;

    for( auto& attr : attrs )
      ret.push_back( std::string( attr.first + "=" + attr.second) );
 
    return ret;

  };

  //1) find nex to send request
  std::for_each( rd_list.begin(), rd_list.end(), [&]( rank_desc rank)
  {
    auto [nex_addr, pico_addr ] = rank.get_owner_pair();
    auto [group_id, global_id ] = rank.get_ggid();
    auto extra_params           = rank.func_details->get_attributes();
    auto funcId                 = rank.func_details->get_first_header_attr(vfh::SW_FID);
  
    std::cout << "nex_addr : " << nex_addr << std::endl;
    //get nex
    auto reg  = std::find( this->rank_registry.begin(), this->rank_registry.end(), 
                          std::make_pair(jd_search_types::FIND_NEX, nex_addr) );

    if( reg != rank_registry.end() )
    {
      auto nex  = std::get<job_details::REG_NEX>(*reg);
      //send data back into the local queues or
      //the remote queue
      //NEED TO TEST LOOP BACK
      zmsg_builder mbuilder( this->jobID, this->current_rank_id, 
                             std::string("__REM_CCLAIM__") );
      mbuilder.add_arbitrary_data( pico_addr )
              .add_arbitrary_data( group_id  )
              .add_arbitrary_data( global_id ) 
              .add_arbitrary_data( funcId ) 
              .add_sections( transform_attr( extra_params ) )
              .finalize();

      std::cout << "Transmitting claim : " << mbuilder.get_zmsg() << std::endl;
      nex->send( mbuilder.get_zmsg() );
    }
    else
    {
      std::cout << "Could not find registry entry" << std::endl;
    }

  } );
  
  return 0;
}


//TBD- registers the rank when the claims are finalized
//need claim Ids + global Id to register ranks
//across all included nexuses
int job_interface::_register_claims( std::list<rank_desc>& rd_list)
{

  //go through each rank and register them
  for ( auto& rank : rd_list )
  {
    auto [nex_addr, pico_addr ] = rank.get_owner_pair();
    //get nex
    auto reg   = std::find( rank_registry.begin(), rank_registry.end(), 
                             std::make_pair(jd_search_types::FIND_NEX, nex_addr) );

    auto& rankl = std::get<job_details::REG_RANKL>(*reg); 
    
    //setting phase to requested: remember to set 
    //rank as claimed when claim returns
    rank.set_phase( rank_desc::rank_phase::REQUESTED  );

    //TBD check whether the the rank exists
    rankl.emplace_back(rank);

    std::cout << "_broadcast_rank ... " << std::endl;
    //sending all the ranks
    _broadcast_rank( rank );

  }


  return 0;
}
//TBD - This function will take recommendations
//and make the claims to the nexus/pico
//id is a group_id match global id if its just a single request
int job_interface::make_claims(const std::list<function_entry>& fe_s, 
                               std::list<ulong>& group_ids )
{ 
  std::cout << "_entering make_claims" << std::endl;
  std::list<ulong> _group_ids;
  for( auto fe : fe_s )
  { 
    ulong group_id;
    ulong replicas           = fe.get_num_replicas();
    auto plat_cfg            = fe.get_last_header_attr(vfh::PLAT_CFG);
    std::string rec_strategy = config_file.get_platform_attr(plat_cfg, vph::REC_STRAT); //TBD

    //1) request global ids
    std::list<ulong> global_ids(replicas);
    global_ids.clear();
    _request_global_id(replicas, group_id, global_ids);
    //2) get recommendation engine
    auto& rec = _get_recommd_sys( rec_strategy );
    //3) initialize recommendation egine
    rec->init( &rank_registry );
    //////////////////////////////////////////////////////////////////
    //4) make a recommendation
    std::cout << "ctor rank_desc" << std::endl;
    rank_desc rd(get_owner(), rank_desc::ACCEL, fe); 
    std::list<rank_desc> rd_list(replicas, rd);
    std::cout << "make recommendation" << std::endl;
    rec->make_recommendation( rd_list );
    //////////////////////////////////////////////////////////////////
    //5) Update GroupID and GlobalID
    auto gIter = global_ids.begin();

    std::for_each(rd_list.begin(), rd_list.end(),[&](rank_desc& local_rd)
    {
      //update global and group ids and change state
      local_rd.update_globalId( group_id, *gIter);
     
      gIter = std::next( gIter );
    } );
    //////////////////////////////////////////////////////////////////////
    //6) Make the claims to the pico service(s)
    std::cout << "transmit_claim" << std::endl;
    _transmit_claims( rd_list );
    //////////////////////////////////////////////////////////////////////
    //7)Register Claims to all nexuses in the job
    std::cout << "_register_claim" << std::endl;
    _register_claims( rd_list ); 
    //////////////////////////////////////////////////////////////////////
    //add group id to the output
    std::cout << "group_ids.push_back" << std::endl;
    _group_ids.push_back(group_id);      
  } //end of function request

  //move group ids to output
  group_ids = std::move(_group_ids);

  return 0;
}


int job_interface::get_global_ids( ulong group_id, std::list<ulong>& global_ids)
{
  
  return 0;
}

int job_interface::get_rank_desc(ulong global_id,  rank_desc& rd)
{
  //TBD
  return 0;
}

int job_interface::get_primary_rt_interface(ulong global_id, std::list<rank_interface>& ri)
{
  return 0;
}


using lstr = std::list<std::string>;
template int job_interface::set_configfile(lstr, lstr, lstr);
