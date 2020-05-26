#include <boost/range/algorithm_ext/iota.hpp>
#include <boost/range/algorithm/find.hpp>
#include <client_utils.h>
#include <zmsg_builder.h>
#include <zmq.hpp>
#include <zmq_addon.hpp>

client_utils::client_ctrl client_utils::reverse_lookup( std::string val )
{
  auto pair = std::find_if( client_rolodex.begin(), client_rolodex.end(),
              [&]( auto item )
              {
                return (val == item.second);
              } );

  return pair->first;
}

zmsg_builder<std::string, std::string, std::string, std::string>
client_utils::start_routed_message( client_utils::client_ctrl pmethod, 
                                    std::optional<client_utils::client_ctrl> smethod,
                                    std::string address, std::string key )
{
  zmq::multipart_t empty;
  zmsg_builder<std::string, std::string, std::string, std::string> zempty(empty);

  std::string pmethod_str=PLACEHOLDER, smethod_str=PLACEHOLDER;

  if( client_rolodex.find ( pmethod ) != client_rolodex.end() )
    pmethod_str = client_rolodex.at( pmethod );
  else return std::move(zempty);

  if( smethod && (client_rolodex.find ( smethod.value() ) != client_rolodex.end()) )
    smethod_str = client_rolodex.at( smethod.value() );
  
  zmsg_builder<std::string, std::string, std::string, std::string> 
    zb( address, pmethod_str, smethod_str,  key);

  return std::move( zb );

}

zmsg_builder<std::string, std::string, std::string>
client_utils::reroute_waitable_message( std::string client_addr, std::string key, 
                                        client_ctrl ctrl, zmq::multipart_t&& zmsg)
{
 zmsg_builder<std::string, std::string, std::string> 
   out( std::forward<zmq::multipart_t>(zmsg) );
 out.add_raw_data_top( key,
                       client_rolodex.at(ctrl),
                       client_rolodex.at(client_utils::client_ctrl::resp),
                       client_addr);
 return out;
}

bool metadata::dst_rank_exists() const
{
  return (bool) dst_rank;
}

std::tuple<std::string, std::string, std::string, std::string, bool, bool> 
get_init_parms( int * argc, char ***  argv)
{

  uint dash_dash_pos=0;
  std::vector<int> index(*argc);
  auto vals = valid_inputs;
  char ** d_argv = *argv;

  for( auto i : boost::iota( index, 0 ) )
  {
    //after the pivot is found check key_val pair
    //format --key=val
    if( dash_dash_pos > 0 )
    { 
      //split each token, check each value pair vs. valid inputs
      auto key_val = split_str<'='>(d_argv[i]);
      //loop through each valid input 
      auto entry = vals.find(key_val[0] );
      if( entry != vals.end() ) entry->second = key_val[1];

    }
    //find pivot aka "--"
    if( strncmp(d_argv[i], ACCEL_INPUT_PIVOT, 
                ACCEL_INPUT_PIVOT_SZ) == 0 )  dash_dash_pos = i;
  
  } 

  auto asa      = vals.at( "--accel_address" );
  auto job_id   = vals.at( "--accel_job_id" );
  auto async    = vals.at( "--accel_async" );
  auto repo     = vals.at( "--accel_repo" );
  auto hfile    = vals.at( "--accel_host_file" );
  auto spawn_ba = vals.at( "--accel_spawn_bridge" );

  //return argc
  *argc = dash_dash_pos;

  //get job id
  if( job_id.empty() ) job_id = generate_random_str();

  return std::make_tuple(asa, job_id, hfile, repo,
                         async=="true"?true:false,
                         spawn_ba=="true"?true:false );
}

bool zmq_ping( ZPING_TYPE ptype, std::string addr)
{
  bool ret=false;
  std::string prefix;
  zmq::context_t ctx;
  int timeout = 15000; //5 seconds

  zmq::socket_t sock (ctx, ZMQ_DEALER);
  zmsg_builder mbuilder(std::string("PING"),
                        std::string("PING"),
                        std::string("PING"),
                        std::string("PING") );
  mbuilder.finalize();

  if( ptype == T2T ) prefix = "inproc:///";
  else if( ptype == P2P ) prefix = "ipc:///";
  else if( ptype == N2N ) prefix = "tcp:///";
  else prefix ="";

  std::string home = "ping-" + generate_random_str();
  sock.setsockopt(ZMQ_IDENTITY, home.c_str(), home.length());  

  sock.setsockopt(ZMQ_RCVTIMEO, (void *) &timeout, sizeof(int) );
  //sock.setsockopt(ZMQ_LINGER,   (void *) &timeout, sizeof(int) );
  sock.connect( prefix + addr );

  mbuilder.get_zmsg().send( sock );
  mbuilder.get_zmsg().recv( sock );

  ret = !mbuilder.get_zmsg().empty(); 

  std::cout <<" pinging " << (prefix+addr) << " : " << ret << std::endl;
  //sock.close();
  //ctx.close();

  return ret;

}

void mpi_pack_deleter( void  * pack_ptr)
{
  std::cout << "Deleted pack ptr..." << std::endl;
  free( pack_ptr );
}

void client_utils::cobj_deleter( void  * pack_ptr)
{
  std::cout << "Deleted cobj ptr..." << std::endl;
  free( pack_ptr );
}
