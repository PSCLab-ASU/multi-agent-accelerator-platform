#include <zmq.hpp>
#include <zmq_addon.hpp>
#include <cstdio>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <array>
#include <argparser.h>
#include <vector>
#include <map>
#include <fstream>
#include <numeric>
#include <list>
#include <initializer_list>
#include <variant>
#include <type_traits>
#include <mpi.h>
#include <boost/type_traits.hpp>
#include <boost/type_index.hpp>
#include <boost/range/algorithm.hpp>

#ifndef GENUTILS
#define GENUTILS

#define PLACEHOLDER "PLACEHOLDER"

#define REGISTRY_ENTRY(class)  \
      class * g_##class = new class();

template < typename K, typename V>
using var_maps = std::variant< std::map<K,V>, std::multimap<K,V> >;

 
enum ZPING_TYPE { NONE, T2T, P2P, N2N }; //thread-to-thread, process-to-process, node-to-node
enum struct zmq_transport_t { NONE, EXTERNAL, PROCESS, THREAD };

enum struct zmsg_section_type : ushort {STRING=0, LONG, SHORT, ULONG, FLOAT, DOUBLE, BOOL, INT, MEMBLK, ZMSG, NONE};

typedef zmsg_section_type kernel_function_t;


static const std::map<zmq_transport_t, std::string> g_transport_map = 
{
  {zmq_transport_t::NONE, ""},
  {zmq_transport_t::EXTERNAL, "tcp://"},
  {zmq_transport_t::PROCESS, "ipc:///"},
  {zmq_transport_t::THREAD,  "inproc://"},
};

static const std::map<std::string, zmsg_section_type> g_zst_map = 
{ 
  {"STRING", zmsg_section_type::STRING},
  {"INT",    zmsg_section_type::INT},
  {"LONG",   zmsg_section_type::LONG},
  {"SHORT",  zmsg_section_type::SHORT},
  {"ULONG",  zmsg_section_type::ULONG},
  {"FLOAT",  zmsg_section_type::FLOAT},
  {"DOUBLE", zmsg_section_type::DOUBLE},
  {"BOOL",   zmsg_section_type::BOOL},
  {"NONE",   zmsg_section_type::NONE}
};

enum struct env_vars { ENV_AGENT_ROOT=0, ENV_NONE };

const std::map<env_vars, std::string> g_env_vars_map =
{
 { env_vars::ENV_AGENT_ROOT, "AGENT_ROOT_DIR"}
};

typedef struct _pcie_device_id 
{
  std::string bus_id;
  std::string vid;
  std::string pid;
  std::string ss_vid;
  std::string ss_pid;
  _pcie_device_id() {}
 
} pcie_device_id;

typedef struct _defer_request
{
} defer_request;



typedef struct _rt_request
{
  //reserve context 0 for localbuffers
  ulong launching_slot_id;
  ulong capturing_slot_id;
  ulong tag;
  ulong addr;
  ulong alloc_id;
  ulong top_size;
  std::vector<ulong> v_len; 

  _rt_request( );

  _rt_request( zmq::multipart_t & );
  
} rt_request;

//one per FPGA
typedef struct _alloc_request
{
  uint algo_type;        //IP DESIGNATION
  uint datatype;   //DATA TYPE INFORMATION 
  uint max_slots; 
  uint active_slots; 
  uint vendor_id;
  uint prod_id;
  uint version_id;
  uint allocation_id; //used ONLY in case of deallocations and modify 

  _alloc_request( );

  _alloc_request( zmq::multipart_t & );

} alloc_request;

template <char delimiter>
class WordDelimitedBy : public std::string
{};

template <char delimiter>
std::istream& operator>>(std::istream&, WordDelimitedBy<delimiter>& );

template <char>
std::vector<std::string> split_str( std::string );

void stripUnicode(std::string &);

std::vector<std::string> read_machine_file( std::string, std::string);

int read_complete_file( std::string, std::string &);

std::ostream & operator << (std::ostream &, const alloc_request &); 
//std::ostream & operator << (std::ostream &, const kernel_repo_entry &); 


std::string exec(const char* ); 

std::multimap< std::string, std::vector<std::string> > enumerate_kernels_in_repo(std::string );
std::vector< std::string > identify_kernel(std::string );

std::vector< std::string > get_default_parm_from_aocx(std::string, std::string );
std::vector<std::string > get_default_mac_from_aocx(std::string );
std::vector<std::string > get_slot_count(std::string ); 
std::vector<std::string > get_MathML_description(std::string ); 

void filt_device_list(std::vector<pcie_device_id >&, std::string, std::string);

std::vector<pcie_device_id > get_pci_devices(std::string);

int parse_sw_resource( std::string, std::multimap<std::string, std::string> &);

void ap_usage(std::vector<std::string>, ArgumentParser &, bool);

int build_dummy_nex_header(std::string, zmq::multipart_t &);

int build_nex_header(zmq::multipart_t&, std::initializer_list<std::string> );

int build_nex_section_hdr(zmq::multipart_t&, zmsg_section_type, ulong);

template<typename T>
zmsg_section_type find_section_type();

template <typename T>
int build_nex_section(zmq::multipart_t&, std::list<T>); 

void insert_completion_tag(zmq::multipart_t & );

std::string get_hostname();

int get_mpi_dtype_size( uint );

template<typename ...Ts>
int get_input_params(const int , const char *[], bool, Ts& ...);

bool check_complete( std::string, ulong );
bool check_no_response_pkt( zmq::multipart_t & );
int  no_response_pkt( zmq::multipart_t & );

bool check_retransmission_pkt( const zmq::multipart_t );

int  retransmission_pkt(zmq::multipart_t &);

template< typename K, typename V>
void print_mmap( std::ostream&, std::multimap<K,V> const &);

template< typename K, typename V>
void print_map( std::ostream&, std::map<K,V> const &);

template< typename K, typename V>
std::list<std::string> flattened_map(const var_maps<K,V>& map);

void sanitize_map( std::map<std::string, std::string>& );

template<typename ...Ts>
void print_types()
{
  using TTypes = std::tuple<Ts...>;
  constexpr std::size_t size = std::tuple_size_v< std::tuple<Ts...> >;
  using Indices = std::make_index_sequence<size>;
  
  auto print_type = []<class T, std::size_t...I>(std::index_sequence<I...>)->void
  {
    using boost::typeindex::type_id_with_cvr;
    ((std::cout << (type_id_with_cvr<std::tuple_element_t<I,T>>().pretty_name()),
      (std::cout << "\n")),...);
  };
  print_type.template operator()<TTypes>(Indices{});

}

//TBD: Need to add concepts to protect types
//     Check if map has a first and secon
//     Check if val and map.second have a equality operator
//Carefull, each entry val must be unique
auto reverse_map_find( auto& base_map, auto val )
{
  for( auto& entry : base_map ) 
  {
    if( entry.second == val )
    {
      return entry.first;
    }
  } 
  
}

//get enviroment vars
template<env_vars evar>
constexpr std::function<std::optional<std::string>()> get_env_var( )
{
  return []( )->std::optional<std::string>
  {
    std::string env_var = g_env_vars_map.at(evar);

    std::string upper_ev, lower_ev;
    boost::transform( env_var, upper_ev.begin(),
    [](unsigned char c) { return std::toupper(c); } );

    boost::transform( env_var, lower_ev.begin(),
    [](unsigned char c) { return std::tolower(c); } );

    char* lenv_var = std::getenv( lower_ev.c_str() );
    char* uenv_var = std::getenv( upper_ev.c_str() );

    if( (lenv_var == nullptr) && (uenv_var == nullptr) )
    {
      std::cout << "Could not find root agent directory!" << std::endl;
      return {};
    }
    else if( uenv_var != nullptr )
      return uenv_var;
    else if(lenv_var != nullptr )
      return lenv_var;
    return {};
  };

}

template <typename C, typename T>
concept PushBackMovable = requires (C c, T t) {
    { c.push_back(t) } -> std::same_as<void>;
    { c.push_back(std::move(t)) } -> std::same_as<void>;
};

template <typename T, PushBackMovable<std::remove_reference_t<T>> C>
void operator+=(C& lhs, T&& rhs)
{
    lhs.push_back(std::forward<T>(rhs));
}

std::string generate_random_str();


#endif

