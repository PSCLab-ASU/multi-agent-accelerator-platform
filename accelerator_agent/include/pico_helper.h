#include <zmq.hpp>
#include <zmq_addon.hpp>
#include <algorithm>
#include <tuple>
#include <iostream>
#include <utils.h>
#include <zmsg_builder.h>

#ifndef PSHELPER
#define PSHELPER

//pico service helper functions - addr, method                  
int ps_create_msg_header ( std::string, std::string, zmq::multipart_t *);
//vector hold a list of resource
int ps_create_identify_resource_msg( zmsg_builder<std::string, std::string, std::string, std::string>&&,
                                     bool, std::vector<std::string >, zmq::multipart_t * );

#endif
