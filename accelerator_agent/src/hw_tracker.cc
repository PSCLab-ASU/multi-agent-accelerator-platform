#include <map>
#include <zmq.hpp>
#include <zmq_addon.hpp>
#include <functional>
#include <iostream>
#include <algorithm>
#include <hw_tracker.h>
#include <utils.h>

hw_tracker::hw_tracker(){}

hw_tracker::~hw_tracker(){}

hw_tracker::hw_tracker( std::string id, std::string zaddr, zmq::context_t * zctx) 
: id(id), addr(zaddr), _ctx()
{
  //zmq context
  _ctx = std::shared_ptr<zmq::context_t>(zctx);
  //create zmq socket
  std::string home = std::string("nexus-") + get_hostname();
  _zlink = std::make_shared<zmq::socket_t>(*_ctx.get(), ZMQ_DEALER);
  _zlink->setsockopt(ZMQ_IDENTITY, home.c_str(), home.length() );
  //connect socket
  std::string address = std::string("tcp://") + addr;
  _zlink->connect(address);
}

int hw_tracker::generate_header( std::string method, zmq::multipart_t & msg)
{
  msg.addstr(id);     // address
  msg.addstr("");     //zmq protocol : blank
  msg.addstr(method); //method
  return 0;
}

int hw_tracker::request_rank( std::string job_id, std::string owning_rank,
                              std::tuple<ulong, ulong, std::string> gr_gl_func_id,
                              std::list<std::string> extra_params )
{
  zmsg_builder mbuilder( job_id, owning_rank,
                         std::string("__PI_CLAIM__") );

  mbuilder.add_arbitrary_data<ulong>( std::get<0>( gr_gl_func_id ) )
          .add_arbitrary_data<ulong>( std::get<1>( gr_gl_func_id ) )
          .add_arbitrary_data<std::string>( std::get<2>( gr_gl_func_id ) )
          .add_sections<std::string>(extra_params)
          .finalize();
  std::cout << "sending data to pico service :" << mbuilder.get_zmsg() << std::endl;
  _send_msg( mbuilder.get_zmsg() );

 return 0;
}

int hw_tracker::_send_msg( const zmq::multipart_t& msg) const
{
  zmq::multipart_t lmsg = msg.clone();

  lmsg.send(*_zlink);
  return 0;
} 

int hw_tracker::_recv_msg( zmq::multipart_t& msg)
{
  msg.send(*_zlink);
  return 0;
} 

bool operator==(const hw_tracker & lhs, const std::string hw_tr_id)
{
  return lhs.id == hw_tr_id;
}

bool operator==(const shwe & lhs, const std::pair< std::vector<std::string>, HW_SEARCH_TYPES::FIND_HW_ELM> val)
{
  //has to match the hw_vendor_desc, and the hw_sub_vendor_desc
  bool result = ((lhs.hw_vendor_desc.first == val.first[0]) && (lhs.hw_vendor_desc.second == val.first[1])) &&
                ((lhs.hw_sub_vendor_desc.first == val.first[2]) && (lhs.hw_sub_vendor_desc.second == val.first[3]));
  return result;
}
 
bool operator==(const sswe& lhs, const std::pair< std::vector<std::string>, HW_SEARCH_TYPES::FIND_SW_ELM> val)
{
  bool result = (lhs.sw_vidpid.first == val.first[0] && lhs.sw_vidpid.second == val.first[1]);
  return result;
} 

bool operator==(const sfunce & lhs, const std::pair< std::multimap<std::string, std::string>, HW_SEARCH_TYPES::FIND_FUNC_ELM> val)
{
  bool result = true; //((lhs.sw_fidverid.first == val.first[0]) && (lhs.sw_fidverid.second == val.first[1]));
  return result;
} 

bool operator==(const sfunce & lhs, const sfunce & rhs)
{
  bool bIds = (lhs.sw_fidverid == rhs.sw_fidverid);
  bool bFunc_name = (lhs.func_name == rhs.func_name);
  bool bMathML_desc = (lhs.mathML_desc == rhs.mathML_desc);
  bool bRepo  = (lhs.repo_loc == rhs.repo_loc);
  bool bFileName = (lhs.filename == rhs.filename);
  bool bFullPath = (lhs.full_path == rhs.full_path);
  
  return bIds && bFunc_name && bMathML_desc && bRepo
              && bFileName && bFullPath;
}

int sfunce::import(std::multimap<std::string, std::string>& mmap)
{
  
  //TBD //TBD//TBD//TBD
  //make sure to put owner in the maoaa
  repo_loc  = mmap.find("path")->second; 
  filename  = mmap.find("fname")->second;
  full_path = repo_loc + "/" + filename;
  auto ids  = split_str<':'>( mmap.find("id")->second );
  ////////////////////////////////////////////////
  sw_fidverid = std::make_pair(ids[6],ids[7]);
  ////////////////////////////////////////////////
  auto p = mmap.equal_range("mac_addrs");
  for(auto i = p.first; i != p.second; i++)
    ip_mac_addrs.push_back(i->second);
  ///////////////////////////////////////////////

  return 0;
}

int sfunce::get_full_funcId(std::string & id)
{
  std::string fid      = sw_fidverid.first; 
  std::string verid    = sw_fidverid.second;
  std::string sw_vid   = parent->sw_vidpid.first;
  std::string sw_pid   = parent->sw_vidpid.second;
  std::string ss_vid   = parent->parent->hw_sub_vendor_desc.first;
  std::string ss_pid   = parent->parent->hw_sub_vendor_desc.second;
  std::string vid      = parent->parent->hw_vendor_desc.first;
  std::string pid      = parent->parent->hw_vendor_desc.second;
  std::string pico_id  = parent->parent->parent->id;
  bool        hw_state = parent->parent->hw_state;
  ulong       num_devs = parent->parent->NumDevices;

  id = pico_id + ":" + vid + ":" + pid + ":" + ss_vid + ":" + ss_pid + ":" +
       sw_vid + ":" + sw_pid + ":" + fid + ":" + verid + ":" +
       std::to_string(hw_state) + ":" + std::to_string(num_devs);
 
  std::cout << "Cache ID : " << id << std::endl;

  return 0;
} 

shwe& hw_tracker::find_create_elem( std::vector<std::string> ids)
{
  auto hw_lookup = HW_SEARCH_TYPES::FIND_HW_ELM::BY_HWALL;  //searches for both a:b:c:d
  auto hweIt = std::find(hw_elements.begin(), hw_elements.end(), std::make_pair(ids ,hw_lookup));

  if( hweIt == hw_elements.end())
  {
    shwe lhwe = shwe(ids[0], ids[1], ids[2], ids[3]);
    lhwe.parent = this;
    //add to the list of hw_references
    hw_elements.push_back(lhwe);
    //returne the reference from std::vector
    return hw_elements.back();
  }
  else
  {
    hweIt->parent = this;
    return *hweIt;  //found entry

  }
 
}

sswe& shwe::find_create_elem( std::vector<std::string> ids)
{
  auto sw_lookup = HW_SEARCH_TYPES::FIND_SW_ELM::BY_SWALL; 
  auto sweIt = std::find(sw_elements.begin(), sw_elements.end(), std::make_pair(ids, sw_lookup));

  if( sweIt == sw_elements.end() )
  {
    //did not find any sw_vid, or sw_pid
    sswe lswe = sswe(ids[0], ids[1]);
    lswe.parent = this;
    sw_elements.push_back(lswe);
    return sw_elements.back();
  }
  else
  { 
    sweIt->parent = this;
    return *sweIt;
  } 

}

sfunce& sswe::find_create_elem( std::multimap<std::string, std::string> mmap)
{
  sfunce lsfunce= sfunce();
  lsfunce.import(mmap); //TBD: Implement import
  
  auto sfunceIt = std::find(func_elements.begin(), func_elements.end(), lsfunce); //TBD implement operator==
  
  if( sfunceIt == func_elements.end() )
  {
    lsfunce.parent = this;
    func_elements.push_back(lsfunce);
    return func_elements.back();
   
  }
  else
  {
    sfunceIt->parent = this;
    return *sfunceIt;
  }

}
