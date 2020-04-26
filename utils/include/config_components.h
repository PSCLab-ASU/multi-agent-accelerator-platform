#include <string>
#include <variant>
#include <list>
#include <map>
#include <algorithm>
#include <json_viewer.h>
#include <type_traits>
#include <iostream>
#include <utils.h>
#include <boost/range/algorithm/transform.hpp>

#ifndef CONFIGCOMPONENTS 
#define CONFIGCOMPONENTS

#define CFG_WILDCARD "*"
//////////////////////////////////////////////////////////////////
enum struct valid_platform_header{
 PLAT_CFG, REC_STRAT, VAR3, NO_PARMS
};

using vph = valid_platform_header;
const std::map<valid_platform_header, std::string> g_plat_map {
  {vph::PLAT_CFG,  "platform_id"              },
  {vph::REC_STRAT, "recommendation_strategy" },
  {vph::VAR3,      "var3"                    }
};
///////////////////////////////////////////////////////////////////
enum struct valid_function_header{
 HW_VID=0, HW_PID, HW_SS_VID, HW_SS_PID,
 SW_VID, SW_PID, SW_CLID, SW_FID, SW_VERID, FUNC_ALIAS, 
 PLAT_CFG, REPLICA, NO_PARMS
};

using vfh = valid_function_header;
const std::map<valid_function_header, std::string> g_func_map {
  {vfh::FUNC_ALIAS, "func_alias"   }, 
  {vfh::REPLICA,    "func_repl"    },
  {vfh::HW_VID,     "hw_vid"       }, 
  {vfh::HW_PID,     "hw_pid"       },
  {vfh::HW_SS_VID,  "hw_ss_vid"    }, 
  {vfh::HW_SS_PID,  "hw_ss_pid"    },
  {vfh::SW_VID,     "sw_vid"       }, 
  {vfh::SW_PID,     "sw_pid"       },
  {vfh::SW_FID,     "sw_fid"       }, 
  {vfh::SW_CLID,    "sw_clid"      }, 
  {vfh::SW_VERID,   "sw_verid"     },
  {vfh::PLAT_CFG,   "platform_id"  }
};
///////////////////////////////////////////////////////////////////
enum struct valid_host_header{
 NAME=0, IPADDR, PORT, NO_PARMS 
};

using vhh = valid_host_header;
const std::map<valid_host_header, std::string> g_host_map {
  {vhh::NAME,   "host_name" },
  {vhh::IPADDR, "ip_addr"   },
  {vhh::PORT,   "port"      }
};
////////////////////////////////////////////////////////////////////


class base_entry{
  public:
    enum attr_search_option { FRONT, BACK };
 
    using attr_T = std::map<std::string, std::string>;

    base_entry( std::string );

    //get the header map
    virtual std::multimap<vhh, std::string>& get_header_desc(const vhh );
    virtual std::multimap<vfh, std::string>& get_header_desc(const vfh );
    virtual std::multimap<vph, std::string>& get_header_desc(const vph );

    const std::string get_json_str() const { return _as_is; };

    std::string  get_mapkey_str() { return _map_key; };
    void         set_mapkey_str( std::string new_key ) { _map_key = new_key; };

    void add_attribute( std::string key, std::string val)
         { _attrs[key] = val; };

    const std::string get_attribute( std::string key)
         { return _attrs.at(key); };

    const attr_T& get_attributes() const
         { return _attrs;};      
   
    template <typename T, int N=2>
    const std::array< std::list<std::string>, N>  serializeObj ();
 
    template <typename T> 
    std::list<std::string> get_header_attr( const typename T::key_type );

    template <typename T> 
    std::string get_header_attr( const typename T::key_type, const attr_search_option );

    template <typename T>
    void insert_header_attr( const typename T::key_type , const std::string);
 
    friend std::ostream& operator<<(std::ostream &, const base_entry& ); 

  private:
    //map key
    std::string _map_key;
    //json string
    std::string _as_is;
    //dissected attribute list
    attr_T _attrs;
};

class host_entry final : public base_entry{
  public:

    using desc_t   = std::multimap<vhh, std::string>;
    using key_type = desc_t::key_type;     
    //ctor
    host_entry( std::string);
    //get the header map
    desc_t& get_header_desc(const key_type) final;
    //gives the first value of the list related
    //to the key specified since its a multimap
    std::string get_first_header_attr(const key_type);
    std::string get_last_header_attr(const key_type);

    //overloaded std::cout
    friend std::ostream& operator<<(std::ostream &, const host_entry& ); 
   
    //NEED TO FIX TBD doesn't compile with private
    private:
    //functional keys
    desc_t _header_desc;
    
};

class function_entry final : public base_entry{

 public:
   using desc_t = std::multimap<vfh, std::string>;
   using key_type = desc_t::key_type;     
  
   //configure file json string
   function_entry( std::string );
   function_entry(std::string, std::map<vfh, std::string> );

   //get the header map
   desc_t& get_header_desc(const key_type) final;
   //gives the first value of the list related
   //to the key specified since its a multimap
   std::string get_first_header_attr(const key_type);
   std::string get_last_header_attr(const key_type);

   //serialize 
   const std::pair<std::list<std::string>, 
                   std::list<std::string> >  serialize ();

   //get alias
   const std::string get_func_alias() const;
   
   //get get_replicas
   ulong get_num_replicas();

   //format is a key value list. each entry has placement
   //information attached ex. func_alias=value 
   void import( const std::string, const std::string );

   const std::vector<std::string> 
        get_constraints_for( valid_function_header ) const;
   
   const bool contains_kv( function_entry& ) const;
    
   const desc_t& get_data() const;

   //operators
   //find based on alias
   bool operator==(const std::string& );
   //compared based on two function_entries
   bool operator==(const function_entry& );
   //overloaded std::cout
   friend std::ostream& operator<<(std::ostream &, const function_entry& ); 

 private:
   //functional keys
   desc_t _header_desc;
   
};

class platform_entry final : public base_entry{
  public:
    using desc_t = std::multimap<vph, std::string>; 
    using key_type = desc_t::key_type;     
 
    platform_entry( std::string );

    //get the header map
    desc_t& get_header_desc(const key_type) final;
    //gives the first value of the list related
    //to the key specified since its a multimap
    std::string get_first_header_attr(const key_type);
    std::string get_last_header_attr(const key_type);

    //import function
    void import(const std::string, const std::string);

    friend std::ostream& operator<<(std::ostream &, const platform_entry& ); 
  private:
   //functional keys
   desc_t _header_desc;
};


using config_entry     = std::variant< host_entry, function_entry, platform_entry>;
using ConfigFileData   = std::list<config_entry>;

bool operator==(const config_entry&, const config_entry&); 

class configfile{

  private:
    template<typename T>
    std::map<std::string, T> _get_typed_list() const;

    ConfigFileData data;

  public:

    configfile();
    
    bool import( std::string );

    bool set_config_sections( std::list<std::string>[3] );
 
    const bool entry_exists( const config_entry& ) const;
    //add entries to the config data
    void add_entry( config_entry );
    //returns json strings in a list
    template<typename T>
    std::list<std::string> get_section( ) const;
    //uses complete host_addr + port number as key
    std::map<std::string, host_entry > get_host_list() const{ 
                return _get_typed_list<host_entry>(); 
    };
    //uses function alias as key    
    std::map<std::string, function_entry > get_func_list() const{ 
                return _get_typed_list<function_entry>(); 
    };
    //uses platform characteristic ID as key
    std::map<std::string, platform_entry > get_plat_list() const{
                return _get_typed_list<platform_entry>();
    };
    
    function_entry get_function_entry( std::string );
    std::string    get_function_header( std::string );
 
    //gell all host address
    std::list<std::string> get_all_host_addrs();
    std::list<std::string> get_all_host_addrs_with_port();
    std::list<std::string> get_all_conn_str(zmq_transport_t );
 
    //retrieve platform attributes
    std::string get_platform_attr(std::string, vph) const;
    
    friend bool operator==(const config_entry&, const config_entry&);
    friend std::ostream& operator<<(std::ostream &, const configfile& ); 
    
};

#endif


