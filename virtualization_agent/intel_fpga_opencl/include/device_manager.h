#include "utils.h" 
#include "base_device_manager.h"

#ifndef DEVMGR
#define DEVMGR
typedef unsigned char BYTE;

class device_manager : public base_device_manager
{

  public:

    using input_type  = base_device_manager::InputType;
    using header_type = base_device_manager::HeaderType;
    using lookup_type = base_device_manager::LookupType;
    using return_type = base_device_manager::CommandType::result_type;

    device_manager();

  private:

    return_type _default      ( header_type, input_type );
    return_type _matrix_mult  ( header_type, input_type );
    return_type _hello_world  ( header_type, input_type );
};

#endif
