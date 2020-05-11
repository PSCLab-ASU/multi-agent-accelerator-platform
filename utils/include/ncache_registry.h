#include "config_components.h"
#include <optional>
#include <string>
#include <vector>
#include <tuple>
#include <utils.h>

#ifndef NCACHE_REG
#define NCACHE_REG

class cached_resource_desc
{

  public: 

    template<typename ... Ts>
    cached_resource_desc ( Ts... ts) 
    {
      int in_sz = sizeof...(ts);
      std::string inputs[in_sz] = {ts...};
      
      for(int i=0; i < in_sz; i++ )
      {
        //do first split on semicolo
        auto split_res = split_str<':'>( inputs[i] );
        //run through each entry
        for(int j=0; j < split_res.size(); j++ )
        {
          auto keyval = split_str<'='>( split_res[j] );
          if( keyval.size() == 1 )
            _insert(j, keyval[0] );
          else
            _insert(keyval[0], keyval[1] );
        }
      }
      //fill the rest of the entries with *
      _fill_wildcards();
    }

    //accessor chunks
    const std::string& get_hw_vid()    const { _func_desc.at(vfh::HW_VID); }    
    const std::string& get_hw_pid()    const { _func_desc.at(vfh::HW_PID); }    
    const std::string& get_hw_ss_vid() const { _func_desc.at(vfh::HW_SS_VID); } 
    const std::string& get_hw_ss_pid() const { _func_desc.at(vfh::HW_SS_PID); } 
    const std::string& get_sw_vid()    const { _func_desc.at(vfh::SW_VID); }    
    const std::string& get_sw_pid()    const { _func_desc.at(vfh::SW_PID); }    
    const std::string& get_sw_clid()   const { _func_desc.at(vfh::SW_CLID); }   
    const std::string& get_sw_fid()    const { _func_desc.at(vfh::SW_FID); }    
    const std::string& get_sw_verid()  const { _func_desc.at(vfh::SW_VERID); }  
    const std::map<valid_function_header, std::string>& get_data() const
    {
      return _func_desc;
    }
    //operators
    bool operator==( const cached_resource_desc& rhs ) 
    {
      return (rhs.get_data() == get_data() );
    }
   
    template<valid_function_header comp>
    static auto compare( const std::string& val )
    {
      return [=](const cached_resource_desc& input){ 
                 if constexpr( comp == vfh::HW_VID ) 
                   return  (val == input.get_hw_vid() );
                 else if constexpr( comp == vfh::HW_PID ) 
                   return  (val == input.get_hw_pid() );
                 else if constexpr( comp == vfh::HW_SS_VID ) 
                   return  (val == input.get_hw_ss_vid() );
                 else if constexpr( comp == vfh::HW_SS_PID ) 
                   return  (val == input.get_hw_ss_pid() );
                 else if constexpr( comp == vfh::SW_VID ) 
                   return  (val == input.get_sw_vid() );
                 else if constexpr( comp == vfh::SW_PID ) 
                   return  (val == input.get_sw_pid() );
                 else if constexpr( comp == vfh::SW_FID ) 
                   return  (val == input.get_sw_fid() );
                 else if constexpr( comp == vfh::SW_CLID ) 
                   return  (val == input.get_sw_clid() );
                 else if constexpr( comp == vfh::SW_VERID ) 
                   return  (val == input.get_sw_verid() );
                 else return false;
                };
    }

    static auto compare_functional( const std::string& clid, 
                                    std::optional<std::string> fid )
    {
      return [=](const cached_resource_desc& input){ 
                   return  (clid == input.get_sw_clid() ) || 
                           (fid && ( fid.value() == input.get_sw_fid() )) ||
                           (fid && ( fid.value() == "*") );
             };
    }

    static auto compare_execute( const std::string& clid, 
                                 const std::string& fid )
    {
      return [=](const cached_resource_desc& input){ 
                   if( (clid == "*" && fid == "*") )
                     return false;
                   else if( clid == "*" && fid != "*" )
                     return ( (input.get_sw_fid() == fid) &&
                               input.check_hw_fully_desc() );
                   else if( clid != "*" && fid == "*" )
                     return ( (input.get_sw_clid() == clid) &&
                               input.check_hw_fully_desc() );
                   else if( clid != "*" && fid != "*" )
                     return ( (input.get_sw_clid() == clid) &&
                              (input.get_sw_fid() == fid) &&
                               input.check_hw_fully_desc() );
                   else return false;    

             };
    }

    static auto compare_supportable( const std::string& hw_vid,
                                     const std::string& hw_pid, 
                                     std::optional<std::string> hw_ss_vid,
                                     std::optional<std::string> hw_ss_pid )
    {
      return [=](const cached_resource_desc& input){ 

                   bool ret = true;
                   if( hw_vid != "*" ) ret &= (hw_vid == input.get_hw_vid() );
                   if( hw_pid != "*" ) ret &= (hw_vid == input.get_hw_vid() );
                   ret &= input.check_sw_wildcard();
                   if( hw_ss_vid && hw_ss_vid != "*" ) ret &= ( hw_ss_vid.value() == input.get_hw_ss_vid() );
                   if( hw_ss_pid && hw_ss_pid != "*" ) ret &= ( hw_ss_pid.value() == input.get_hw_ss_pid() );
                   return ret;
             };
    }
   
    bool check_sw_wildcard() const
    {
      return ( _func_desc.at(vfh::SW_VID)   == "*" ) &&
             ( _func_desc.at(vfh::SW_PID)   == "*" ) &&
             ( _func_desc.at(vfh::SW_FID)   == "*" ) &&
             ( _func_desc.at(vfh::SW_CLID)  == "*" ) &&
             ( _func_desc.at(vfh::SW_VERID) == "*" );
    }

    bool check_hw_fully_desc() const
    {
      return ( _func_desc.at(vfh::HW_VID)    != "*" ) &&
             ( _func_desc.at(vfh::HW_PID)    != "*" ) &&
             ( _func_desc.at(vfh::HW_SS_VID) != "*" ) &&
             ( _func_desc.at(vfh::HW_SS_PID) != "*" );
    }
   
    std::string stringify() const;

  private:
    void _insert( int, std::string );
    void _insert( std::string, std::string);
    void _fill_wildcards();
 
    std::map<valid_function_header, std::string> _func_desc = g_func_map; 

};

class ncache_registry
{
  public:

    //ncache_registry(){}
 
    void add_resources( std::list<std::string> resources)
    {
        for( auto resource : resources)
        {
          std::cout << "Attempting to add resource : " << resource << std::endl;
          add_resource( resource );
        }
    }
    
    //add partial cache lines
    //requires just value
    //RESTRICTED values cannot have semicolons or equals signsa
    template<typename ... Ts > 
    void add_resource( Ts... ts)
    {
        cached_resource_desc res(ts...);

        if( !exact_exists( res ) )
        {
	  //add to the cache
          _caches.emplace_back( res ); 
        }
        else std::cout << "Ignoring entry ..." << std::endl;
    }

    //functional existense implies you can execute
    //a kernel on which only applies to FID and CLID
    uint functional_exists( std::string clid, std::optional<std::string> fid ) const;

    //equal to the equality operator
    bool functional_exists( cached_resource_desc res) const;

    //equal to the equality operator
    bool exact_exists( cached_resource_desc res) const;

    bool can_completely_support( std::string ) const;

    bool can_execute( cached_resource_desc res ) const;

    uint can_support( std::string, std::string,
                      std::optional<std::string>, 
                      std::optional<std::string> ) const;
        
    //find full line
    std::optional<cached_resource_desc> 
      find_functional(std::string, std::optional<std::string> );

    std::list<std::string> dump_manifest() const;

  private:
                //cached resource description
    std::vector<cached_resource_desc> _caches;

};

#endif
