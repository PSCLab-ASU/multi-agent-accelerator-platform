#include <zmsg_builder.h>

template<typename ...Ts>
zmsg_builder<Ts...>::zmsg_builder( const zmq::multipart_t& msg )
: _msg(msg.clone())
{}

template<typename ...Ts>
zmsg_builder<Ts...>::zmsg_builder( zmq::multipart_t&& msg )
: _msg(std::forward<zmq::multipart_t>(msg) )
{}

template<typename ...Ts>
zmsg_builder<Ts...>::zmsg_builder( Ts... inp )
{
  std::tuple<Ts...> tL = {inp...};
  _recursive_add_arbitrary_data<_N,_N>( tL ); 
}

template<typename ...Ts>
zmsg_builder<Ts...>::zmsg_builder( zmsg_viewer<Ts...>& vwr)
{

}

template<typename ...Ts>
zmsg_viewer<Ts...> zmsg_builder<Ts...>::get_viewer() const
{
   //msg is passed by refence
   zmsg_viewer<Ts...> viewer(_msg);

   return viewer; 
}

template<typename ...Ts>
zmsg_builder<Ts...>& zmsg_builder<Ts...>::add_custom_hdr(TypeList inp)
{
  _recursive_add_arbitrary_data<_N,_N>( inp ); 
  return *this;
}

template<typename ...Ts> template<typename ... Targs>
zmsg_builder<Ts...>& zmsg_builder<Ts...>::add_raw_data_top( Targs ... args)
{
  bool dummy[sizeof...(Targs)] = {( this->_msg.pushstr(args), false)...};  
  return *this;
}

template<typename ...Ts> template <typename ...Targs>
zmsg_builder<Ts...>& zmsg_builder<Ts...>::add_arbitrary_data(Targs... args)
{
  auto add_val = [&]<typename T>(T val){
    //add type description
    this->_msg.addtyp<ushort>( (ushort)find_section_type<T>() );
    //add vector size
    this->_msg.addtyp<ulong>( 1 );
    //add each element
    if constexpr(std::is_same<T, std::string>::value)
      this->_msg.addstr(val);
    else
      this->_msg.addtyp<T>(val);
  };
  bool dummy[sizeof...(Targs)] = {( add_val(args), false)...};  
  
  return *this;
}

template<typename ...Ts> template <typename ...Targs>
zmsg_builder<Ts...>& zmsg_builder<Ts...>::add_arbitrary_data_top(Targs... args)
{
  auto add_val = [&]<typename T>(T val){
    //add each element
    if constexpr(std::is_same<T, std::string>::value)
      this->_msg.pushstr(val);
    else
      this->_msg.pushtyp<T>(val);
    //add vector size
    this->_msg.pushtyp<ulong>( 1 );
    //add type description
    this->_msg.pushtyp<ushort>( (ushort)find_section_type<T>() );
  };
  bool dummy[sizeof...(Targs)] = {( add_val(args), false)...};  
  
  return *this;
}

template<typename ...Ts> template<typename ...Targs>
zmsg_builder<Ts...>& zmsg_builder<Ts...>::add_sections(std::list<Targs>... args)
{
  auto add_val = [&]<typename T>(std::list<T> sect){
    //add type description
    this->_msg.addtyp<ushort>( (ushort) find_section_type<T>() );
    //add vector size
    this->_msg.addtyp<ulong>( sect.size() );
    //add each element
    std::for_each (sect.begin(), sect.end(),[&](T& val)
    { 
      //add each element
      if constexpr(std::is_same<T, std::string>::value)
        this->_msg.addstr(val);
      else
        this->_msg.addtyp<T>(val);
    }); //endof foreach
  }; //endof add_val
   
  bool dummy[sizeof...(Targs)] = {( add_val(args), false)...};  
  
  return *this;
}

template<typename ...Ts>
zmsg_builder<Ts...>& zmsg_builder<Ts...>::add_memblk( int b_signed, int b_int_float, 
                                                      int type_size, const void* mem, size_t sz)
{
  //add type description
  this->_msg.addtyp<ushort>( (ushort) find_section_type<zmq::message_t>() );
  //add vector size
  this->_msg.addtyp<ulong>( sz );
  //add signed information
  this->_msg.addtyp<ulong>( b_signed );
  //add type information
  this->_msg.addtyp<ulong>( b_int_float );
  //add type size information
  this->_msg.addtyp<ulong>( type_size );
  //add memory block
  this->_msg.addmem( mem, sz*type_size );

  return *this;
}

template<typename ...Ts>
zmsg_builder<Ts...>& zmsg_builder<Ts...>::finalize()
{
  _insert_completion_tag();
  _finalized = true;
  return *this;
}

template<typename ...Ts>
int zmsg_builder<Ts...>::_insert_completion_tag()
{
  _msg.addtyp<ushort>(0);
  _msg.addtyp<ulong>(1);
  _msg.addstr("END_COMMAND");
  return 0;
}

template<typename ...Ts> template <std::size_t N, std::size_t MAX> 
void zmsg_builder<Ts...>::_recursive_add_arbitrary_data(TypeList tL)
{
  if constexpr ( N != 0 )
  {
    auto val = std::get<MAX-N>(tL);
    //add each element
    if constexpr(std::is_same<decltype(val), std::string>::value)
      _msg.addstr(val);
    else
      _msg.addtyp<decltype(val)>(val);

    _recursive_add_arbitrary_data<N-1, MAX>(tL);
  }

}
//0)
template zmsg_builder<>::zmsg_builder();
//1)
template zmsg_builder<std::string, std::string>::zmsg_builder(std::string, std::string);
template zmsg_builder<std::string, std::string, std::string>::zmsg_builder(std::string, std::string, std::string);
template zmsg_builder<std::string, std::string, std::string, std::string>::
         zmsg_builder(std::string, std::string, std::string, std::string);
template zmsg_builder<std::string, std::string, std::string, std::string>::zmsg_builder(const zmq::multipart_t &);
template zmsg_builder<std::string, std::string>::zmsg_builder(const zmq::multipart_t &);
template zmsg_builder<std::string, std::string, std::string>::zmsg_builder(const zmq::multipart_t &);
template zmsg_builder<std::string, std::string, std::string, std::string>::zmsg_builder( zmq::multipart_t &&);
template zmsg_builder<std::string, std::string>::zmsg_builder(zmq::multipart_t &&);
template zmsg_builder<std::string, std::string, std::string>::zmsg_builder(zmq::multipart_t &&);
//2)
template zmsg_builder<std::string, std::string>&
zmsg_builder<std::string, std::string>::add_arbitrary_data<bool>(bool);

template zmsg_builder<std::string, std::string, std::string>&
zmsg_builder<std::string, std::string, std::string>::add_arbitrary_data<bool>(bool);

template zmsg_builder<std::string, std::string>&
zmsg_builder<std::string, std::string>::add_arbitrary_data<std::string>(std::string);

template zmsg_builder<std::string, std::string, std::string>&
zmsg_builder<std::string, std::string, std::string>::add_arbitrary_data<std::string>(std::string);

template zmsg_builder<std::string, std::string, std::string, std::string>&
zmsg_builder<std::string, std::string, std::string, std::string>::add_arbitrary_data<std::string>(std::string);

template zmsg_builder<>& zmsg_builder<>::add_arbitrary_data<std::string>(std::string);
template zmsg_builder<>& zmsg_builder<>::add_arbitrary_data<ulong>(ulong);

template zmsg_builder<std::string, std::string>&
zmsg_builder<std::string, std::string>::add_arbitrary_data<int>(int);

template zmsg_builder<std::string, std::string>&
zmsg_builder<std::string, std::string>::add_arbitrary_data<ulong>(ulong);

template zmsg_builder<std::string, std::string, std::string>&
zmsg_builder<std::string, std::string, std::string>::add_arbitrary_data<ulong>(ulong);

template zmsg_builder<std::string, std::string, std::string, std::string>&
zmsg_builder<std::string, std::string, std::string, std::string>::add_arbitrary_data<ulong>(ulong);

template zmsg_builder<std::string, std::string, std::string, std::string>&
zmsg_builder<std::string, std::string, std::string, std::string>::add_arbitrary_data<bool>(bool);
//2b)
template zmsg_builder<std::string, std::string>&
zmsg_builder<std::string, std::string>::add_arbitrary_data_top<std::string>(std::string);

template zmsg_builder<std::string, std::string, std::string>&
zmsg_builder<std::string, std::string, std::string>::add_arbitrary_data_top<std::string>(std::string);

template zmsg_builder<std::string, std::string, std::string>&
zmsg_builder<std::string, std::string, std::string>::add_arbitrary_data_top<int>(int);

//3)
template zmsg_builder<>&
zmsg_builder<>::add_sections<ulong>(std::list<ulong>);

template zmsg_builder<>&
zmsg_builder<>::add_sections<std::string>(std::list<std::string>);

template zmsg_builder<std::string, std::string>&
zmsg_builder<std::string, std::string>::add_sections<std::string>(std::list<std::string>);

template zmsg_builder<std::string, std::string, std::string>&
zmsg_builder<std::string, std::string, std::string>::add_sections<std::string>(std::list<std::string>);

template zmsg_builder<std::string, std::string, std::string, std::string>&
zmsg_builder<std::string, std::string, std::string, std::string>::add_sections<std::string>(std::list<std::string>);

template zmsg_builder<std::string, std::string, std::string>&
zmsg_builder<std::string, std::string, std::string>::add_sections<ulong>(std::list<ulong>);
//4)
template zmsg_builder<std::string, std::string>&
zmsg_builder<std::string, std::string>::finalize();

template zmsg_builder<std::string, std::string, std::string>&
zmsg_builder<std::string, std::string, std::string>::finalize();

template zmsg_builder<std::string, std::string, std::string, std::string>&
zmsg_builder<std::string, std::string, std::string, std::string>::finalize();

template zmsg_builder<>& zmsg_builder<>::finalize();
//5)
template zmsg_viewer<std::string, std::string>
zmsg_builder<std::string, std::string>::get_viewer() const;

template zmsg_viewer<std::string, std::string, std::string>
zmsg_builder<std::string, std::string, std::string>::get_viewer() const;

template zmsg_viewer<std::string, std::string, std::string, std::string>
zmsg_builder<std::string, std::string, std::string, std::string>::get_viewer() const;
//6)
template zmsg_builder<std::string, std::string>&
zmsg_builder<std::string, std::string>::add_memblk(int, int, int, const void*, size_t);
template zmsg_builder<std::string, std::string, std::string>&
zmsg_builder<std::string, std::string, std::string>::add_memblk(int, int, int, const void*, size_t);
template zmsg_builder<std::string, std::string, std::string, std::string>&
zmsg_builder<std::string, std::string, std::string, std::string>::add_memblk(int, int, int, const void*, size_t);
//7)
template
zmsg_builder<std::string, std::string>& 
zmsg_builder<std::string, std::string>::add_raw_data_top( std::string );

template
zmsg_builder<std::string, std::string>& 
zmsg_builder<std::string, std::string>::add_raw_data_top( std::string, std::string );

template
zmsg_builder<std::string, std::string, std::string>& 
zmsg_builder<std::string,std::string, std::string>::add_raw_data_top( std::string, std::string, std::string );
template
zmsg_builder<std::string, std::string, std::string>& 
zmsg_builder<std::string,std::string, std::string>::add_raw_data_top( std::string, std::string, std::string, std::string );
