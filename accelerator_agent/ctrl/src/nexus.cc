#include <nexus.h>
#include <zmsg_builder.h>

#define BIND_ACTION(action)            \
  std::bind(&nexus::action,\
            this,                      \
            std::placeholders::_1);

#define DUMMY_JOBID std::string("DUMMY_JOBID1")
#define DUMMY_PROC  std::string("0")


nexus::nexus()
{
  //main resource allocation methods
  command_set["rollcall"] = BIND_ACTION(_nexctl_rollcall_);
  command_set["queryres"] = BIND_ACTION(_nexctl_queryres_);
  command_set["claim"] = BIND_ACTION(_nexctl_claim_);
  command_set["DEFAULT"]  = BIND_ACTION(_nexctl_default_);
}

nexus::nexus(zmq::context_t * zmq_context, std::string port,
             std::string params) :
  nexus()
{
  //define name 
  nex_name = "nexusctrl";

  //connection to data channel (bidirectional)
  //eternal connection
  zcontext     = zmq_context;
  //nexus port
  nx_port      = port;
  //current parameters - TBD convert string to map
  auto keyval = split_str<','>(params);

  for( auto itm : keyval )
  {
    auto el = split_str<'='>(itm);
  
    if( el.size() > 1 )
      current_params.insert( make_pair(el[0], el[1]) );
  }
}

nexus::~nexus()
{
 std::cout << "Calling nexus destructor" <<std::endl; 
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
////////////////////////////INTERNAL API//////////////////////////////////
//the purpose of this function is to send a connection request to the
//remote nexus - only spreads the node 0 word to all the nexuses
template<size_t N>
int nexus::_broadcast_nex_command(std::string method, std::array<extra_params, N> inp)
{
  //setup multipart msga
  ulong num_responses = nxhost_set.size();
  zmsg_builder mBuilder(method, generate_random_str() );

  
  std::for_each(inp.begin(), inp.end(), [&](auto in) 
  {
     if( in )
      mBuilder.add_sections(in.value());
  } );
 
  mBuilder.finalize();

  //cycles through all the sockets configure
  zmq::multipart_t msg = std::move(mBuilder.get_zmsg());
  for(auto& target : nxhost_set )
  {
    std::cout <<target.host_addr << ": " <<"Broadcasting (" << method << ")" << std::endl;
    msg.send( const_cast<nxhost&>(target).z_sock );
  } 
  return 0;
}

int nexus::_gather_nex_results()
{
  //get all the responsesa
  zmq::multipart_t msg;
  ulong num_responses = nxhost_set.size();

  msg.clear();
  while ( num_responses )
  {
    for(auto& target : nxhost_set )
    {
      msg.recv(const_cast<nxhost&>(target).z_sock, ZMQ_NOBLOCK);
      if( !msg.empty() )
      {
        std::cout <<"Addr = " << target.host_addr << std::endl;
        std::cout << msg << std::endl;
        num_responses--;
      }
    }
  }
  return 0;
}
template<size_t N>
int nexus::_broadcast_gather(std::string method, std::array<extra_params,N>  parms)
{
  _broadcast_nex_command(method, parms);
  _gather_nex_results();
  return 0;
}
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
////////////////////////////EXZTENRAL API/////////////////////////////////
nexus::action_func nexus::action_ctrl(std::string method, std::string host_list)
{
  auto hosts = split_str<','>(host_list);
  
  for(auto& host : hosts)
  { 
    //adds the default port number in case none are provided
    if( host.find(":") == std::string::npos ) 
      host = host + ":" + nx_port; 
    //this call create AND connects a zmq::socket_t
    nxhost_set.emplace( zcontext, nex_name, host );
  }

  auto func = command_set[method];
  if( func == nullptr ) func = command_set["DEFAULT"];
 
  return func;
}

int nexus::_nexctl_default_(InputType inp)
{
  return 0;
}

int nexus::_nexctl_rollcall_(InputType inp)
{
  //_broadcast_gather("__HW_ROLLCALL__", std::array<extra_params,0>{} );
  _broadcast_nex_command("__HW_ROLLCALL__", std::array<extra_params,0>{} );
  return 0;
}

int nexus::_nexctl_queryres_(InputType inp)
{
  _broadcast_gather("__HW_QUERYRES__", std::array<extra_params,0>{});
  return 0;
}

int nexus::_nexctl_claim_(InputType inp)
{
  //fill TBD!!!!
  std::string claim;
  print_map(std::cout, current_params);
  claim = std::accumulate(current_params.begin(), current_params.end(), std::string(""),
          [](const std::string acc, InputType::value_type el)
          {
            return el.first + "=" + el.second + ";";
          } ); 
                  
  std::cout << "claim issue: " << claim << std::endl;
  extra_params sect1 = std::list(1, claim);
  extra_params sect2 = std::list(1, std::string("") );

  _broadcast_gather("__REM_CLAIM__", std::array{sect1, sect2} );
  return 0;
}

