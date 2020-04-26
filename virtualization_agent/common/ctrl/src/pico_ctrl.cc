#include <cstdlib>
#include <ctime>
#include "utils.h"
#include <pico_ctrl.h>
#include <pico_utils.h>
#include <zmsg_builder.h>
#include <random>

#define BIND_ACTION(action)            \
  std::bind(&pico_ctrl::action,\
            this);

pico_ctrl::pico_ctrl()
{
  //main resource allocation methods
  command_set["sample"]   = BIND_ACTION(_picctl_sample_);
  command_set["fsample"]  = BIND_ACTION(_picctl_fsample_);
  command_set["DEFAULT"]  = BIND_ACTION(_picctl_default_);
}

pico_ctrl::pico_ctrl(zmq::socket_t * rtr,
                     zmq::socket_t * pub,
                     std::string claim_id, 
                     std::string stim_file, int nargs) 
: pico_ctrl()
{
  rtr_zsock = rtr;
  pub_zsock = pub;

  std::string name = "pico_ctrl";
  //save the stim file
  defintion_file = stim_file;
  //same the sample claimID
  claim = claim_id;
  //args
  n_args = nargs;

  //get the name of the pico_service
  _rollcall();

}

pico_ctrl::~pico_ctrl()
{  
  std::cout << "Calling pico_ctrl destructor" <<std::endl; 
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
////////////////////////////INTERNAL API//////////////////////////////////
int pico_ctrl::_send(zmq::multipart_t & zmsg, bool pub_dealer )
{
  std::cout << "sending : " << zmsg << std::endl;
  if (pub_dealer) zmsg.send( *rtr_zsock );
  else zmsg.send( *pub_zsock );
  return 0;
}

int pico_ctrl::_recv( zmq::multipart_t& zmsg)
{
  std::cout << "recv : " << zmsg << std::endl;
  zmsg.recv( *rtr_zsock );

  return 0;
}

//send command to pico service
int pico_ctrl::_sendrecv_command(zmq::multipart_t& szmsg, zmq::multipart_t& rzmsg, bool pub_dealer )
{
  _send( szmsg, pub_dealer );
  _recv( rzmsg );
  return 0;
}

int pico_ctrl::_rollcall( )
{
  std::cout << "entering "<< __func__ << std::endl;
  zmq::multipart_t rmzsg;
  std::string key = generate_random_str();
  auto szbmsg = pico_utils::start_request_message( pico_utils::pico_ctrl::reg_resource, key );
  //add destination
  szbmsg.add_arbitrary_data( std::string("ALL") );
  szbmsg.finalize();

  _sendrecv_command( szbmsg.get_zmsg(), rmzsg, false );

  auto zmsgv = zmsg_viewer<std::string, std::string, std::string>( rmzsg );

  auto[k, method, name] = zmsgv.get_header();

  pico_name = name;
  std::cout << "pico_name = " << pico_name << std::endl;
  return 0;
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
////////////////////////////EXZTENRAL API/////////////////////////////////
pico_ctrl::action_func pico_ctrl::action_ctrl(std::string method)
{
  auto func = command_set[method];
  if( func == nullptr ) func = command_set["DEFAULT"];
 
  return func;
}

int pico_ctrl::_picctl_default_()
{
  std::cout << "entering " << __func__ << std::endl;

  return 0;
}

int pico_ctrl::_picctl_sample_()
{
  std::cout << "entering " << __func__ << std::endl;
  using vfloat = std::vector<float>;

  std::string key = generate_random_str();
  auto zbmsg = pico_utils::start_request_message( pico_utils::pico_ctrl::send, key );
  //add destionation
  zbmsg.add_raw_data_top( pico_name );
  //add claim information  
  zbmsg.add_arbitrary_data( claim );
  //add number of args
  zbmsg.add_arbitrary_data((ulong) n_args );

  auto matrix = std::vector<vfloat>(n_args); 
 
  std::for_each(matrix.begin(), matrix.end(), [&] ( auto& row )
  {
    std::random_device rd;  
    std::mt19937 gen(rd());
    std::uniform_real_distribution<> dis(0., 1.0);
    size_t len = 100*dis(gen) + 1; 
    //randomly fill in data
    std::cout << "len = " << len << std::endl;
    size_t i = len;
    while( i-- ) { row.push_back( dis(gen) ); }
    std::cout << "row.length() = " << row.size() << std::endl;
    //add it to the message
    zbmsg.add_memblk(true, 1, sizeof(float), row.data(), len); 
  } );

  //send message
  zmq::multipart_t rzmsg;
  _sendrecv_command( zbmsg.get_zmsg(), rzmsg, true );

  std::cout << "recv_msg : " << rzmsg << std::endl;

  return 0;
}

int pico_ctrl::_picctl_fsample_()
{
  std::cout << "entering " << __func__ << std::endl;


  return 0;
}

