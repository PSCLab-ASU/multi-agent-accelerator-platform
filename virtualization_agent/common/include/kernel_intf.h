#include "base_utils.h"
#include "kernel_db.h"

#ifndef KERN_INTF
#define KERN_INTF

typedef pico_return ki_return;

template<typename T>
class kernel_interface
{
  public:
    kernel_interface();

    ki_return create_entry( const std::string& dir,
                            const std::string& filename, ulong& fid);

    ki_return add_kernel_info ( const ulong& fid,
                                const kernel_hw_desc& khd,
                                const kernel_sw_desc& ksd,
                                const kernel_func_desc& kfd,
                                ulong& func_id );

    ki_return add_function_info( const ulong& fid,
                                 const ulong& func_id,
                                 const prototype_desc& );
    
    ki_return remove_kernel_file( const ulong& fid);
    //get entire repo cache lines
    ki_return get_all_clines( std::vector<std::string>&);
    //get all cache lines from a single file
    ki_return get_all_clines_infile( const ulong&, std::vector<std::string>& );
    //get a single cache line from a paritcular file and func
    ki_return get_kernel_cline( const ulong&, const ulong&, std::string& );

  private:
    std::string _generate_cline( const std::pair<std::string, std::string>&, 
                                 const kernel_hw_desc&, const kernel_sw_desc&,
                                 const kernel_func_desc& );   

    const kernel_file& _get_kernel_file( const ulong&, ki_return&) const;

    std::atomic_bool _alock;

    //kernel index: id, absolute path
    std::unique_ptr<T> _pkernel_db;
};

using kernel_interface_kdb = kernel_interface<kernel_db>;

#endif
