#include <pico_utils.h>

void pico_utils::send_deleter( void * ) 
{
  std::cout << "Deleting send ptr ... " << std::endl;
}

pico_utils::pico_ctrl pico_utils::reverse_lookup( std::string val)
{
  auto pair = std::find_if( pico_utils::pico_rolodex.begin(),
                            pico_utils::pico_rolodex.end(),
                            [&](auto item)
                            {
                              return (val == item.second);
                            } );
  return pair->first;

}

zmsg_builder< std::string, std::string>
pico_utils::start_request_message( pico_utils::pico_ctrl method, std::string key)
{
  zmq::multipart_t empty;
  if( pico_rolodex.find ( method ) != pico_rolodex.end() )
  {
    std::string method_str = pico_rolodex.at( method );
    zmsg_builder<std::string, std::string>  zb( method_str, key);
    return std::move( zb );
  }
  else std::cout << "Invalid pico action..."<< std::endl;
  return std::move( zmsg_builder<std::string, std::string>(empty) );

}

