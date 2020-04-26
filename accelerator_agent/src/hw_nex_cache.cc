#include <hw_nex_cache.h>
#include <utils.h>

/////////////////////////////////////////////////////////////////////////
////////////////////////nex_cache definition/////////////////////////////
hw_nex_cache::_hw_nex_cache(std::string rem_host, std::string host)
: nex_cache_data(rem_host, host)
{
   //remember to prefix identity with 'p2p' then connect 
   std::string this_host = get_hostname();
   sock.setsockopt(ZMQ_IDENTITY, this_host.c_str(), this_host.length());
   std::cout << "connecting to "<< conn_str() << std::endl;
   sock.connect(conn_str());
}

int hw_nex_cache::send(zmq::multipart_t& mp)
{
 std::cout << "Sending data: " << mp <<  std::endl;
 mp.send(sock);

 return 0;
}

int hw_nex_cache::recv(zmq::multipart_t& mp)
{
 mp.recv(sock);
 return 0;
}

int hw_nex_cache::sendrecv(zmq::multipart_t& mp)
{
   send( mp );
   recv( mp );
   return 0;
}

int hw_nex_cache::_add_header(zmq::multipart_t& mp)
{
 return 0;
}

const std::map<valid_function_header, std::string> 
        hw_nex_cache::transform_nexcache_view(std::string nex_cache) const
{
  std::map<valid_function_header, std::string> out;
  auto cache_parts = split_str<':'>(nex_cache);
  
  if( cache_parts.size() == NEX_CACHE_PARTS)
  {
    //hard mapping
    //cache_parts[0]  = pico_service_id 
    out[vfh::HW_VID]    = cache_parts[1];  
    out[vfh::HW_PID]    = cache_parts[2];  
    out[vfh::HW_SS_VID] = cache_parts[3];  
    out[vfh::HW_SS_PID] = cache_parts[4];  
    out[vfh::SW_VID]    = cache_parts[5];  
    out[vfh::SW_PID]    = cache_parts[6];  
    out[vfh::SW_FID]    = cache_parts[7];  
    out[vfh::SW_VERID]  = cache_parts[8];
    //cache_parts[9] hw_state
    //cache_parts[10] num_devices  
  }
  else
  {
    std::cout << "Invalid Cache length: "<< cache_parts.size() << std::endl;
  }
  
  return out;
};

bool hw_nex_cache::contains_function( const function_entry& fe) const
{
  const std::string base_attrs ="";
  auto clines = get_raw_cachelines();
  return std::any_of(clines.begin(), clines.end(),[&](std::string cache_line ) 
         { 
           auto trans = transform_nexcache_view(cache_line);
           function_entry cache_fe(base_attrs, trans );
           return fe.contains_kv(cache_fe); 
         } );

}

const std::list<std::string>
hw_nex_cache::get_pico_service_list(const function_entry& fe) const
{
  const std::string base_attrs ="";
  auto clines = get_raw_cachelines();
  std::list<std::string> temp;
  
  for(auto cline : clines)
  {
    auto cache_parts = split_str<':'>(cline);
    auto trans = transform_nexcache_view(cline);
    function_entry cache_fe(base_attrs, trans );
    if( fe.contains_kv(cache_fe) ) 
      temp.push_back(cache_parts[0]);
  }
  return temp; 
}
/////////////////////////////////////////////////////////////////////////
/////////////////////nex_cache_data definition///////////////////////////
nex_cache_data::_nex_cache_data(std::string rem_host, std::string host) 
: ctx(zmq::context_t()), sock(ctx, ZMQ_DEALER), host_name(rem_host), host_addr(host), bActive(false)
{

}

std::string nex_cache_data::conn_str()
{
  return std::string("tcp://") + host_addr;
}

void nex_cache_data::add_cache_data(std::list<std::string> cache_lines )
{
  as_is = cache_lines;
  //contiue to breakdown the cache entries
  //TBD....
}

const std::list<std::string>& nex_cache_data::get_raw_cachelines() const
{
  return as_is;
}

/////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////

bool operator==(nex_cache_ptr& nptr, std::string& hostname)
{
  return (nptr->host_addr == hostname);
} 

bool operator==(nex_cache_ptr& lhs, nex_cache_ptr& rhs)
{
  return (lhs->host_addr == rhs->host_addr);
}
 
