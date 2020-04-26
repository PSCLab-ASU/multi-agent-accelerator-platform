#include "device_req_resp.h"

device_request_t::device_request_t( device_request_t && rhs)
: base_req_resp( rhs.get_self_tid() )
{


}

device_request_t::device_request_t(ulong self_tid)
: base_req_resp(self_tid)
{

}


device_response_t::device_response_t(ulong self_tid)
: base_req_resp(self_tid)
{

}

device_response_t::device_response_t( device_response_t && rhs)
: base_req_resp( rhs.get_self_tid() )
{


}
