#include <pico_helper.h>


int ps_create_msg_header ( std::string addr, std::string method, zmq::multipart_t * msg)
{


 return 0;
}


int ps_create_identify_resource_msg( zmsg_builder<std::string, std::string, std::string, std::string> && mb, 
                                     bool checkin, std::vector<std::string > repos, zmq::multipart_t * msg)
{
 std::list<std::string> repo_v( repos.begin(), repos.end() );
  mb.add_arbitrary_data( checkin )
    .add_sections(repo_v)
    .finalize();
 
  *msg = std::move( mb.get_zmsg() );

 return 0;
}
