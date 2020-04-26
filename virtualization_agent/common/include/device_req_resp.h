#include "base_utils.h"

#ifndef DEVREQRESP_T
#define DEVREQRESP_T

class device_request_t : public base_req_resp
{
  public:
    device_request_t(ulong);
    device_request_t( device_request_t &&);
    device_request_t * operator->(){
      return this;
    }
  private:

};


class device_response_t : public base_req_resp
{
  public:
    using dev_payload_t = std::variant<dev_kernel_payload,
                                       std::string>;

    device_response_t( ulong );
    device_response_t( device_response_t &&);
    device_response_t * operator->(){
      return this;
    }

    void set_dev_data( dev_payload_t&& pl )
    {
      _dev_payload = pl;
    }  

    template<typename T>
    const T& get_dev_data() const
    {
      return std::get<T>( _dev_payload );
    }
 
  private:

    dev_payload_t  _dev_payload;

};

using device_request_ll  = device_request_t;
using device_response_ll = device_response_t;
#endif
