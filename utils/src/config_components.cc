#include <config_components.h>
#include <type_traits>

///////////////////////////////////////////////////////////////////
////////////////////// base_entry methods//////////////////////////
///////////////////////////////////////////////////////////////////
base_entry::base_entry(std::string json) : _as_is(json)
{
  if( json != "")
  {
    json_viewer jviewer("attr");
    _attrs = jviewer.get_object(json);
  }
}

std::ostream& operator<<(std::ostream& out, const base_entry& base_entry)
{
  //TBD
  //print_mmap(out, base_entry._attrs );
  return out;
}

host_entry::desc_t& 
base_entry::get_header_desc(const vhh key )
{
   throw std::runtime_error("Incidentally called the host base get_header_desc");
} 
function_entry::desc_t& 
base_entry::get_header_desc(const vfh key)
{
   throw std::runtime_error("Incidentally called the function base get_header_desc");
} 
platform_entry::desc_t& 
base_entry::get_header_desc(const vph key)
{
   throw std::runtime_error("Incidentally called the platform base get_header_desc");
} 

template <typename T>
std::list<std::string> base_entry::get_header_attr( const typename T::key_type key)
{
  //this is a virutal function overload
  typename T::desc_t header = get_header_desc(key);

  auto it_pair = header.equal_range(key);
  std::list<std::string> ret;
  std::for_each(it_pair.first, it_pair.second, [&](typename T::desc_t::value_type inp)
  {
    ret.push_back(inp.second);
  } ); 

  return ret;
}

template <typename T>
std::string base_entry::get_header_attr( const typename T::key_type key, const attr_search_option opt)
{
  auto header_list = get_header_attr<T>(key);
  if( opt == FRONT ) return header_list.front();
  else if( opt == BACK ) return header_list.back();
  else return "NO_PARM";
}


template <typename T>
void base_entry::insert_header_attr( const typename T::key_type key, const std::string val)
{
   typename T::desc_t& header = get_header_desc(key);

   header.insert( std::make_pair(key, val) ); 
}

template <typename T, int N>
const std::array<std::list<std::string>, N>  base_entry::serializeObj( )
{
  using ReturnType     = std::list<std::string>;
  using ArrReturnType  = std::array<ReturnType, N>;

  ArrReturnType ret;
 
  typename T::key_type key_tag = T::key_type::NO_PARMS;

  typename T::desc_t& header = get_header_desc(key_tag);
  //set
  auto flat_base   = flattened_map<std::string, std::string>( _attrs ); 
  auto flat_header = flattened_map<typename T::key_type, std::string>( header ); 

  std::get<0>(ret) = flat_base;
  std::get<1>(ret) = flat_header;

  return ret;
  
}

///////////////////////////////////////////////////////////////////
////////////////////// host_entry methods//////////////////////////
///////////////////////////////////////////////////////////////////

host_entry& host_entry::operator=( const host_entry& rhs)
{
  this->_header_desc = rhs.get_header_desc();
  return *this;
}

host_entry::desc_t& 
host_entry::get_header_desc(const host_entry::key_type key) 
{
  return this->_header_desc;
}

host_entry::host_entry(std::string json) : base_entry(json)
{
  //clear mmap
  _header_desc.clear();

  using T = valid_host_header;

  auto save_array = [&](T ktype, std::string key_val)
  {
    json_viewer jv(key_val);
    auto vals = jv.get_array(json);
    for( auto& val : vals)
      if( val.empty() )
        this->_header_desc.emplace(ktype, "*");
      else
        this->_header_desc.emplace(ktype, val);
  }; 
 
  for( auto& entry : g_host_map )
    save_array(entry.first, entry.second );
  
  //set map name
  set_mapkey_str( _header_desc.find(T::NAME)->second );
}

std::ostream& operator<<(std::ostream& out, const host_entry& host_e)
{
  print_mmap(out, host_e._header_desc );
  return out;
}

std::string host_entry::get_first_header_attr(const host_entry::key_type key)
{
  std::string val = get_header_attr<std::remove_pointer_t<decltype(this)> >(key, base_entry::FRONT); 
  return val;
}

std::string host_entry::get_last_header_attr(const host_entry::key_type key)
{
  std::string val = get_header_attr<std::remove_pointer_t<decltype(this)> >(key, base_entry::BACK); 
  return val;
}

std::string host_entry::get_ext_connstr()
{
  return g_transport_map.at(zmq_transport_t::EXTERNAL) + 
         get_last_header_attr(vhh::NAME) + ":" +
         get_last_header_attr(vhh::PORT);
}

node_mode host_entry::get_node_mode()
{
  std::string mode_str = get_last_header_attr(vhh::MODE);
  return reverse_map_find(g_node_mode_map, mode_str);

}

///////////////////////////////////////////////////////////////////
////////////////////// func_entry methods//////////////////////////
///////////////////////////////////////////////////////////////////
function_entry::desc_t& 
function_entry::get_header_desc(const function_entry::key_type key)
{
  return this->_header_desc;
}

function_entry::function_entry(std::string json) : base_entry(json)
{
  //clear mmap
  _header_desc.clear();
 
  using T = valid_function_header;

  auto save_array = [&](T ktype, std::string key_val)
  {
    json_viewer jv(key_val);
    auto vals = jv.get_array(json);
    for( auto& val : vals)
    { 
      if( val.empty() )
        this->_header_desc.emplace(ktype, "*");
      else
        this->_header_desc.emplace(ktype, val);
    }
  }; 

  for( auto& entry : g_func_map )
  {
    save_array(entry.first, entry.second );
  }
  //set map name
  set_mapkey_str( _header_desc.find(T::FUNC_ALIAS)->second );

}

function_entry::function_entry( std::string json, std::map<valid_function_header, std::string> m)
: base_entry(json)
{
  using T = valid_function_header;
  _header_desc.clear();
  //moving backend data structure
  _header_desc.insert(m.begin(), m.end() );
  //set map name 
  auto alias_name_it = _header_desc.find(T::FUNC_ALIAS);
 
  if( alias_name_it != _header_desc.end() )
    set_mapkey_str( _header_desc.find(T::FUNC_ALIAS)->second );
  else
    set_mapkey_str("*");
}

const std::string function_entry::get_func_alias() const
{
  return _header_desc.find(valid_function_header::FUNC_ALIAS)->second;
}

const std::vector<std::string> function_entry::get_constraints_for(valid_function_header vfh) const
{
  //auto iter_pair = _func_desc.equal_range(vfh);
  auto iter_pair = _header_desc.equal_range(vfh);
  std::vector<std::string> ll;
  for( auto e = iter_pair.first; e != iter_pair.second; e++)
    ll.push_back(e->second);

  return ll;
}

const bool function_entry::contains_kv( function_entry& fe) const
{
  //determine if the fe is a subset of this ptr
  //need to see if fe fits within the constraints of thisa
  using element = std::pair<const valid_function_header, std::string>;
  auto data = fe.get_data();
  bool match = true;
  for(auto bIter = data.begin(), eIter = data.end(); bIter != eIter; 
      bIter = data.upper_bound( bIter->first )  ) 
  {
    //happens once for every key in the multimap
    const auto fe_eqr   = data.equal_range( bIter->first );
    const auto this_eqr = this->_header_desc.equal_range( bIter->first );

    match &= std::any_of( fe_eqr.first, fe_eqr.second, [&](const element& looking_for)
    {
      std::string s = looking_for.second;
      return std::any_of( this_eqr.first, this_eqr.second, [&](const element& within)
             {    
                  std::string ss = within.second;
                  if( (s  == CFG_WILDCARD) || 
                      (ss == CFG_WILDCARD) || 
                      (s  == ss)  )
                  {
                    return true; 
                  }
                  else return false;
             } ); //end of the first any_of
    } ); //end of second any_of
  
  } //end of for loop 

  return match;

}

ulong function_entry::get_num_replicas()
{
  auto itVal = _header_desc.equal_range(vfh::REPLICA);
  ulong d = std::distance(itVal.first, itVal.second);
  auto nx = std::next(itVal.first, d-1);
  return std::stoul( nx->second );
}


bool function_entry::operator==(const std::string& alias)
{
  return false;
}

bool function_entry::operator==(const function_entry& func_entry)
{ 
  return (_header_desc == func_entry._header_desc);
}

const function_entry::desc_t& function_entry::get_data() const
{
  return _header_desc;
}

void function_entry::import( const std::string func_desc, 
             const std::string meta_data )
{
  //lambda to find key based on value 
  auto str_to_vfh = [](std::string key)->vfh
  {
    using T = std::pair<const vfh, std::string>;
    auto vfh_entry = std::find_if(g_func_map.begin(), g_func_map.end(), [&]( T val)
    {
      return key == val.second;
    });

    if( vfh_entry == g_func_map.end())
      return vfh::NO_PARMS;
    else  
      return vfh_entry->first;
  };

  //need to fuse config entry ith function based onr res/meta
  //start with a copy of the configfile entry
  //then fill in the override key1=val1,key2=val2,key3=val3
  auto func_parm_list = split_str<','>(func_desc);
  auto meta_parm_list = split_str<','>(meta_data);

  auto alias_def = std::find_if(func_parm_list.begin(), func_parm_list.end(),
                               [](std::string keyval)
                               {
                                  auto kv = split_str<'='>(keyval);
                                  return  ( kv[0] == g_func_map.at(vfh::FUNC_ALIAS) );
                               } );

  std::string alias = split_str<'='>(*alias_def)[1];
  //add all the header information
  std::for_each(func_parm_list.begin(), func_parm_list.end(),[&]( const std::string inp )
  {
    auto skeyval = split_str<','>(inp);
    auto key     = str_to_vfh( skeyval[0] );
    if( key != key_type::NO_PARMS )
      insert_header_attr<std::remove_pointer_t<decltype(this)> >(key, skeyval[1] );

  } ); 
  
  //add meta paraemter 
  std::for_each(meta_parm_list.begin(), meta_parm_list.end(),[&]( const std::string inp )
  {
    auto keyval = split_str<','>(inp);
    add_attribute(keyval[0], keyval[1]);

  } ); 

}

std::string function_entry::get_first_header_attr(const function_entry::key_type key)
{
  std::string val = get_header_attr<std::remove_pointer_t<decltype(this)> >(key, base_entry::FRONT); 
  return val;
}

std::string function_entry::get_last_header_attr(const function_entry::key_type key)
{
  std::string val = get_header_attr<std::remove_pointer_t<decltype(this)> >(key, base_entry::BACK); 
  return val;
}

const std::pair<std::list<std::string>, std::list<std::string> > function_entry::serialize()
{

  auto [base, derived] = serializeObj<std::remove_pointer_t<decltype(this)> >(); 
  return std::make_pair(base, derived);
  
}

std::ostream& operator<<(std::ostream& out, const function_entry& func_entry)
{
  print_mmap(out, func_entry._header_desc );
  return out;
}
///////////////////////////////////////////////////////////////////
////////////////////// plat_entry methods//////////////////////////
///////////////////////////////////////////////////////////////////
platform_entry::desc_t& 
platform_entry::get_header_desc(const platform_entry::key_type key )
{
  return this->_header_desc;
}

platform_entry::platform_entry(std::string json) : base_entry(json)
{ 
  //clear mmap
  _header_desc.clear();

  using T = valid_platform_header;

  auto save_array = [&](T ktype, std::string key_val)
  {
    json_viewer jv(key_val);
    auto vals = jv.get_array(json);
    for( auto& val : vals)
      if( val.empty() )
        this->_header_desc.emplace(ktype, "*");
      else
        this->_header_desc.emplace(ktype, val);
  }; 
 
  for( auto& entry : g_plat_map )
    save_array(entry.first, entry.second );

  //set map name
  set_mapkey_str( _header_desc.find(T::PLAT_CFG)->second );

}

void platform_entry::import(const std::string plat_desc, const std::string meta_desc)
{
  #warning "platform_entry: TBD"
}

std::string platform_entry::get_first_header_attr(const platform_entry::key_type key)
{
  std::string val = get_header_attr<std::remove_pointer_t<decltype(this)> >(key, base_entry::FRONT); 
  return val;
}

std::string platform_entry::get_last_header_attr(const platform_entry::key_type key)
{
  std::string val = get_header_attr<std::remove_pointer_t<decltype(this)> >(key, base_entry::BACK); 
  return val;
}

std::ostream& operator<<(std::ostream& out, const platform_entry& plat_entry)
{
  print_mmap(out, plat_entry._header_desc); 
  return out;
}

///////////////////////////////////////////////////////////////////
////////////////////// ConfigFile methods//////////////////////////
///////////////////////////////////////////////////////////////////

configfile::configfile()
{
  data.clear();
}

bool configfile::import( std::string cfg )
{
  std::list<std::string> ll[3];
  
  auto res_addrs = read_machine_file(cfg, "host_list");
  auto func_list = read_machine_file(cfg, "func_list");
  auto res_meta  = read_machine_file(cfg, "platform_list");

  ll[0]  = std::list<std::string>(res_addrs.begin(), res_addrs.end());
  ll[1]  = std::list<std::string>(func_list.begin(), func_list.end());
  ll[2]  = std::list<std::string>(res_meta.begin(), res_meta.end());  

  auto ret = set_config_sections( ll );

  return ret; 
}

bool configfile::set_config_sections( std::list<std::string> config_sections[3] )
{
  auto add_to_config = [&](int idx,  std::list<std::string> config ){
            std::for_each(config.begin(), config.end(), 
                          [&](const std::string& entry){
                          //std::cout << "Adding config: " << entry << std::endl;
                          if ( idx == 0 )
                            this->add_entry(host_entry(entry));
                          else if ( idx == 1) 
                            this->add_entry(function_entry(entry));
                          else if ( idx == 2) 
                            this->add_entry(platform_entry(entry));
                         });
  }; 
  
  //add host section
  add_to_config(0, config_sections[0]);  
  //add function section
  add_to_config(1, config_sections[1]);  
  //add meta section
  add_to_config(2, config_sections[2]);  
  
 
  return false;

}

const bool configfile::entry_exists( const config_entry& ce) const
{

  bool exists = std::any_of(data.begin(), data.end(), [&](const config_entry& temp_ce)
  {
    //calls the overloade function for config_entry == config_entry....
    return (ce == temp_ce);
  });

  return exists;
}


void configfile::add_entry( config_entry entry)
{
  //remove repeats
  if( !entry_exists( entry ) )
  {
    data.push_back( entry );
  }
  else
  {
    std::cout << "Entries does not exists" << std::endl;
  }
}

template<typename T>
std::list<std::string> configfile::get_section() const
{
  std::list<std::string> config_section;

  //go through all the entries in the vector
  for(auto& config_entry : data)
  {
    //go through each variant element
    std::visit([&]( auto&& entry ){
      using dec_T = std::decay_t< decltype(entry) >;
      if constexpr ( std::is_same_v<T, dec_T> )
        config_section.push_back( entry.get_json_str() );
    }, config_entry );
  }

  return config_section;
}

template<typename T>
std::map<std::string, T> configfile::_get_typed_list() const
{
  std::map<std::string, T> config_section;

  //go through all the entries in the vector
  for(auto& c_entry : data)
  {
    //go through each variant element
    std::visit([&]( auto& entry ){
      using dec_T = std::decay_t< decltype(entry) >;
      dec_T d_entry = entry;
      if constexpr ( std::is_same_v<T, dec_T> )
        config_section.emplace( d_entry.get_mapkey_str(), d_entry );
    }, c_entry );
  }

  return config_section;

}

function_entry configfile::get_function_entry(std::string alias)
{
  auto func_list = get_func_list();
  return func_list.at(alias);
}

std::string configfile::get_platform_attr(std::string id, vph key ) const
{
  //get the list of platform configs
  auto plats = get_plat_list();
  //get the attributes in speciifc id
  platform_entry& plat = plats.at(id);
  auto attr            = plat.get_header_attr<platform_entry>( key );

  return attr.front();
}


std::list<std::string> configfile::get_all_host_addrs() 
{
  auto htmp = get_host_list();
  std::list<std::string> ret(htmp.size());
  std::transform(htmp.begin(), htmp.end(), ret.begin(),
                 [](auto t)->std::string
                 {
                   return t.second.get_last_header_attr(vhh::NAME);
                 });
  return ret;
}

std::list<std::string> configfile::get_all_host_addrs_with_port() 
{
  auto htmp = get_host_list();
  std::list<std::string> ret(htmp.size());
  std::transform(htmp.begin(), htmp.end(), ret.begin(),
                 [](auto t)->std::string
                 {
                   return t.second.get_last_header_attr(vhh::NAME) + ":" +
                          t.second.get_last_header_attr(vhh::PORT);
                 });
  return ret;
}

std::list<std::string> configfile::get_all_conn_str(zmq_transport_t transport )
{
  auto hosts = get_all_host_addrs_with_port();
  std::string prefix = g_transport_map.at(transport);

  boost::range::transform( hosts, hosts.begin(), 
                           [&](const std::string& nex)->std::string
                           {
                             return prefix + nex;  
                           } );
 return hosts;    
}

std::string configfile::get_function_header( std::string falias)
{

  auto fe = get_function_entry( falias );

  auto dm = fe.get_data();
  auto& fmap = g_func_map;
 
  std::string desc = fmap.at(vfh::HW_VID)    + "=" + 
                     dm.equal_range(vfh::HW_VID).first->second + ":" +
                     fmap.at(vfh::HW_PID)    + "=" + 
                     dm.equal_range(vfh::HW_PID).first->second + ":" +
                     fmap.at(vfh::HW_SS_VID) + "=" + 
                     dm.equal_range(vfh::HW_SS_VID).first->second + ":" +
                     fmap.at(vfh::HW_SS_PID) + "=" + 
                     dm.equal_range(vfh::HW_SS_PID).first->second + ":" +
                     fmap.at(vfh::SW_VID)    + "=" + 
                     dm.equal_range(vfh::SW_VID).first->second + ":" +
                     fmap.at(vfh::SW_PID) + "=" + 
                     dm.equal_range(vfh::SW_PID).first->second + ":" +
                     fmap.at(vfh::SW_CLID) + "=" + 
                     dm.equal_range(vfh::SW_CLID).first->second + ":" +
                     fmap.at(vfh::SW_FID) + "=" + 
                     dm.equal_range(vfh::SW_FID).first->second + ":" +
                     fmap.at(vfh::SW_VERID) + "=" + 
                     dm.equal_range(vfh::SW_VERID).first->second;

  std::cout << "Get function details: " << falias << " -> " << desc << std::endl; 
  return desc;
}

bool operator==(const config_entry& lhs, const config_entry& rhs)
{
  //TBD
  return false;
}

std::ostream& operator<<(std::ostream& out, const configfile& config)
{
  //call each individual overload for ostream
  for(auto& config_entry : config.data)
  {
    //go through each variant element
    std::visit([&]( auto&& entry ){
      out << entry;
    }, config_entry );
  }
  
  return out;
}


//////////////////////////template//////////////////////////////
template std::list<std::string> configfile::get_section<host_entry>()     const;
template std::list<std::string> configfile::get_section<function_entry>() const;
template std::list<std::string> configfile::get_section<platform_entry>() const;

template void base_entry::insert_header_attr<host_entry>(     const host_entry::key_type ,     const std::string );
template void base_entry::insert_header_attr<function_entry>( const function_entry::key_type , const std::string );
template void base_entry::insert_header_attr<platform_entry>( const platform_entry::key_type , const std::string );

template std::list<std::string> base_entry::get_header_attr<host_entry>     ( const host_entry::key_type     );
template std::list<std::string> base_entry::get_header_attr<function_entry> ( const function_entry::key_type );
template std::list<std::string> base_entry::get_header_attr<platform_entry> ( const platform_entry::key_type );

template std::string base_entry::get_header_attr<host_entry>     
( const host_entry::key_type,     const attr_search_option );
template std::string base_entry::get_header_attr<function_entry> 
( const function_entry::key_type, const attr_search_option );
template std::string base_entry::get_header_attr<platform_entry> 
( const platform_entry::key_type, const attr_search_option );
