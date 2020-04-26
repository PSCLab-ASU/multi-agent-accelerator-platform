#include <string>
#include <map>

#ifndef RESLOGIC
#define RESLOGIC

struct resource_logic
{
  resource_logic( ) : resource("sw_fid=default") {
    set_rsign( resource );
  };
  resource_logic( std::string res );

  void set_rsign (std::string );
  bool operator== ( const resource_logic & rhs ) const;
  bool operator< ( const resource_logic & rhs ) const;
  
  std::string resource;
  std::map<std::string, std::string> resource_desc;

};

#endif
