#include "base_utils.h"

#ifndef SYSREQRESP_T
#define SYSREQRESP_T


class system_request_t : public base_req_resp
{
  public:
    system_request_t( ulong ); 
    system_request_t( system_request_t&&);
    system_request_t* operator->(){
      return this;
    }

  private:

};


class system_response_t : public base_req_resp
{
  public:
    //ulong equals one for response
    system_response_t( ulong );
    system_response_t( system_response_t&&);
    system_response_t* operator->(){
      return this;
    }

  private:
  
};

//using system_request_ll  = std::list<system_request_t>;
//using system_response_ll = std::list<system_response_t>;

using system_request_ll  = system_request_t;
using system_response_ll = system_response_t;
#endif

