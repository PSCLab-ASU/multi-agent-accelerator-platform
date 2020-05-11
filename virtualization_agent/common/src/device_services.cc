#include "device_services.h"
#include "device_manager.h"

device_services::device_services()
: basic_services()
{
  dm = std::make_unique<device_manager>();
  dm->enumerate_devices();
}

void device_services::build_command_set()
{
  using ctrl = pico_utils::pico_ctrl;

  auto bind_action = [&](auto func )
  {
    return std::bind(func, this, std::placeholders::_1);
  };
  
  command_set[ctrl::reg_resource]   = bind_action(&device_services::_register_resource_req);
  command_set[ctrl::ident_resource] = bind_action(&device_services::_identify_resource_req);
  command_set[ctrl::alloc_resource] = bind_action(&device_services::_allocate_resource_req);
  command_set[ctrl::send]           = bind_action(&device_services::_send_req);

}

void device_services::_process_protos( std::map<std::string, std::string> in_map, 
                                      std::map<std::string, prototype_desc>& out_map )
{
  std::for_each( std::begin(in_map), std::end(in_map), 
                [&]( auto entry )
  {
    auto jmap = json_viewer().get_object_lv0( entry.second);
    out_map.emplace(entry.first, jmap);
  });
}

/////////////////////////////////////////////////////////////////////////////////////////////
bool device_services::_parse_and_commit_func( const std::pair<std::string, std::string>& dir_filename,
                                              const std::string& globals, const std::string& func_definition,
                                              ulong& fid, std::vector<ulong>& func_ids, std::vector<ulong>& num_devices)
{
  std::cout << "calling dev-function " << __func__ << std::endl;
  ulong func_id;
  json_viewer hwsw_desc   ("hwsw_descriptions");
  json_viewer regmap_desc ("register_maps");
  auto hwsw_section     = hwsw_desc.get_object(globals);
  auto rmap_section     = regmap_desc.get_object(globals);
  //produces maps for each json in the index
  auto hw_section = json_viewer().get_object_lv0(hwsw_section.at("hw_desc") );
  auto sw_section = json_viewer().get_object_lv0(hwsw_section.at("sw_desc") );
  
  auto amaps      = json_viewer().get_object_lv0( rmap_section.at("accel_maps") );
  auto smaps      = json_viewer().get_object_lv0( rmap_section.at("sys_maps") );
 
  auto funcs      = json_viewer("function_descs").get_object(func_definition);
  auto protos     = json_viewer("prototypes").get_object(func_definition);

  //construct each hw_section
  std::map<std::string, kernel_hw_desc> khd;
  //software description
  std::map<std::string, kernel_sw_desc> ksd;
  //function
  std::map<std::string, kernel_func_desc> kfd;
  //prototypes
  std::map<std::string, prototype_desc> kpd;
  //accelerator maps
  std::map<std::string, kernel_func_desc::reg_map > kfd_amap, kfd_symap;
  /////////////////////////////////////////////////////////////////////////////////////////////////
  //transform strings to structs
  using namespace std;
  _conv_kdb(pair(hw_section, ref(khd)),  pair(sw_section, ref(ksd)), pair(funcs, ref(kfd) ),
            pair(amaps, ref(kfd_amap)), pair(smaps, ref(kfd_symap)) ); 
  //special case because protos can't be sanitized
  _process_protos( protos, kpd);
  /////////////////////////////////////////////////////////////////////////////////////////////////
  //Go through each function call database interface
  //create entry
  bool err = true;
  err = get_kintf()->create_entry( dir_filename.first, dir_filename.second, fid);
  if ( err ) return false;

  for( auto func : funcs )
  {
    err = false;
    std::string function_name = func.first;
    auto fce = json_viewer().get_object_lv0(func.second);
    sanitize_map( fce );
    std::string hwd_ind = "hw_desc", swd_ind="sw_desc", cl_ind="classId",
                func_ind = "funcId", ver_ind="verId", acc_ind="accel_map", sy_ind="sys_map";
 
    //update function name
    kfd.at(function_name).update_name( function_name);
    //update reg_maps
    kfd.at(function_name).update_maps(fce.at(acc_ind), kfd_amap.at( fce.at(acc_ind) ), 
                                      fce.at(sy_ind), kfd_symap.at( fce.at(sy_ind) ) );
    //create kernel info   
    err |= get_kintf()->add_kernel_info( fid, khd.at(fce.at(hwd_ind) ), 
                                       ksd.at(fce.at(swd_ind) ), 
                                       kfd.at(function_name), func_id);
    //add function details
    err |= get_kintf()->add_function_info( fid, func_id, kpd.at(function_name) );

    std::string pcie_lookup = khd.at( fce.at(hwd_ind) ).cache_like_fmt();
    auto supp_dev_cnt = get_pci_devices( pcie_lookup ).size(); 
   
    if( !err ) 
    {
      func_ids.push_back(func_id);
      num_devices.push_back(supp_dev_cnt);
    }
    else
    {
      get_kintf()->remove_kernel_file( fid );
    }   
  }
  if( func_ids.empty() || num_devices.empty() )
    return false;  // do NOT return entry to system
  else
    return true; //return entry to system
} 

std::vector<std::string> device_services::_get_kernel_list( std::string repo_dir )
{
  std::vector<std::string> temp;
  std::vector<std::string> filts = dm->get_valid_file_exts();
  
  std::string ls_command  = std::string("ls ") + repo_dir + std::string("*");
  std::string resp   = exec(ls_command.c_str());

  auto files = split_str<'\n'>(resp);

  std::remove_if( std::begin(files), std::end(files), [&]( const std::string& file )
  {
     bool not_remove = std::none_of( std::begin(filts), std::end(filts), 
                                 [&]( const std::string& filter)
                   {
                     return ( file.find( filter ) == std::string::npos );
                   });

    if( !not_remove ) std::cout << "Removing " << file << std::endl;
    return !not_remove;
  } );

  return files;
}

/////////////////////////////////////////////////////////////////////////////////////////////
basic_services::action_func device_services::action_ctrl(pico_utils::pico_ctrl method)
{
  
  return command_set.at(method);
}

void device_services::operator()(single_q& q1, single_q& q2, 
                                 std::shared_ptr<shmem_store>& _sh_store)
{ 
  std::cout << "calling dev-function " << __func__ << std::endl;
  main_system_qs = std::make_pair(q1, q2);

  //WARNING: do not use the .first item to
  //         ONLY use .second
  set_io_queues ( main_system_qs );
  
  set_shmem_store( _sh_store );

  build_command_set();

  while(true)
  {
    //consume request from system service
    main_system_qs.first->consume_one(*this);
    //check device manager for results
    _process_rpc( std::move( dm->try_read() ) );
    //heart beat device manager
    dm->process_single();
    //check shutdown requests
    if ( is_shutdown() )
    {
      //consume all requests
      main_system_qs.first->consume_all(*this);
      //wait until all pending requests from DM and devices services are completed
      //TBD TRACK OUTSTANDING REQUEST
      while( dm->any_execution_remaining() ) 
      {
        //check device manager for results
        _process_rpc( std::move( dm->try_read() ) );
        //heart beat device manager
        dm->process_single();
      }
      break;
    }

  }
  std::cout << "Shutting down device_services" << std::endl;
}

//process rpc results
void device_services::_process_rpc( decltype (device_manager().try_read() ) && rpc_results )
{
  pico_return pret;

  if( rpc_results )
  {
    std::cout << "Got results from device manager ... " << std::endl;

    auto r_payload = std::move( rpc_results.value() );
    auto dev_resp = get_entry<device_response_ll>( r_payload.get_tid(), pret);

    dev_resp->set_data( std::move(r_payload) );
    send_upstream( dev_resp->get_self_tid() );
   
  }
}

///////////////////////////////////////////////////////////////////////////
////////////////////COMMMAND SET///////////////////////////////////////////
pico_return device_services::_send_req( ulong tid )
{
  std::cout << "calling dev-function " << __func__ << std::endl;
  bool data_exists = false;
  //link the request to the response so that
  //the chain wont deallocate
  auto& dev_resp = get_pktgen()->opt_generate<ulong, device_response_ll>(tid);
  //get a reference to the payload
  auto& payload  = get_send_payload( tid, data_exists);

  std::cout << "resource request for " << payload.get_resource () << std::endl;

  //submit it to the device manager via const reference
  //TBD need to move get_self_tid() to resource_logic!!!! TBD
  dm->submit( {dev_resp->get_self_tid(), payload} ); 

  return pico_return{};
}

pico_return device_services::_register_resource_req( ulong tid )
{
  std::cout << "calling dev-function " << __func__ << std::endl;
  auto& dev_resp = get_pktgen()->opt_generate<ulong, device_response_ll>(tid);
  std::vector<std::string> dev_desc;
  //retrieve filter settings
  auto manifest =  dm->get_manifest();
  std::string ext_addr = get_pcfg()->get_external_address();
  manifest.push_front( ext_addr );
  //construct registration data
  std::vector<std::string> v_cache;
  v_cache.insert( v_cache.begin(), manifest.begin(), manifest.end() );

  misc_string_payload data( v_cache );

  dev_resp->set_data( data );
  std::cout << "Fetching Id..." << tid << std::endl;
  auto id = dev_resp->get_self_tid();
  std::cout << "Sending data upstream..." << std::endl;
  send_upstream( dev_resp->get_self_tid() );
  return pico_return{};
}

pico_return device_services::_identify_resource_req( ulong tid )
{
  std::cout << "calling dev-function " << __func__ << std::endl;
  bool data_exists = false;
  std::string globals, function_definitions;
  ulong file_id;
  std::vector<ulong> func_ids, num_devices;
  dev_kernel_payload dkp;

  auto& payload  = get_idn_payload( tid, data_exists);
  
  if( data_exists )
  {
    auto& dev_resp = get_pktgen()->opt_generate<ulong, device_response_ll>(tid);

    for( auto dir : payload.get_data() )
    {
      auto files = _get_kernel_list(dir); 

      //go through each file
      for( auto filename : files )
      {
         if( !filename.empty() )
         {
           std::pair<std::string, std::string> dir_filename = { dir, filename };
           //read meta data from file
           dm->get_file_metadata( dir_filename, globals, function_definitions);
           //parse json string and commit to kernel repo
           bool save = _parse_and_commit_func( dir_filename, globals, function_definitions,  //inbound vars
                                               file_id, func_ids, num_devices ); //out bound vars

           if( save ) dkp.emplace_back(file_id, func_ids, num_devices );
           //reset variables
           func_ids.clear();
           num_devices.clear();
         }
       }      
    }
    
    dev_resp->set_dev_data( std::move(dkp) );   
    send_upstream( dev_resp->get_self_tid() );

  }

  return pico_return{};
}

pico_return device_services::_allocate_resource_req( ulong tid )
{

  std::cout << "calling dev-function " << __func__ << std::endl;
  return pico_return{};
}
