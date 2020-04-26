#include "kernel_intf.h"

template<typename T>
kernel_interface<T>::kernel_interface()
{
  _pkernel_db = std::make_unique<T>();
}

template<typename T>
ki_return 
kernel_interface<T>::create_entry( const std::string& dir,
                                   const std::string& filename, ulong& fid)
{
  auto ret = _pkernel_db->create_entry( dir, filename, fid);
  return ki_return{ret};
}

template<typename T>
ki_return 
kernel_interface<T>::add_kernel_info ( const ulong& fid,
                                       const kernel_hw_desc& khd,
                                       const kernel_sw_desc& ksd,
                                       const kernel_func_desc& kfd,
                                       ulong& func_id )
{
  auto ret = _pkernel_db->add_kernel_info(fid, khd, ksd, kfd, func_id);
  return ki_return{ret};
}

template<typename T>
ki_return
kernel_interface<T>::add_function_info( const ulong& fid,
                                        const ulong& func_id,
                                        const prototype_desc& proto_d)
{
  auto ret = _pkernel_db->add_function_info(fid, func_id, proto_d);
  return ki_return{ret};
}

template<typename T>
ki_return 
kernel_interface<T>::remove_kernel_file( const ulong& fid)
{
  auto ret = _pkernel_db->remove_kernel_file( fid );
  return ki_return{ret};
}
////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////
//Query section
//
template<typename T>
ki_return
kernel_interface<T>::get_all_clines( std::vector<std::string>& clines )
{
  ki_return ki;
  auto kern_file_ids  = _pkernel_db->get_kernel_file_ids();
  for( auto file_id : kern_file_ids)
  {
    ki = get_all_clines_infile( file_id, clines);
    if( ki ) std::cout << " Could not get clines for fid : "<< file_id << std::endl;
  }
  return ki_return{};
}

template<typename T>
ki_return
kernel_interface<T>::get_all_clines_infile( const ulong& fid, 
                                            std::vector<std::string>& clines)
{
  ki_return ki;
  bool func_exists = false;
  auto& kern_file  = _get_kernel_file( fid, ki);
  if( !ki )
  {
    auto func_ids = kern_file.get_func_ids();
    for(auto func_id : func_ids )
    {
      std::string cline;
      ki = get_kernel_cline( fid, func_id, cline);
      if( ki ) clines.push_back(cline); 
    } 
  }else { return ki;}
  
  return ki_return{};
}

template<typename T>
ki_return
kernel_interface<T>::get_kernel_cline( const ulong& fid, const ulong& func_id,
                                       std::string& cline )
{
  ki_return ki;
  bool func_exists = false;
  auto& kern_file  = _get_kernel_file( fid, ki);
  auto& kern_loc   = kern_file.get_location();
  if( !ki )
  { 
    auto[khd, ksd, kfd, kf] = kern_file.get_function(func_id, func_exists);
    if( func_exists ) cline = _generate_cline( kern_loc, khd, ksd, kfd );   
  }
  else { std::cout << "Cannot find function " << std::endl; }

  if( !func_exists ) std::cout << "func does not exists" << std::endl; 

  return ki;
}

template<typename T>
const kernel_file&
kernel_interface<T>::_get_kernel_file( const ulong& fid, ki_return& kr) const
{
  bool file_exists;
  auto& ret = _pkernel_db->get_kernel_file( fid , file_exists);
  kr = ki_return{!file_exists};
  return ret;
}

template<typename T>
std::string
kernel_interface<T>::_generate_cline( const std::pair<std::string, std::string>& loc, const kernel_hw_desc& khd, 
                                      const kernel_sw_desc& ksd, const kernel_func_desc& kfd)
{
  using namespace std;
  std::string cline;

  cline =  "path="  + loc.first  + ",";
  cline += "fname=" + loc.second + ",";
  cline += "id="    + khd.cache_like_fmt() + ":" 
                    + ksd.cache_like_fmt() + ":"
                    + kfd.cache_like_fmt() + ",";
  cline += "mac_addr=0:0:0:0:0:0,";
  cline += "xFunction=[xml]";
  return cline;
}


template kernel_interface<kernel_db>::kernel_interface();
template ki_return kernel_interface<kernel_db>::create_entry(const std::string &, const std::string &, ulong&);

template ki_return kernel_interface<kernel_db>::add_kernel_info ( const ulong&,
                                                                  const kernel_hw_desc&,
                                                                  const kernel_sw_desc&,
                                                                  const kernel_func_desc&,
                                                                  ulong& );

template ki_return kernel_interface<kernel_db>::add_function_info( const ulong&,
                                                                   const ulong&,
                                                                   const prototype_desc&);

template ki_return kernel_interface<kernel_db>::remove_kernel_file( const ulong&);

template ki_return kernel_interface<kernel_db>::get_all_clines( std::vector<std::string>& );

template ki_return kernel_interface<kernel_db>::get_all_clines_infile( const ulong&, 
                                                                       std::vector<std::string>& );

template ki_return kernel_interface<kernel_db>::get_kernel_cline( const ulong&, const ulong&,
                                                                  std::string& );

template const kernel_file& kernel_interface<kernel_db>::_get_kernel_file( const ulong&, ki_return& ) const;
