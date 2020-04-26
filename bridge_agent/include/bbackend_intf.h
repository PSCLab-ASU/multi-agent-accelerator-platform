#include <zmq.hpp>
#include <zmq_addon.hpp>
#include <iostream>
#include <sys/types.h>
#include <unistd.h>
#include <list>
#include <tuple>
#include <utils.h>
#include <zmsg_builder.h>
#include <zmsg_viewer.h>

//algo_type
//datatype
//maxslots
//active_slots
//vendor_id
//prod_id
//version_id
//allocation_id
class bbackend_intf
{
  public:
    bbackend_intf();
    bbackend_intf(std::string addr);
    ~bbackend_intf();
    
    int init_job(std::string, ulong, ulong,
                 std::list<std::string>,
                 std::list<std::string>,
                 std::list<std::string>);

    int init_rank(std::string, ulong );
    int claim_rank( std::list<std::string>, std::list<std::string>, ulong * );
 

  private:
    void _add_req_header( std::string, zmq::multipart_t *);

    std::string res_addr;
    std::string job_id;
    ulong this_rank;
    zmq::context_t ctx;
    zmq::socket_t * backend;

};


