#include <bbackend_intf.h>

bbackend_intf::bbackend_intf()
{
 ctx = zmq::context_t();
 backend = new zmq::socket_t(ctx, ZMQ_DEALER);
}

bbackend_intf::bbackend_intf(std::string addr) : bbackend_intf()
{
  res_addr = addr; 
}

bbackend_intf::~bbackend_intf()
{
  delete backend;
}

int bbackend_intf::init_job(std::string mpirun_id, ulong src_rank, ulong max_cpu_cnt,
                            std::list<std::string> nex_addrs,  
                            std::list<std::string> func_lookup,
                            std::list<std::string> meta_data) 
{

  /////////////////////////////////////////////////
  //save member variable
  /////////////////////////////////////////////////
  job_id = mpirun_id;
  this_rank = src_rank;
  /////////////////////////////////////////////////
  //TODO: temporarily setup zmq::identity here/////
  /////////////////////////////////////////////////
  std::string id = mpirun_id + std::string("_") + std::to_string(src_rank);
  backend->setsockopt(ZMQ_IDENTITY, id.c_str(), id.length());
  std::cout << "connecting to " << res_addr <<std::endl;
  backend->connect(res_addr);

  zmsg_builder zmbuilder(
                 job_id,
                 std::to_string(this_rank),
                 std::string("__NEX_INITJOB__") );

  zmbuilder.add_arbitrary_data<bool>(true)
           .add_arbitrary_data<ulong>(max_cpu_cnt)
           .add_sections<std::string>(nex_addrs)
           .add_sections<std::string>(func_lookup)
           .add_sections<std::string>(meta_data)
           .finalize();
  
  std::cout << "sending msg = " << zmbuilder.get_zmsg() << std::endl;
  //send command to resource manager
  zmbuilder.get_zmsg().send(*backend);
  //zmbuilder.get_zmsg().recv(*backend);

  return 0;
}


int bbackend_intf::init_rank(std::string mpirun_id, ulong src_rank)
{

  zmq::multipart_t req, reply;
  //SEND A REQ to the resmgr_frontend (on resource manager side)
  //AKA the RESOURCE MGR ROUTER
  _add_req_header("__REM_JINIT__", &req);
  ///////////////////////////////////////
  //PAYLOAD SECTION//////////////////////
  //add end tags
  insert_completion_tag( req );
  //////////////////////////////////////
  //////////////////////////////////////

  //send command to resource manager
  req.send(*backend);

  //get response
  reply.recv(*backend);
  //std::cout <<"Response from resource manager: " << reply<<std::endl;
  return 0;
}

int bbackend_intf::claim_rank( std::list<std::string> funcs, 
                               std::list<std::string> meta, 
                               ulong * new_rank)
{

  zmsg_builder zmbuilder(
                 job_id,
                 std::to_string(this_rank),
                 std::string("__REM_CLAIM__") );
 
  zmbuilder.add_sections<std::string>(funcs)
           .add_sections<std::string>(meta)
           .finalize();

  std::cout << "sending msg = " << zmbuilder.get_zmsg() << std::endl;
  //send command to resource manager
  zmbuilder.get_zmsg().send(*backend);
  std::cout << "Reading claim information" << std::endl;
  zmbuilder.get_zmsg().recv(*backend);
  zmq::multipart_t rep;
  std::cout << "recv = " << zmbuilder.get_zmsg() << std::endl;
  std::cout << "**************************************" << std::endl;
  return 0;
}

void bbackend_intf::_add_req_header(std::string method, zmq::multipart_t * mp)
{
  mp->addstr(job_id);
  mp->addstr( std::to_string(this_rank) );
  mp->addstr( method );
}

