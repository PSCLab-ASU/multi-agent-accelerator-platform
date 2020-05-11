#include "collage.h"

int main(const int argc, const char * argv[] )
{
    std::cout << "Starting OCL RT Dealer" <<std::endl;
    std::string port="accel_port", repo="repo_dir", pub="pub_port", owner="owner", pids="pids";
    std::string ext = "ext_port";
    //modifies port and repo in place
    get_input_params(argc, argv, false, port, pub, ext, owner, pids, repo);
    //create socket
    zmq::context_t context;
    zmq::socket_t recv_sock = zmq::socket_t(context, ZMQ_ROUTER);
    zmq::socket_t ocl_sock  = zmq::socket_t(context, ZMQ_DEALER);
    zmq::socket_t pub_sock  = zmq::socket_t(context, ZMQ_SUB);
    //socket must bind to the nexsus
    std::string address = std::string("ipc:///") + port;
    std::string pub_address = std::string("ipc:///") + pub;
    std::string ext_address = std::string("tcp://*:") + ext;
    //ocl_sock.setsockopt(ZMQ_IDENTITY, z_identity, 8);
    //pub_sock.setsockopt(ZMQ_IDENTITY, z_identity, 8);
    pub_sock.setsockopt(ZMQ_SUBSCRIBE, "", 0);

    //address needs to be put in the manifest
    //the nexsus connects to this port
    std::cout << "binding ext_addr: " << ext_address << std::endl;
    recv_sock.bind(ext_address);
    std::cout << "address : " << address << std::endl;
    ocl_sock.connect(address);
    std::cout << "pub_address : '" << pub_address <<"'"<< std::endl;
    pub_sock.connect(pub_address);
    std::cout << "complete socket" << std::endl;

    //validate up to here
    collage col(100);
    //set the ownership of this pico service
    col.set_external_address( owner + ":" + ext );
    col.set_owner(owner);
    //setting repo
    col.set_repo(repo);
    //sets which vids:pids to filter out of
    //lscpi format VID:PID
    col.set_filters(pids);
    //kicking off system and device services
    col.init_services();
    //multipart request
    zmq::multipart_t recv_msg, ocl_msg, pub_msg, reply_msg;
    pico_return pico_status;
    target_sock_type tsock_t;
    bool status = false;

    auto process_msg = [&]( target_sock_type ts,
                            zmq::socket_t& sock, 
                            zmq::multipart_t& req)->bool
                       {
                         bool ret = req.recv(sock, ZMQ_NOBLOCK );
                         if( !req.empty() ) 
                         {
                           std::cout << " recieved msg :" << req <<  std::endl;
                           col.submit_request(ts, req);
                           //col.print_header();
                         }
                         return ret;
                       };

    auto process_resp = [&]( )->bool
                        {
                           zmq::multipart_t resp;
                           target_sock_type ts;
                           pico_status = col.process_q_resp( resp, ts );

                           if( ts == target_sock_type::TST_RTR )
                           {
                             std::cout << "sending out to TST_RTR :" << std::endl;
                             std::cout << resp << std::endl;

                             resp.send(recv_sock);
                           }
                           else if( ts == target_sock_type::TST_DEALER )
                           {
                             std::cout << "sending out to TST_DEALER :" << std::endl;
                             std::cout << resp << std::endl;

                             resp.send(ocl_sock);
                           }
                           else {}

                           return true;
                        };    

    while(1)
    {
      ////////////////////////////////////////////////////// 
      //recieve data
      //cout << "Entering runtime listening state:"<<endl;
      //INPUT from EXTERNAL ROUTER (CONN) 
      status = process_msg(target_sock_type::TST_RTR, 
                           recv_sock, recv_msg);
      //////////////////////////////////////////////////////                  
      //DEALER IPC SOCKET to the local nexus
      status = process_msg(target_sock_type::TST_DEALER, 
                           ocl_sock, ocl_msg);
      //////////////////////////////////////////////////////                  
      //Subscription to the publisher home_nexus
      status = process_msg(target_sock_type::TST_DEALER,
                           pub_sock, ocl_msg);
      //////////////////////////////////////////////////////
      //process queue data from 
      ///////////////////////////////////////////////////////     
      status = process_resp();
    } //end of while loop
    
    std::cout << "Shutting down pico service" << std::endl;
    col.shutdown_services();
    recv_sock.close();
    ocl_sock.close();
    pub_sock.close();

}
