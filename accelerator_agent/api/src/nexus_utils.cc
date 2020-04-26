#include <nexus_utils.h>

nexus_utils::nexus_ctrl nexus_utils::reverse_lookup( std::string val )
{
 auto pair = std::find_if( nexus_utils::nexus_rolodex.begin(), 
                           nexus_utils::nexus_rolodex.end(), 
                           [&](auto item)
                           {
                             return (val == item.second);
                           } );
 return pair->first;
}

zmsg_builder<std::string, std::string>
nexus_utils::start_request_message( nexus_utils::nexus_ctrl method, std::string key)
{
  zmq::multipart_t empty;
  if( nexus_rolodex.find ( method ) != nexus_rolodex.end() )
  {
    std::string method_str = nexus_rolodex.at( method );
    zmsg_builder<std::string, std::string>  zb( method_str, key);
    return std::move( zb );
  }
  else std::cout << "Invalid nexus action..."<< std::endl;  
  return std::move( zmsg_builder<std::string, std::string>(empty) );

}

zmsg_builder<std::string, std::string, std::string>
nexus_utils::start_routed_message( nexus_utils::nexus_ctrl method, std::string address, std::string key)
{
  zmq::multipart_t empty;
  if( nexus_rolodex.find ( method ) != nexus_rolodex.end() )
  {
    std::string method_str = nexus_rolodex.at( method );
    zmsg_builder<std::string, std::string, std::string>  zb( address, method_str, key);
    return std::move( zb );
  }
  else std::cout << "Invalid nexus action..."<< std::endl;  
  return std::move( zmsg_builder<std::string, std::string, std::string>(empty) );

}
