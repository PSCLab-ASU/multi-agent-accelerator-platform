#include <map>
#include <string>
#include <accel_utils.h>

#ifndef MPIACCELUTILS
#define MPIACCELUTILS

struct claim_info
{
  bool bactive;
  std::string _id;
  std::string _alias;
  std::map<std::string, std::string> _characteristic;

  claim_info() { bactive = false; };

  claim_info( std::string cid, std::string alias )
  { 
    bactive=false;  
    _alias = alias; 
    _id = cid; 
  }

  const std::string& get_id( ) const 
  { 
    return _id; 
  }

  void set_id( std::string id ) 
  { 
    _id = id; 
  }

  void set_alias( std::string alias) 
  { 
    _alias = alias; 
  }
  void set_metadata( std::map<std::string, std::string> md )
  { 
    _characteristic = md; 
  }

  void activate() 
  {
    bactive = true; 
  }

  void deactivate() 
  { 
    bactive = false; 
  }

  bool operator ==( std::string cid)
  {
    return (this->_id == cid);
  }
  
};

#endif

