#include "pico_utils.h"
#include "basic_services.h"

#ifndef SYSSERVICE
#define SYSSERVICE

class system_services : public basic_services
{
  public: 
   
    using basic_services::operator();

    system_services();

    void operator()(q_arry &, q_arry &, 
                    std::shared_ptr<shmem_store>& ); 

    void build_command_set(); 

    basic_services::action_func action_ctrl( pico_utils::pico_ctrl ) final;

  private:
    
    std::shared_ptr<system_request_ll>&  get_system_request ( const ulong&, pico_return& );
    std::shared_ptr<device_response_ll>& get_device_response( const ulong&, pico_return& );

    std::map<pico_utils::pico_ctrl, 
             std::pair<basic_services::action_func,
                       basic_services::action_func>  > command_set;

    std::pair<single_q, single_q > main_system_qs;

    std::pair<single_q, single_q > main_device_qs;
    //////////////////////////////////////////////
    //COMMAND SET
    //////////////////////////////////////////////
    pico_return _register_resource_req( ulong );
    pico_return _register_resource_resp( ulong );
    //////////////////////////////////////////////
    pico_return _identify_resource_req( ulong );
    pico_return _identify_resource_resp( ulong );
    //////////////////////////////////////////////
    pico_return _allocate_resource_req( ulong );
    pico_return _allocate_resource_resp( ulong );
    //////////////////////////////////////////////
    pico_return _send_req( ulong );
    pico_return _send_resp( ulong );
    //////////////////////////////////////////////

};



#endif
