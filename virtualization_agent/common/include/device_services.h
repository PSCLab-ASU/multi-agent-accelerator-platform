#include "pico_utils.h"
#include "basic_services.h"
#include "shmem_store.h"
#include "device_manager.h"
#include "json_viewer.h"

#ifndef DEVSERVICE
#define DEVSERVICE

class device_services : public basic_services
{
  public:
 
    using basic_services::operator();

    device_services();

    void operator()( single_q &, single_q &, 
                     std::shared_ptr<shmem_store>& );
 
    basic_services::action_func action_ctrl( pico_utils::pico_ctrl ) final;

    void build_command_set();

  private:

    void _process_rpc ( decltype (device_manager().try_read() )&& );

    void _process_protos( std::map<std::string, std::string>, 
                         std::map<std::string, prototype_desc>& );

    std::vector<std::string> _get_kernel_list( std::string );

    void _conv_kdb( auto... inout_pairs )
    {
      auto _trans = []( auto& inout_pair )
      {
        auto&  in = inout_pair.first;         //map<str,str>
        auto& out = inout_pair.second.get();  //map<str,struct>
        std::for_each( std::begin( in ), std::end( in ), 
                       [&](auto in_entry)
        {
          auto input_map = json_viewer().get_object_lv0(in_entry.second); 
          sanitize_map( input_map);
          out.emplace( in_entry.first, input_map);
        });

      }; //end of _trans
      int dummy[sizeof...(inout_pairs)] = { (_trans(inout_pairs), 0)... };
    }  //end of 

    bool _parse_and_commit_func( const std::pair<std::string, std::string>&,
                                 const std::string&, const std::string&, 
                                 ulong&, std::vector<ulong>&, std::vector<ulong>&);
    
    std::map<pico_utils::pico_ctrl, basic_services::action_func >  command_set;
    
    std::pair<single_q, single_q> main_system_qs;
    std::unique_ptr<device_manager> dm;
    
    pico_return _register_resource_req( ulong );
    pico_return _identify_resource_req( ulong );
    pico_return _allocate_resource_req( ulong );
    pico_return _send_req( ulong );

};

#endif
