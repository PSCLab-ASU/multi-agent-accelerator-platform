#include <utils.h>
#include <string>
#include <list>
#include <zmq.hpp>
#include <zmq_addon.hpp>
#include <utility>
#include <zmsg_viewer.h>
#include <algorithm>
#include <functional>
#include <iostream>

#ifndef ZMSGBUILDER
#define ZMSGBUILDER

template <typename ...Ts>
class zmsg_builder {

  public:

  using TypeList = std::tuple<Ts...>;
  //constexpr static std::size_t _N = std::tuple_size<TypeList>::value;
  constexpr static std::size_t _N = sizeof...(Ts);
  friend class zmsg_viewer<Ts...>;

  //ctor over zmq
  zmsg_builder( const zmq::multipart_t& );
  zmsg_builder( zmq::multipart_t&& );

  //converter from zmsq_viewer to zmsg_builder
  zmsg_builder( zmsg_viewer<Ts...>& );

  //init with temlpate values
  zmsg_builder( Ts... );

  //conversion methods
  zmsg_viewer<Ts...> get_viewer() const;

  //purpose of thie function is to attach
  //arbitrary data with no format
  zmsg_builder& add_custom_hdr(TypeList);
  
  //single elements added in the format of the 
  //datatype->len=1->data
  template< typename ... Targs>
  zmsg_builder& add_arbitrary_data_top( Targs... );

  template< typename ... Targs>
  zmsg_builder& add_arbitrary_data( Targs... );

  //adds a section of data
  // datatype->len=N->data
  template <typename ...Targs>
  zmsg_builder& add_sections( std::list<Targs>... );

  zmsg_builder& add_memblk(int, int, int, const void*, size_t  );

  //adds the complete clause at the end
  zmsg_builder& finalize();

  //convertible to zmq::multipart_t
  zmq::multipart_t& get_zmsg() {return _msg;};

  //add raw data
  template< typename ... Targs>
    zmsg_builder& add_raw_data_top( Targs ... );


  private:

  int _insert_completion_tag();
  
  template <std::size_t N, std::size_t MAX>
  void _recursive_add_arbitrary_data(TypeList);

  bool _finalized;
  zmq::multipart_t _msg;
};

#endif



