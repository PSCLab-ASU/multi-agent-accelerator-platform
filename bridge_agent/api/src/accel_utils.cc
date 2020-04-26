#include <accel_utils.h>

std::optional<accel_utils::accel_ctrl>
accel_utils::reverse_lookup( std::string val )
{

  auto pair = std::find_if( accel_rolodex.begin(), accel_rolodex.end(),
               [&]( auto item ) 
               {
                 return (val == item.second);
               } );
  if( pair == accel_rolodex.end() ) return {};
  else return pair->first; 
}

zmsg_builder<std::string, std::string, std::string>
accel_utils::start_request_message( accel_utils::accel_ctrl method, std::string key)
{
  zmq::multipart_t empty;
  if( accel_rolodex.find ( method ) != accel_rolodex.end() )
  {
    std::string method_str = accel_rolodex.at( method );
    zmsg_builder<std::string, std::string, std::string>  zb( method_str, PLACEHOLDER, key);
    return std::move( zb );
  }
  else std::cout << "Invalid accel action..."<< std::endl;  
  return std::move( zmsg_builder<std::string, std::string, std::string>(empty) );

}


zmsg_builder<std::string, std::string, std::string, std::string>
accel_utils::start_routed_message( accel_utils::accel_ctrl method, std::string address, std::string key)
{
  zmq::multipart_t empty;
  if( accel_rolodex.find ( method ) != accel_rolodex.end() )
  {
    std::string method_str = accel_rolodex.at( method );
    zmsg_builder<std::string, std::string, std::string , std::string>  zb( address, method_str, PLACEHOLDER, key);
    return std::move( zb );
  }
  else std::cout << "Invalid accel action..."<< std::endl;  
  return std::move( zmsg_builder<std::string, std::string, std::string, std::string>(empty) );

}

