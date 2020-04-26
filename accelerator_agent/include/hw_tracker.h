#include <map>
#include <zmq.hpp>
#include <zmq_addon.hpp>
#include <functional>
#include <iostream>
#include <list>
#include <memory>
#include <zmsg_builder.h>

#ifndef HWTRACK
#define HWTRACK

typedef struct _HW_SEARCH_TYPES
{
  //find the pico service
  enum struct FIND_WRAPPER { BY_ID=0, BY_OWNER, BY_IDOWNER};
  //find the HW element
  enum struct FIND_HW_ELM { BY_HWVID=0, BY_HWPID, BY_HWVIDPID,
                            BY_HWSSVID, BY_HWSSPID, BY_HWSSVIDPID,  BY_HWALL  };
  //find the SW element
  enum struct FIND_SW_ELM { BY_SWVID=0, BY_SWPID, BY_SWVIDPID,
                            BY_FILENAME, BY_REPO, BY_SWALL};
  //find the function element
  enum struct FIND_FUNC_ELM { BY_FID=0, BY_VERID, BY_FIRVERID };

} HW_SEARCH_TYPES;

class hw_tracker {
  public: 
  hw_tracker();
  hw_tracker(std::string, std::string, zmq::context_t *);
  ~hw_tracker();
 
  friend bool operator==(const hw_tracker &, const std::string );

  //generate envelope for target HW service
  int generate_header( std::string, zmq::multipart_t & );
  //address for the local hardware resourcea
  //each struct points to a nexus addr : zmq_identifier
  std::string id;
  std::string addr;
  


  typedef struct _singl_hw_elem {

    _singl_hw_elem(){
       hw_vendor_desc = hw_sub_vendor_desc = std::make_pair("","");
       hw_state       = false;
       NumDevices     = 0;
    }

    _singl_hw_elem(std::string vid, std::string pid,
                   std::string ss_vid, std::string ss_pid, bool state=false, ulong num_devices=0){
       hw_vendor_desc     = std::make_pair(vid, pid);
       hw_sub_vendor_desc = std::make_pair(ss_vid, ss_pid);
       hw_state           = state;
       NumDevices         = num_devices;
    }
    //overloaded operator
    friend bool operator==(const _singl_hw_elem &, 
                           const std::pair< std::vector<std::string>, HW_SEARCH_TYPES::FIND_HW_ELM>);
 

    //parent pointer
    hw_tracker * parent;

    //vendor, product, version id
    //this is the list of vendor description
    //format: VID:PID allows list with [...] and PID can have an asterisks
    std::pair<std::string, std::string> hw_vendor_desc;
    //format: VID:PID allow list with [...] and VID:PID can have asterisks
    std::pair<std::string, std::string> hw_sub_vendor_desc; 
    //incase there are multiple pcie devices per 
    std::vector<std::string> bus_ids;
    //hardware status
    //HW: true if hardware exists, else false
    bool hw_state;
    //number of local devices attached to nexus
    ulong NumDevices;
    int set_hardware_state (bool state ){ hw_state = state; return 0; }
    int kernel_exists(bool & exist) { exist = sw_elements.size() > 0; return 0; } 
 
    //format: name of kernel file and vector:
    //vector has SW_VID, SW_PID, SW_FID, SW_VERID, SW_GID, SW_MathML
    typedef struct _singl_sw_elem {
      //parent 
      _singl_hw_elem * parent;
      //member variables
      std::pair<std::string, std::string> sw_vidpid;

      _singl_sw_elem(){
        sw_vidpid = std::make_pair("","");
      }
      _singl_sw_elem(std::string vid, std::string pid){
        sw_vidpid   = make_pair(vid, pid);
      } 
      //overloaded operators
      friend bool operator==(const _singl_sw_elem&, 
                             const std::pair< std::vector<std::string>, HW_SEARCH_TYPES::FIND_SW_ELM>);
     

      typedef struct _singl_func_elem{
           std::pair<std::string, std::string> sw_fidverid;
           std::string func_name;
           std::string mathML_desc;
           //repo location
           std::string repo_loc;
           //filename
           std::string filename;
           //local or remote path to kernel
           std::string full_path;
           //every peace of hardware might have an optional ip or mac address
           std::vector< std::string >  ip_mac_addrs;
           //parent link
           _singl_sw_elem * parent;

           void set_mathML_desc(std::string mathml) { mathML_desc = mathml; }

           _singl_func_elem(){
               sw_fidverid = std::make_pair("", "");
               func_name   = "";
               mathML_desc = "";
               repo_loc    = "";
               filename    = "";
               full_path   = "";
           };

           _singl_func_elem(std::string fid, std::string ver_id, std::string lfilename,
                            std::string fname, std::string kern_path, std::string repo) 
          {
               sw_fidverid = std::make_pair(fid, ver_id);
               func_name   = fname;
               full_path   = kern_path;
               repo_loc    = repo;
               filename    = lfilename;
           }; 
           friend bool operator==(const _singl_func_elem & lhs, const _singl_func_elem & rhs);
           friend bool operator==(const _singl_func_elem &, 
                                   const std::pair< 
                                         std::multimap<std::string, std::string>, HW_SEARCH_TYPES::FIND_FUNC_ELM>); 

           int import(std::multimap<std::string, std::string> &);
           int get_full_funcId(std::string &);

       } singl_func_elem;
       std::list<singl_func_elem> func_elements;
       //create of find func_elements
       singl_func_elem& find_create_elem( std::multimap<std::string, std::string> );
     
    } singl_sw_elem;
    //ONLY member function in singl_hw_elem
    std::list<singl_sw_elem> sw_elements;
    //create of find sw_elements
    singl_sw_elem& find_create_elem( std::vector<std::string> );

  } singl_hw_elem;
  //only member functoin in singl_hw_elem
  std::list<singl_hw_elem> hw_elements;
  //create of find hw_elements
  singl_hw_elem& find_create_elem( std::vector<std::string>);
  
  //mthods for hw_tracker
  int request_rank(std::string, std::string,               
                   std::tuple<ulong, ulong, std::string>,
                   std::list<std::string> );
 
  private: 
    int _send_msg( const zmq::multipart_t& ) const;
    int _recv_msg( zmq::multipart_t& );

    //connection to router
    std::shared_ptr<zmq::context_t> _ctx;
    std::shared_ptr<zmq::socket_t>  _zlink;

  //
};

typedef hw_tracker::singl_hw_elem shwe; 
typedef hw_tracker::singl_hw_elem::singl_sw_elem sswe;
typedef hw_tracker::singl_hw_elem::singl_sw_elem::singl_func_elem sfunce; 


#endif
