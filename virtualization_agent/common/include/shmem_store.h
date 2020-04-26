#include "pico_utils.h"
#include "kernel_intf.h"
#include "job_info.h"
#include "pico_cfg.h"
#include "pending_msgs.h"
#include "generator_template.h"
#include "system_req_resp.h"
#include "device_req_resp.h"


#ifndef SHMEMSTORE
#define SHMEMSTORE

enum struct item_key {
  PICO_CFG=0,
  KERNEL_INTF,
  JOB_INFO,
  PKT_GEN,
  PMSG
};

class shmem_store{

  public: 
    //static const pico_return pico_error;

    using item_token = std::atomic_bool;
    using pkt_gen_ts = pktgen_interface<device_request_ll,
                                        device_response_ll,
                                        system_request_ll,
                                        system_response_ll >;


    shmem_store();
  
    template <item_key item> auto& get_item()
    {
      if constexpr ( item == item_key::KERNEL_INTF )
        return _k_intf;
      else if constexpr ( item == item_key::JOB_INFO )
        return _j_info;
      else if constexpr ( item == item_key::PKT_GEN )
        return _pkt_gen;
      else if constexpr ( item == item_key::PICO_CFG )
        return _pico_cfg;
      else if constexpr ( item == item_key::PMSG )
        return _pico_pmsg_reg;
      else

        return (_p_error = pico_return{-1, "Item Failed to return"} );
   }
 
   template <item_key item> const auto& get_c_item()
   {
     return get_item<item>();
   }

  private:

    std::shared_ptr<pico_cfg>                     _pico_cfg;
    std::shared_ptr<kernel_interface<kernel_db> > _k_intf;
    std::shared_ptr<job_info>                     _j_info;
    std::shared_ptr<pico_pending_msg_registry>    _pico_pmsg_reg;
    std::shared_ptr<pkt_gen_ts >                  _pkt_gen;

    static pico_return _p_error;
};



#endif

