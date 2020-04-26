#include "pico_utils.h"

#ifndef KERNTRACK
#define KERNTRACK

using cfg_ctor_t = std::map<std::string, std::string>;

struct prototype_desc
{
  prototype_desc(){}

  prototype_desc( const cfg_ctor_t );

  using parm_types = std::vector<kernel_function_t>; 
                              //named input param type, required/not
  using parameter = std::tuple<std::string, parm_types, bool>;
 
  cfg_ctor_t raw_data;
  std::vector<parameter> v_parm;
  

};

struct kernel_hw_desc
{
  kernel_hw_desc( ) {};
  kernel_hw_desc( const cfg_ctor_t );

  std::string vid; 
  std::string pid;
  std::string ss_vid;
  std::string ss_pid;
  
  std::string cache_like_fmt() const
  {
    return vid + ":" + pid + ":" + ss_vid + ":" + ss_pid;
  }

  cfg_ctor_t raw_data;
};

struct kernel_sw_desc
{
  kernel_sw_desc() {}
  kernel_sw_desc( const cfg_ctor_t );

  std::string sw_vid; 
  std::string sw_pid;

  cfg_ctor_t raw_data;

  std::string cache_like_fmt() const
  {
    return sw_vid + ":" + sw_pid;
  }

};

struct kernel_func_desc
{
  kernel_func_desc() {}
  kernel_func_desc( const cfg_ctor_t );

  std::string func_name;
  std::string sw_ClassId; 
  std::string sw_FuncId; 
  std::string sw_verId;

  struct reg_map
  {
    ulong size;
    std::optional<std::string> standard;

    reg_map(): size(0) {};

    reg_map( const cfg_ctor_t );

    reg_map( size_t sz) : size(sz) {};

    void set_size( size_t sz ) { size = sz; } 

    void set_default_values( std::list<std::string> vals );

    //stores key, desc, default value
    std::list<std::string> data;
    cfg_ctor_t raw_data;

  };

  std::string cache_like_fmt() const
  {
    return func_name + ":" + sw_ClassId + ":" + sw_FuncId + ":" + sw_verId;
  }

  void update_name( std::string fname ) { func_name = fname; }

  void update_maps( std::string acc_name, reg_map  m1, 
                    std::string sy_name,  reg_map  m2) { 
        accel_map.first  = acc_name; sys_map.first = sy_name; 
        accel_map.second = m1; sys_map.second = m2;
  } 
 
  std::pair<std::string, reg_map > accel_map;
  //sys map version , and map
  std::pair<std::string, reg_map > sys_map;

  cfg_ctor_t raw_data;
};

class kernel_function 
{
  public:
    kernel_function();

    int add_function_proto( const prototype_desc& proto_d);

  private:
    std::vector<prototype_desc> v_proto_desc;

};

class kernel_file
{
  public:
    using kFunc_entry = std::tuple<kernel_hw_desc, 
                                   kernel_sw_desc,
                                   kernel_func_desc,
                                   kernel_function>;

    kernel_file(const std::string&, const std::string&);

    int add_kernel_func_desc( const kernel_hw_desc&, 
                              const kernel_sw_desc&,
                              const kernel_func_desc&,
                              ulong&  func_id );

    int add_kernel_func_proto( const ulong& func_id,
                               const prototype_desc& p_desc);

    std::vector<ulong> get_func_ids() const;

    const std::pair<std::string, std::string>& get_location() const
    { return dir_filename; }

    const kFunc_entry& get_function( const ulong&, bool& ) const; 

    const std::map<ulong, kFunc_entry>& get_all_functions();

  private:
    std::map<ulong, kFunc_entry> _kernel_functions;
    ulong _func_ids;
    std::pair<std::string, std::string> dir_filename;
};


class kernel_db
{
  public:
    kernel_db();
    
    int create_entry( std::string dir, 
                      std::string filename, ulong& fid);

    int add_kernel_info ( const ulong& fid, 
                          const kernel_hw_desc& khd, 
                          const kernel_sw_desc& ksd, 
                          const kernel_func_desc& kfd,
                          ulong& func_id );

    int add_function_info( const ulong& fid, 
                           const ulong& func_id, 
                           const prototype_desc& );

    int remove_kernel_file( const ulong& fid);
    
    std::vector<ulong> get_kernel_file_ids() const;

    const kernel_file& get_kernel_file( const ulong&, bool&  ) const;

    const std::map<ulong, kernel_file>& get_kernel_files() const;
    
  private:

    //kernel index: id, absolute path
    std::map<ulong, kernel_file> _kernel_files;
    ulong _kf_cnt;
};


#endif
