#include <string>
#include <utility>
#include <variant>
#include <map>
#include <tuple>
#include <mutex>
#include <list>
#include <iostream>
#include <boost/range/algorithm.hpp>

#ifndef PENDMSGS_
#define PENDMSGS_

enum arg_sign_t{ ARG_UNSIGNED=0, ARG_SIGNED=1 };
enum arg_data_t{ ARG_REAL=0, ARG_INT=1 };
using arg_header_t  = std::tuple<arg_sign_t, arg_data_t, uint, size_t >;
using arg_element_t = std::tuple<arg_header_t, const void *>;

template<typename T> 
struct arg_definition
{
  using base_type_t     = T;

  arg_definition( T&& bdata) : _base_data( std::forward<T>(bdata) ) {};

  const T& get_const_bdata() { return _base_data; }
  T& get_bdata() { return _base_data; }

  void set_nargs( ushort nargs ) { _arg_index = _num_args = nargs; }

  ushort get_nargs( ) { return _num_args; }

  ushort get_size() const { return _data.size(); }

  void push_arg( arg_element_t&& args) { _data.push_back( args ); }

  arg_element_t pop_arg() const { return _data.at(--_arg_index); }

  bool empty() { return (_arg_index == 0); }

  ushort _num_args;
  mutable uint _arg_index; 

  std::vector< arg_element_t > _data;

  T _base_data; 

};

struct _base_pending_msg
{
  std::string _key;
  std::string _requesting_id;

  _base_pending_msg( std::string libId, std::string rid) {
    _key = libId; 
    _requesting_id = rid;
  }

  std::pair< std::string, std::string> get_ids() const
  {
    return std::make_pair( _key, _requesting_id );
  }

  std::string get_rid( ) const { return _requesting_id; }
  std::string get_kid( ) const { return _key; }

  void set_rid( std::string rid ) { _requesting_id = rid; }
  void set_kid( std::string kid ) { _key = kid; }

};

struct other_pending_msg : _base_pending_msg
{
  other_pending_msg( std::string libId, std::string rid) 
  : _base_pending_msg( libId, rid){};


};

#endif
