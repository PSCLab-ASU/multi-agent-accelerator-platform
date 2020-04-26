#include "shmem_store.h"

shmem_store::shmem_store()
: _pico_cfg(      std::make_shared<pico_cfg>()                       ),
  _k_intf(        std::make_shared<kernel_interface<kernel_db> >()   ),
  _j_info(        std::make_shared<job_info>()                       ),
  _pico_pmsg_reg( std::make_shared<pico_pending_msg_registry>()      ),
  _pkt_gen(       std::make_shared<shmem_store::pkt_gen_ts>()        )
{}

