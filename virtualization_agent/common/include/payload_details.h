#include <string>
#include <vector>
#include <algorithm>
#include <variant>
#include <tuple>
#include <memory>
#include <zmq.hpp>
#include <payloads.h>
#include <pico_utils.h>
#include <ranges>
#include <resource_logic.h>

#ifndef PAYLOADDESC
#define PAYLOADDESC

typedef std::pair<std::string, std::string> string_pair;
using namespace std::ranges::views;

template <typename T>
struct misc_payload
{
  using v_type = std::vector<T>;

  misc_payload() = default;
  misc_payload(const v_type& );
  
  const v_type& get_data( ) const { return vdata; }
  void push_data ( T& meta  ) { vdata.push_back( meta ); }
  void set_data( v_type dat ) { vdata = dat; }

  v_type vdata; 

};

struct register_payload
{
  struct components{

    components() = default;

    components( std::string );

    std::string stringify();

    std::string vid{"*"};
    std::string pid{"*"};
    std::string ss_vid{"*"};
    std::string ss_pid{"*"};    

  };

  register_payload() = default;

  register_payload( std::string );

  
  std::vector<std::string> transform();

  const auto& get_components() const
  {
    return hw_desc;
  }
  
  std::vector<components> hw_desc;
};

struct single_kernel_payload{
   
   single_kernel_payload( ulong l_fid, std::vector<ulong> l_func_ids,
                          std::vector<ulong> nDevices)
   : fid(l_fid), func_ids(l_func_ids), num_of_devices(nDevices) {};    
   
   void push_func( ulong func_id, ulong num_devices ) 
   { 
     func_ids.push_back( func_id); 
     num_of_devices.push_back( num_devices );
   }

   std::vector<std::pair<ulong,ulong> > get_func_data() const
   {
     std::vector<std::pair<ulong,ulong> > out;
     
     for( int i =0; i < func_ids.size(); i++)
       out.emplace_back(func_ids[i], num_of_devices[i]);

     return out;
   }

   ulong fid;
   std::vector<ulong> func_ids;
   std::vector<ulong> num_of_devices;
};

struct identify_payload : misc_payload<std::string>{
  identify_payload ( const bool&, const std::vector<std::string>& );
  bool Append;
};


struct send_payload 
: public arg_definition<std::vector<zmq::message_t> >
{
  using arg_headers_t =std::vector<arg_header_t>;
  using arg_definition::base_type_t;

  struct header {
    using LookupType = resource_logic;
    header(): method("") {}
    header( std::string rsign) : method(rsign) {}
    void set_rsignature(std::string rsign ) { method.set_rsign( rsign ); } 
    LookupType  method;
  };

  send_payload( arg_headers_t&&, base_type_t&& );

  void set_id( std::string id ) { _AaId = id; }
  void set_resource( std::string res) 
  { 
    _resource_str = res;
    _hdr.set_rsignature( _resource_str );  
  }

  std::string get_resource( ) const { return _resource_str; }

  header get_header() const;

  std::string get_id() const { return _AaId; }
 
  std::string _AaId; 
  std::string _resource_str;
  header _hdr;

};

struct recv_payload 
: public arg_definition<
           std::vector< std::shared_ptr<void> > >
{
  using recv_deleter_t = std::function<void(void *)>;
  using arg_definition::base_type_t;

  recv_payload( );

  static void recv_deleter(void * ptr)
  { 
    std::cout << "Deleting recieve buffer..." << std::endl;  
    free(ptr); 
  }
  
  base_type_t::value_type allocate_back( arg_header_t argh)
  {
    auto[sign_t, data_t, t_size, size] = argh;
    //step 1: get base data
    auto& base_data = get_bdata();
    //step 2: push new arg into base data
    base_data.emplace_back(malloc( t_size * size), recv_deleter);
    //push arg information
    auto& sptr = base_data.back();
    push_output( {argh, sptr.get() } );
    //return shared ptr
    return base_data.back();
  }
 
  void push_output( arg_element_t&& args)
  {
    push_arg( std::forward<arg_element_t>(args) );
  }

  arg_element_t pop_output( ) const { return pop_arg(); }

  void set_tid(ulong tid ){ _tid = tid; }
  ulong get_tid( ){ return _tid; }

  ulong _tid; 

};

//general payload types
typedef misc_payload<std::string> misc_string_payload;
//system payload types
//--
//device payload types
typedef std::vector<single_kernel_payload> dev_kernel_payload;
#endif
