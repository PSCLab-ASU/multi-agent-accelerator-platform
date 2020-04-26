#include <json.h>
#include <iostream>
#include <list>

#ifndef JSONVIEWER
#define JSONVIEWER


struct json_viewer {
  std::string key; //used for lookup

  json_viewer() {}

  json_viewer(std::string lkup) : key(lkup) {}
  //////////////////////////////////////////////
  //update key
  json_viewer set_key( std::string lkup )
  { this->key = lkup; return *this; };
  //////////////////////////////////////////////
  //used for visitor function
  std::string operator()(std::string inp){ 
    std::string err;
    auto jconfig = json::Json::parse(inp, err);
    return jconfig[key].string_value();
  }
  /////////////////////////////////////////////////////
  //return a list of jason
  std::list< std::string > get_array(std::string inp){
    std::string err;
    std::list< std::string> temp;
    auto jconfig = json::Json::parse(inp, err);
    if( key.empty() )
      for(auto& attr : jconfig.array_items() )
         temp.push_back(attr.dump() );
    else if( jconfig[key].is_array() )
      for(auto& attr : jconfig[key].array_items() )
         temp.push_back(attr.string_value() );
    else
      temp.push_back( jconfig[key].string_value() );

    return temp;
  }
  ////////////////////////////////////////////////////////////////
  //return a list of json
  std::map<std::string, std::string > get_object(std::string inp){
    std::string err;
    std::map<std::string, std::string> temp;
    auto jconfig = json::Json::parse(inp, err);
    if( jconfig[key].is_object() )
      for(auto& attr_pair : jconfig[key].object_items() )
      { 
         temp.emplace( attr_pair.first, attr_pair.second.dump() );
      }
    return temp;
  }
  ///////////////////////////////////////////////////////////////////////////
  ////////////////////////////////////////////////////////////////
  //return a list of json
  std::map<std::string, std::string > get_object_lv0(std::string inp){
    std::string err;
    std::map<std::string, std::string> temp;
    auto jconfig = json::Json::parse(inp, err);
      for(auto& attr_pair : jconfig.object_items() )
      {  
         temp.emplace( attr_pair.first, attr_pair.second.dump() );
      }
    return temp;
  }

};

#endif
