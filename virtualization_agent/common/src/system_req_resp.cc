#include "system_req_resp.h"

system_request_t::system_request_t(system_request_t && rhs)
: base_req_resp( rhs.get_self_tid() )
{
  

}

system_request_t::system_request_t(ulong self_id)
: base_req_resp( self_id )
{

}

system_response_t::system_response_t(ulong self_id)
: base_req_resp( self_id )
{

}

system_response_t::system_response_t( system_response_t && rhs)
: base_req_resp( rhs.get_self_tid() )
{


}

