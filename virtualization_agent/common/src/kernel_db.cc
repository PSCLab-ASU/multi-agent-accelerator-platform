#include "kernel_db.h" 
#include <utility>
#include <tuple>
#include "json_viewer.h"


kernel_hw_desc::kernel_hw_desc( const cfg_ctor_t str_map)
{ 
  std::cout << "TBD: constructing " << __func__ << std::endl; 
  auto set_var = [&]( const std::string key, std::string& var)
  {
    try { var = str_map.at(key); } catch(std::out_of_range const& exc){ var = ""; };
  };

  set_var("vid", vid);
  set_var("pid", pid);
  set_var("ss_vid", ss_vid);
  set_var("ss_pid", ss_pid);
  raw_data = str_map;
}

kernel_sw_desc::kernel_sw_desc(const cfg_ctor_t str_map)
{ 
  std::cout << "TBD: constructing " << __func__ << std::endl; 
  auto set_var = [&]( const std::string key, std::string& var)
  {
    try { var = str_map.at(key); } catch(std::out_of_range const& exc){ var = ""; };
  };

  set_var("sw_vid", sw_vid);
  set_var("sw_pid", sw_pid);
  raw_data = str_map;
}

kernel_func_desc::kernel_func_desc(const cfg_ctor_t str_map)
{ 
  std::cout << "TBD: constructing " << __func__ << std::endl; 
  auto set_var = [&]( const std::string key, std::string& var)
  {
    try { var = str_map.at(key); } catch(std::out_of_range const& exc){ var = ""; };
  };

  set_var("sw_vid",    func_name);
  set_var("classId",   sw_ClassId);
  set_var("funcId",    sw_FuncId);
  set_var("verId",     sw_verId);
  set_var("accel_map", accel_map.first);
  set_var("sys_map",   sys_map.first);

  raw_data = str_map;
}

prototype_desc::prototype_desc(const cfg_ctor_t str_map)
{
  std::cout << "TBD: constructing " << __func__ << std::endl;

  std::string parm_name, parm_type, parm_req;
  std::for_each( std::begin(str_map), std::end(str_map), 
                 [&] (auto parm ) 
  {
    //go through each param
    auto jmap = json_viewer().get_object_lv0( parm.second );

    auto set_var = [&]( const std::string key, std::string& var)
    {
      try { var = jmap.at(key); } catch(std::out_of_range const& exc){ var = ""; };
    };
    
    set_var("dtype",    parm_type);
    set_var("required", parm_req);
    //list of types
    auto types = json_viewer().get_array(parm_type); 
    std::vector<kernel_function_t> v_kft;

    //transform from string to type
    std::for_each( std::begin(types), std::end(types), 
                   [&](auto type )
    { 
        kernel_function_t kft;   
        try { kft = g_zst_map.at(type); } 
        catch(std::out_of_range const& exc){ kft = kernel_function_t::NONE; }
        v_kft.push_back( kft );

    } );

    v_parm.emplace_back(parm.first, v_kft, (parm_req=="true") );
  });
   
  raw_data = str_map;

}

kernel_func_desc::reg_map::reg_map(const cfg_ctor_t str_map)
{
 std::cout << "TBD: constructing " << __func__ << std::endl; 
 auto set_var = [&]( const std::string key, std::string& var)
 {
   try { var = str_map.at(key); } catch(std::out_of_range const& exc){ var = ""; };
 };
 std::string std, sdata, ssize;

 set_var("std", std);
 set_var("size", ssize);
 set_var("data", sdata);

 if( !std.empty() ) standard = std;

 size = std::stoul( ssize );

 auto v_data = json_viewer().get_array(sdata); 
 set_default_values( v_data );

 raw_data = str_map;
}

void kernel_func_desc::reg_map::set_default_values( std::list<std::string> vals )
{
  data = vals;
}
///////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////Kernel Function Section /////////////////////////////
kernel_function::kernel_function(){}

int kernel_function::add_function_proto(const prototype_desc& proto_d)
{
  v_proto_desc.push_back(proto_d);
}

///////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////Kernel File Section /////////////////////////////////
kernel_file::kernel_file(const std::string& dir, const std::string& filename ) 
: _func_ids(0), dir_filename(dir, filename) {}

int kernel_file::add_kernel_func_desc( const kernel_hw_desc& khd,
                                       const kernel_sw_desc& ksd,
                                       const kernel_func_desc& kfd,
                                       ulong&  func_id )
{
  kernel_file::kFunc_entry desc = std::make_tuple(khd, ksd, kfd, kernel_function() );

  std::pair<ulong, kernel_file::kFunc_entry> item{_func_ids, desc };

  _kernel_functions.insert( item );

  func_id = _func_ids;
  _func_ids++;
  return 0;
}

int kernel_file::add_kernel_func_proto( const ulong& func_id,
                                        const prototype_desc& p_desc)
{
  try
  {
    std::get<kernel_function>(_kernel_functions.at(func_id)).
                                add_function_proto( p_desc );
  }
  catch(...) { std::cout << " Function id: " << func_id << " does not exists" << std::endl; return -1; }

  return 0;
}

std::vector<ulong> kernel_file::get_func_ids() const
{
  std::vector<ulong> out;
  for( auto func : _kernel_functions) out.push_back(func.first); 
 
  return out;
}

const kernel_file::kFunc_entry& 
kernel_file::get_function( const ulong& func_id, bool& data_exists ) const
{
  data_exists = false;

  try 
  { 
    auto& kf = _kernel_functions.at(func_id); 
    data_exists = true; 
    return kf;
  } 
  catch(...)
  { 
    data_exists = false; 
    perror("Error: could not get_function"); 
  }
}

const std::map<ulong, kernel_file::kFunc_entry>& kernel_file::get_all_functions()
{
  return _kernel_functions;
}


///////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////

kernel_db::kernel_db() : _kf_cnt(0)  {}

int kernel_db::create_entry( std::string dir,
                             std::string filename, ulong& fid)
{
   _kernel_files.emplace( _kf_cnt, kernel_file(dir, filename) );

   fid = _kf_cnt;

   _kf_cnt++;
   return 0;
}

int kernel_db::add_kernel_info ( const ulong& fid,
                                 const kernel_hw_desc& khd,
                                 const kernel_sw_desc& ksd,
                                 const kernel_func_desc& kfd,
                                 ulong& func_id )
{

  try{
    _kernel_files.at(fid).add_kernel_func_desc(khd, ksd, kfd, func_id);

  } catch(...){ std::cout << " File id: " << fid << " does not exists" << std::endl; return -1; } 
  
  return 0;
}

int kernel_db::add_function_info( const ulong& fid,
                                  const ulong& func_id,
                                  const prototype_desc& proto_d)
{
  try{
    _kernel_files.at(fid).add_kernel_func_proto( func_id, proto_d);

  } catch(...){ std::cout << " File id: " << fid << " does not exists" << std::endl; return -1; } 

  return 0;
}

int kernel_db::remove_kernel_file( const ulong& fid)
{
   _kernel_files.erase(fid);

  return 0;
}

const kernel_file& kernel_db::get_kernel_file( const ulong& fid, bool& data_exists ) const
{
  data_exists = false;

  try 
  { 
    auto& kf = _kernel_files.at(fid); 
    data_exists = true; 
    return kf;
  } 
  catch(...)
  {
    data_exists = false; 
    perror("Could not find kernel file");
  }

  
}

const std::map<ulong, kernel_file>& kernel_db::get_kernel_files() const
{
  return _kernel_files;
}

std::vector<ulong> kernel_db::get_kernel_file_ids() const
{
  std::vector<ulong> out;
  for( auto k_file : _kernel_files ) out.push_back( k_file.first );
  return out;
}


