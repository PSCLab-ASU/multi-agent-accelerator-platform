#include <typeinfo>
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
#include <utils.h>
#include <functional>
#include <regex>
#include <numeric>
#include <json.h>
#include <json_viewer.h>
#include <config_components.h>
#include <type_traits>
#include <random>


_rt_request::_rt_request( )
{
  top_size = 5;
}

_rt_request::_rt_request( zmq::multipart_t & msg) : _rt_request()
{
  int i=0;
  std::vector<ulong *> vec {&launching_slot_id, &capturing_slot_id,
                       &tag, &addr, &alloc_id };
  while(msg.size())
  {
    ulong data_element = msg.poptyp<ulong>();
    if( i < top_size)
      *vec[i] = data_element;
    else
      v_len.push_back(data_element);
    i++;
  }
}
  
//one per FPGA
_alloc_request::_alloc_request( )
{}

_alloc_request::_alloc_request( zmq::multipart_t & msg)
{
  int i=0;
  std::vector<uint *> vec {&algo_type, &datatype, &max_slots, &active_slots,
                      &vendor_id, &prod_id, &version_id,
                      &allocation_id };
  while(msg.size())
  {
    uint data_element = msg.poptyp<uint>();
    *vec[i] = data_element;
    i++;
  }
}
template <char delimiter>
std::istream& operator>>(std::istream& is, WordDelimitedBy<delimiter>& output)
{
   std::getline(is, output, delimiter);
   return is;
}

bool invalidChar (char c) 
{  
    return !(c>=0 && c <128);   
}
 
void stripUnicode(std::string & str) 
{ 
    str.erase(remove_if(str.begin(),str.end(), invalidChar), str.end());  
}


template<char delimiter>
std::vector<std::string> split_str(std::string inp)
{
  std::istringstream iss(inp);
  std::vector<std::string> parm_data(
    (std::istream_iterator<WordDelimitedBy< delimiter > >(iss)),
    std::istream_iterator<WordDelimitedBy< delimiter > >());
 
  return parm_data;
}

template std::vector<std::string> split_str<'.'>(std::string inp);
template std::vector<std::string> split_str<':'>(std::string inp);
template std::vector<std::string> split_str<';'>(std::string inp);

std::ostream & operator << (std::ostream &out, const alloc_request &el) 
{ 
    out << "algo_type = "     <<el.algo_type<<std::endl; 
    out << "datatype  = "     <<el.datatype<<std::endl; 
    out << "max_slots = "     <<el.max_slots<<std::endl; 
    out << "active_slots = "  <<el.active_slots<<std::endl; 
    out << "vendor_id = "     <<el.vendor_id<<std::endl; 
    out << "prod_id = "       <<el.prod_id<<std::endl; 
    out << "version_id = "    <<el.version_id<<std::endl; 
    out << "allocation_id = " <<el.allocation_id<<std::endl; 
    return out; 
} 

std::string exec(const char* cmd) {
    std::array<char, 128> buffer;
    std::string result;
    std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd, "r"), pclose);
    if (!pipe) {
        throw std::runtime_error("popen() failed!");
    }
    while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
        result += buffer.data();
    }
    return result;
}

std::multimap<std::string, std::vector<std::string> > enumerate_kernels_in_repo(std::string repo_dir)
{
  //call ls on the passed in repo
  std::multimap<std::string, std::vector<std::string> > func_out; 
  std::string command  = std::string("ls ") + repo_dir + std::string("*");
  std::string resp   = exec(command.c_str());
  //std::cout << "repo dir = " << resp <<std::endl;

  //auto parm_data = splt_val.operator()<'\n'>(resp);
  auto parm_data = split_str<'\n'>(resp);
  
  for(auto aocx_file : parm_data)
  {
    //filename + path
    auto&& temp = split_str<'/'>(aocx_file);
    std::string file_name = temp.back();
    //get the id of the kernel
    std::vector<std::string> id = identify_kernel(aocx_file);
    //extract number of slots
    std::string max_slots;
    //HACK to get max_slots
    for(std::string i : id) max_slots = i;
    //mac_addr
    auto mac_addr = get_default_mac_from_aocx(aocx_file);
    //function MAthXML
    auto xFunction = get_MathML_description(aocx_file);
    //greater than one because every read of the file will have a section label
    if( id.size() > 1) 
    {
      func_out.insert( std::make_pair(file_name, id) );
      std::vector<std::string> v_slots = {"slot_count", max_slots};
      func_out.insert( std::make_pair(file_name, v_slots) );
      if( mac_addr.size() > 1)   func_out.insert( std::make_pair(file_name, mac_addr) ); 
      if( xFunction.size() > 1 ) func_out.insert( std::make_pair(file_name, xFunction) );
    }
  }

  return func_out;
  
}

int parse_sw_resource(std::string serial_sw_resource, std::multimap<std::string, std::string> &output)
{
 //input format: id=a:b:c:d:e:f:g:h,file=path/file,mac_addr=xx:xx:xx:xx:xx;...,group_id
 auto all_items = split_str<','>(serial_sw_resource);

 for(auto item : all_items)
 {
   auto keyval = split_str<'='>(item);
   output.insert( std::make_pair(keyval[0], keyval[1]) );
 }

 return 0;
}

std::string generate_random_str()
{
  size_t length = 16; 
  const std::string CHARACTERS = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";

  std::random_device random_device;
  std::mt19937 generator(random_device());
  std::uniform_int_distribution<> distribution(0, CHARACTERS.size() - 1);

  std::string random_string;

  for (std::size_t i = 0; i < length; ++i)
  {
      random_string += CHARACTERS[distribution(generator)];
  }

  return random_string;

}

int read_complete_file( std::string abs_file_path, std::string &file_content ) 
{

  std::ifstream lfile(abs_file_path.c_str() );

  if( lfile.is_open() ) 
  {
    //move cursor to end of file
    lfile.seekg(0, std::ios::end);
    //get the current index of the last characte
    //and reserve std::string size
    file_content.reserve(lfile.tellg());
    //move cursor to the begining of the file
    lfile.seekg(0, std::ios::beg);

    //use iterators to copy into string 
    file_content.assign((std::istreambuf_iterator<char>(lfile)),
                         std::istreambuf_iterator<char>());

  }
  else{ return -1; }
 
  //closing the file handle
  lfile.close();
 
  //return successfule
  return 0;

}


std::vector<std::string > read_machine_file(std::string hostfile, std::string section_name)
{
  std::vector<std::string> entries;
  std::string file_content, err;

  int status = read_complete_file(hostfile, file_content);

  if( status )
  {
    //nothing todo here:
    //if there is no hostfile
    //the nexus rollcall automatically uses localhost
  }
  else
  {  
    auto jconfig = json::Json::parse(file_content, err);
    for( auto& entry : jconfig[section_name].array_items() )
    {
      //std::cout << entry.dump() << std::endl;
      entries.push_back(entry.dump() );
    }
  }

  return entries;
}

std::vector< std::string > identify_kernel( std::string aocx_file )
{
  std::vector<std::string> temp;
 
  try {
    //std::cout <<"Openning file: "<< aocx_file <<std::endl;
    temp = get_default_parm_from_aocx( aocx_file, "kernel_identity" );
    //first element in the kernel lookup is type
    temp.insert(temp.begin(), "kernel_identity");
  }
  catch(...){
    std::cout <<aocx_file <<" is not a FPGA Intel Kernel" <<std::endl;
    temp.clear();
  }
  return temp;
}


std::vector< std::string > get_default_parm_from_aocx(std::string aocx_file, std::string parm)
{
   std::string cmd = "aocl binedit " + aocx_file + " print ." + parm;
   std::string resp = exec(cmd.c_str());
   auto filename = split_str<'/'>( aocx_file).back();
   //std::cout << filename << ": resp = " << resp <<std::endl;
   
   auto parm_data = split_str<':'>(resp);

   return std::move(parm_data);
}

std::vector<std::string > get_default_mac_from_aocx(std::string aocx_file)
{
   std::string cmd = "aocl binedit " + aocx_file + " print .mac_addr";
   std::string resp = exec(cmd.c_str());

   auto mac_addr = split_str<':'>(resp);
   mac_addr.insert(mac_addr.begin(), "mac_addr");

   return mac_addr;
}

std::vector<std::string> get_slot_count(std::string aocx_file)
{
  std::vector<std::string > v_slots = {"0"};
  return v_slots;

}

std::vector<std::string> get_MathML_description(std::string aocx_file)
{
  //auto v_mathML_desc = get_default_parm_from_aocx(aocx_file, "mathml_desc0");
  std::string cmd = "aocl binedit " + aocx_file + " print .mathml_desc0";
  std::string resp = exec(cmd.c_str());
  

  return {"xFunction", resp};
}

bool operator==(const pcie_device_id & lhs, const std::pair<std::string, std::string> elem)
{
  std::string ss_vid, ss_pid;
 
  std::tie(ss_vid, ss_pid) = elem;
  //gaurantees that empty subsystem id are asterisks
  if( ss_vid.empty() ) ss_vid = "*";
  if( ss_pid.empty() ) ss_pid = "*";
  
  if( (ss_vid == "*") && (ss_pid == "*") ) return false;
  else if( (ss_vid != "*") && (ss_pid == "*") ) return (lhs.ss_vid == ss_vid);
  else if( (ss_vid == "*") && (ss_pid != "*") ) return (lhs.ss_pid == ss_pid);
  else return ( (lhs.ss_vid == ss_vid) && (lhs.ss_pid == ss_pid) );
  
  return false;

} 

void filt_device_list(std::vector<pcie_device_id >& devices, std::string ss_vid, std::string ss_pid)
{
  std::remove(devices.begin(), devices.end(), std::make_pair(ss_vid, ss_pid) );
}


std::vector<pcie_device_id > get_pci_devices( std::string inp) /*input to this function looks like a:b:c:d */
{
 //temporary results
 std::vector<pcie_device_id > temp;

 //first element is filter parameter next one is the regexa
 const size_t size = 5;
 const std::pair<std::string, std::string> p[size] = {
  {"^Slot",   "\\s+.+"},
  {"^Vendor", "\\[.+\\]"},
  {"^Device", "\\[.+\\]"},
  {"^SVendor","\\[.+\\]"},
  {"^SDevice","\\[.+\\]"}
 };
 auto lspci = [](std::string vid, std::string pid, std::string filt)->std::string{
             std::string key = "lspci -vmm -nn -d " +
             vid+":"+ pid+ " | grep "+filt;
             std::string cli_results = exec( key.c_str() );
             return cli_results;
           };
 //TBD: Input is a:b:c:d and looks for in lscpi list
 auto vidpids = split_str<':'>(inp);
 //changes asterisks to blanks
 for(auto& id : vidpids) if(id == "*" ) id.clear();
 //fill the rest with spaces just to be sure 
 while (vidpids.size() < 4 ) vidpids.push_back("");
 //bind two parameters (vid:pid to lspci)
 auto lspci_b = std::bind(lspci, vidpids[0], vidpids[1], std::placeholders::_1);

 for(int i=0; i < size; i++)
 {
   //retrieve data
   auto results = split_str<'\n'>(lspci_b( p[i].first ));
   temp.reserve( results.size() );
   temp.resize( results.size() );

   int j = 0;
   for(auto entry : results )
   { 
     std::regex reg(p[i].second);
     std::smatch mat;
     std::string* tt;

     if (std::regex_search(entry, mat, reg)){
         std::string t = mat[0];
         switch(i){
           case 0: tt = &temp[j].bus_id; break;
           case 1: tt = &temp[j].vid;    break;
           case 2: tt = &temp[j].pid;    break;
           case 3: tt = &temp[j].ss_vid; break;
           case 4: tt = &temp[j].ss_pid; break;
         }

      std::regex_search(t, mat, std::regex("[0-9A-Za-z\\.\\:]+"));
      //this is a reference to the temp[j] specific tuple el
      *tt = mat[0];
         
     }
     else std::cout << "No match found [" <<i<<j<<"]"<<std::endl;
 
     j++; 
   }
 }

 //filter out all the unwanted devices
 filt_device_list(temp, vidpids[2], vidpids[3] );

 for(auto item : temp)
   std::cout << item.bus_id << "--" << item.vid << "--" << item.pid << "--" <<
                item.ss_vid << "--" << item.ss_pid << std::endl;

 return temp;
}


void ap_usage(std::vector<std::string> keys, ArgumentParser & ap, bool final_arg)
{
  for(auto key : keys)
  {
    std::string alt_key = std::string("--") + key;
    ap.addArgument(alt_key, 1);

  }

  if( final_arg )
    ap.addFinalArgument("remainder");
}


template<typename ...Ts>
int get_input_params(const int argc, const char *argv[], bool final_arg,  Ts& ... args)
{

  ArgumentParser parser;
  auto initList = {std::ref(args)...};
  using T = typename decltype(initList)::value_type;
  std::vector<T> vec { initList };
  std::vector<std::string> keys { args... };
 
  //add keys to the parser
  ap_usage(keys, parser, final_arg);
  //extract from command line
  parser.parse(argc, argv);

  for(auto entry : vec)
  {
    auto& item = entry.get();
    item = parser.retrieve<std::string>(item);
  }

  return 0;

}

bool check_complete( std::string val, ulong size)
{
  if( size > 1 )
    return false;
  if( val == "END_COMMAND" )
    return true;
  else
    return false; 
}

int no_response_pkt( zmq::multipart_t & mp_msg)
{
    mp_msg.clear();
    mp_msg.addtyp<ushort>(0);
    mp_msg.addtyp<ulong>(1);
    mp_msg.addstr("NO_RESPONSE_PKT");
    return 0; 
}

bool check_no_response_pkt( zmq::multipart_t & mp_msg)
{
    std::cout << "check_no_response: " << mp_msg <<std::endl;
    ushort type = mp_msg.poptyp<ushort>();
    ulong  len  = mp_msg.poptyp<ulong>();
    auto ret_normal = [&](){
      mp_msg.addtyp<ushort>(type);
      mp_msg.addtyp<ulong>(len);
    };

    if( type != 0)
    { 
      //adding back piece of the message
      //that was popped out
      ret_normal();
      std::cout << "1)check_no_response: " << mp_msg <<std::endl;
      return false;
    }  
    else
    {
      std::string label = mp_msg.popstr();
      if( label != "NO_RESPONSE_PKT" )
      {
        //adding the data and types
        mp_msg.addstr(label);
        ret_normal();
        std::cout << "2)check_no_response: " << mp_msg <<std::endl;
        return false;
      }
      else
      { 
        //case when indeed it is a no response
        mp_msg.clear();
        return true;
      }
          
    }
}

int build_dummy_nex_header(std::string method, zmq::multipart_t & mp_msg)
{
  mp_msg.addstr("DUMMY_MPIRIN_ID"); 
  mp_msg.addstr("DUMMY_PROC_ID"); 
  mp_msg.addstr(method); 
  return 0;
}

int build_nex_header(zmq::multipart_t & mp_msg, std::initializer_list<std::string> args)
{
  std::vector<std::string> input = args;
  //input[0] = jobId
  mp_msg.addstr(input[0]);
  //input[1] = global_id 
  mp_msg.addstr(input[1]);
  //input[2] = method_name 
  mp_msg.addstr(input[2]); 
  return 0;
}

int build_nex_section_hdr(zmq::multipart_t& mp_msg, 
                          zmsg_section_type type, ulong len)
{
  mp_msg.addtyp<ushort>((ushort) type);
  mp_msg.addtyp<ulong>(len);
  return 0;
}

template <typename T >
int build_nex_section(zmq::multipart_t& mp_msg, std::list<T> payload)
{
  std::for_each(payload.begin(), payload.end(),
                [&](T entry){ mp_msg.addtyp<T>(entry); } );

  return 0;
}
int get_mpi_dtype_size( uint mpi_type)
{
  int size=4;
  //MPI_Type_size((MPI_Datatype) mpi_type, &size);
  return size;
}

template <typename T>
zmsg_section_type find_section_type()
{
  zmsg_section_type conv;

  if( std::is_same<T, std::string>::value ) 
    conv = zmsg_section_type::STRING;
  else if( std::is_same<T, long>::value ) 
    conv = zmsg_section_type::LONG;
  else if( std::is_same<T, short>::value ) 
    conv = zmsg_section_type::SHORT;
  else if( std::is_same<T, int>::value ) 
    conv = zmsg_section_type::INT;
  else if( std::is_same<T, ulong>::value ) 
    conv = zmsg_section_type::ULONG;
  else if( std::is_same<T, float>::value ) 
    conv = zmsg_section_type::FLOAT;
  else if( std::is_same<T, double>::value ) 
    conv = zmsg_section_type::DOUBLE;
  else if( std::is_same<T, bool>::value ) 
    conv = zmsg_section_type::BOOL;
  else if( std::is_same<T, void*>::value ) 
    conv = zmsg_section_type::MEMBLK;
  else if ( std::is_same<T, zmq::message_t>::value )
    conv = zmsg_section_type::ZMSG;

  return conv;
}

void insert_completion_tag(zmq::multipart_t & msg)
{
  msg.addtyp<ushort>(0);
  msg.addtyp<ulong>(1);
  msg.addstr("END_COMMAND");
}

std::string get_hostname()
{
  std::string str;
  str = exec("hostname");
  str.erase(std::remove(str.begin(), str.end(), '\n'), str.end());
  return str;
}

bool check_retransmission_pkt(const zmq::multipart_t & msg)
{

  return false;
}

int retransmission_pkt(zmq::multipart_t & msg)
{

  return 0;
}

template<typename K, typename V>
void print_mmap(std::ostream& out, std::multimap<K,V> const &m)
{
  out<< std::endl;
  for (std::pair<K,V> const& entry: m) 
  {
    if constexpr( std::is_same_v<decltype(entry.first),std::string> ) 
      out << "{ " << entry.first <<": " <<  entry.second << " }" << std::endl;
    else
      out << "{ " <<  static_cast<ushort>(entry.first) <<": "<<  entry.second <<" }" << std::endl;
  }
}

template<typename K, typename V>
void print_map(std::ostream& out, std::map<K,V> const &m)
{
  out<< std::endl;
  for (std::pair<K,V> const& entry: m) 
  {
    if constexpr( std::is_same_v<decltype(entry.first),std::string> ) 
      out << "{ " << entry.first <<": " <<  entry.second << " }" << std::endl;
    else
      out << "{ " <<  static_cast<ushort>(entry.first) <<": "<<  entry.second <<" }" << std::endl;
  }
}

void sanitize_map( std::map<std::string, std::string>& inout_map )
{
  std::for_each( inout_map.begin(), inout_map.end(),
                [&](auto d_entry)
  {
      auto& val = inout_map[d_entry.first];
      val.erase( std::remove( val.begin(), val.end(), '\"' ), val.end() );
  } );
}

//this function will handle both map and multimap
template<typename K, typename V>
std::list<std::string> flattened_map(const var_maps<K,V>& vmap)
{
  std::list<std::string> ll;
  std::string skey;

  std::visit([&](auto&& map) {

   auto comma_sep = [&]( std::string str, 
                         typename std::remove_reference_t<decltype(map)>::value_type val_pair )
    {
      return std::move(str) + "," + val_pair.second;
    };

   for(auto bIter = map.begin(), eIter = map.end(); bIter != eIter;
        bIter = map.upper_bound( bIter->first )  )
    {

      auto vals = map.equal_range( bIter->first );
      auto first = vals.first;
      
      auto key   = first->first;
      auto value = first->second;
 
      if constexpr( std::is_same_v<K, valid_platform_header> ) 
        skey = g_plat_map.at(key);
      else if constexpr(std::is_same_v<K, valid_function_header> ) 
        skey = g_func_map.at(key);
      else if constexpr(std::is_same_v<K, valid_host_header> ) 
        skey = g_host_map.at(key);
      else
        skey = key;
   
      std::string line = std::accumulate( std::next(vals.first), 
                                          vals.second,
                                          std::string( skey + "=" + value ),
                                          comma_sep );
      ll.push_back(line); 
    } //end of for loop
        

  }, vmap); //end of visit

  return ll;
} 



template void print_map<std::string, std::string>(std::ostream&, std::map<std::string, std::string> const&);
template void print_mmap<std::string, std::string>(std::ostream&, std::multimap<std::string, std::string> const&);
template void print_mmap<vhh, std::string>(std::ostream&, std::multimap<vhh, std::string> const&);
template void print_mmap<vfh, std::string>(std::ostream&, std::multimap<vfh, std::string> const&);
template void print_mmap<vph, std::string>(std::ostream&, std::multimap<vph, std::string> const&);

template int get_input_params<std::string >
                             (const int, const char *[], bool, std::string&, std::string&);
template int get_input_params<std::string >
                             (const int, const char *[], bool, std::string&, std::string&, std::string&);
template int get_input_params<std::string >
                             (const int, const char *[], bool, std::string&, std::string&, std::string&,
                              std::string&);
template int get_input_params<std::string >
                             (const int, const char *[], bool, std::string&, std::string&, std::string&,
                              std::string&, std::string&);
template int get_input_params<std::string >
                             (const int, const char *[], bool, std::string&, std::string&, std::string&,
                              std::string&, std::string&, std::string&);

template zmsg_section_type find_section_type<std::string>();
template zmsg_section_type find_section_type<bool>();
template zmsg_section_type find_section_type<int>();
template zmsg_section_type find_section_type<short>();
template zmsg_section_type find_section_type<ulong>();
template zmsg_section_type find_section_type<float>();
template zmsg_section_type find_section_type<double>();
template zmsg_section_type find_section_type<void*>();
template zmsg_section_type find_section_type<zmq::message_t>();

template std::list<std::string> flattened_map<std::string, std::string>(const var_maps<std::string, std::string>& );
//template std::list<std::string> flattened_map<vph, std::string>(const var_maps<vph, std::string>& );
template std::list<std::string> flattened_map<vfh, std::string>(const var_maps<vfh, std::string>& );
//template std::list<std::string> flattened_map<vhh, std::string>(const var_maps<vhh, std::string>& );
