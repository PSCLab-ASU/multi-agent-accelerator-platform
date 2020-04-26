#include <utils.h>
#include <string>
#include <list>
#include <zmq.hpp>
#include <zmq_addon.hpp>
#include <utility>
#include <functional>
#include <algorithm>
#include <iostream>
#include <type_traits>

#ifndef ZMSGVIEWER
#define ZMSGVIEWER

template <typename ...Ts>
class zmsg_viewer {
  
  public:

  using TypeList = std::tuple<Ts...>;
  
  template <std::size_t N>
  using type = typename std::tuple_element<N, TypeList>::type;
 
  constexpr static std::size_t _N = std::tuple_size<TypeList>::value;

  zmsg_viewer( const zmq::multipart_t& zmsg) : _msg(zmsg) {}

  //prints custom header ONLY
  void print_custom_hdr();

  void print_full_message();

  TypeList get_header( );

  template<typename T>
  std::list<T> get_section( uint section_index );
 
  std::tuple<ulong, ulong, ulong,
             size_t, zmq::message_t > get_memblk( uint );
 
  const zmq::multipart_t& get_data() { return _msg; }
  zmq::multipart_t get_zmsg(){ return std::move( _msg.clone() ); }
  zmq::multipart_t& edit_data_edt() { return const_cast<zmq::multipart_t&>(_msg); }
  
  bool verify_message();

  private:

  const zmq::multipart_t&  _msg;

};



#endif
