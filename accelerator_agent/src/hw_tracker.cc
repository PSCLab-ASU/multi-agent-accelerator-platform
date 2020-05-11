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

