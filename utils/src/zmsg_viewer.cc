#include <zmsg_viewer.h>
#include <type_traits>

template<typename ...Ts>
void zmsg_viewer<Ts...>::print_custom_hdr()
{
  //override in the constructor
  //default is all strings
  std::cout << _msg << std::endl;
}

template<typename ...Ts>
void zmsg_viewer<Ts...>::print_full_message()
{
  //default is all strings
  std::cout << _msg << std::endl;
}

template<typename ...Ts>
std::tuple<Ts...> zmsg_viewer<Ts...>::get_header( )
{
  TypeList tL;
  zmq::multipart_t copy = _msg.clone();

  auto  pop = [&]<size_t IDX>(std::integral_constant<size_t, IDX> ) {
    if constexpr( std::is_same_v< type<IDX>, std::string> )
      return copy.popstr();
    else
      return copy.poptyp<type<IDX> >();
  };

  auto inner = [&]<std::size_t... IDX>(std::index_sequence<IDX...>)
  {
    tL = std::make_tuple( pop(std::integral_constant<size_t, IDX>() )... );
  };

  inner( std::make_index_sequence<_N>() );
  return tL;

}

template<typename ...Ts>
std::tuple<ulong, ulong, ulong, size_t, zmq::message_t>
zmsg_viewer<Ts...>::get_memblk( uint section_index)
{
  TypeList theader;
  std::size_t header_length = _N;
  uint sect_num = section_index + 1;
  std::tuple<ulong, ulong, ulong, size_t, zmq::message_t> out;
  ushort mtype;
  ulong  len;
  ulong vector_size;
  zmq::multipart_t copy = _msg.clone();

  //read header information
  auto header = [&]<std::size_t... IDX>(std::index_sequence<IDX...>)
  {
    if ( _N != 0 )
      theader = std::make_tuple( (IDX, copy.popstr())...);
  };

  auto read_section = [&](bool valid){
    mtype = copy.poptyp<ushort>();
    len   = (mtype == (ushort)zmsg_section_type::ZMSG)? 4 : copy.poptyp<ulong>();
    vector_size = (mtype == (ushort)zmsg_section_type::ZMSG)? copy.poptyp<ulong>() : len;
   
    std::list<std::add_pointer_t<decltype(copy)> > ll(len, &copy );
    if( !valid )
      std::for_each(ll.begin(), ll.end(), [&](zmq::multipart_t* msg){ msg->pop(); } );     
    else
    {
      //found section
      ulong b_sign    = copy.poptyp<ulong>();     
      ulong b_type    = copy.poptyp<ulong>();
      ulong type_size = copy.poptyp<ulong>();
      auto  data      = std::move(copy.pop());      
      //set the output

      out = std::make_tuple(b_sign, b_type, type_size, vector_size, std::move(data) );

    }    
  };//end of lambda: read_section

  //pop the header aways from teh copy;  
  header( std::make_index_sequence<_N>() );
  //pop all of them until valid
  while(sect_num){
     read_section( (sect_num == 1) );
     sect_num--;
  }
  //return last section read
  return out;
  
}

template<typename ...Ts> template<typename T>
std::list<T> zmsg_viewer<Ts...>::get_section( uint section_index )
{
  TypeList theader;
  std::size_t header_length = _N;
  uint sect_num = section_index + 1;
  std::list<T> list;
  ushort mtype;
  ulong  len;
 
  zmq::multipart_t copy = _msg.clone();

  //read header information
  auto header = [&]<std::size_t... IDX>(std::index_sequence<IDX...>)
  {
    if ( _N != 0 )
      theader = std::make_tuple( (IDX, copy.popstr())...);
  };

  //pop empty values
  auto throw_away  = [&](zmq::multipart_t* msg)
  {
    msg->pop();
  };
  //insert element in to list
  auto insert_list = [&](zmq::multipart_t* msg)
  {
      if(mtype == ( (ushort) find_section_type<T>()) )
      {
        if constexpr (std::is_same<T, std::string>::value)
          list.push_back( msg->popstr() );
        else if constexpr (std::is_same<T, std::add_pointer_t<void> >::value)
          list.push_back( msg->pop().data() );
        else if constexpr (std::is_same<T, zmq::message_t >::value)
          list.push_back( std::move(msg->pop()) );
        else
          list.push_back( msg->poptyp<T>() );
   
      }
      else
      {
        std::cout << "Invalid type converion " << std::endl;
      }
  };

  auto read_section = [&](bool valid){
    mtype = copy.poptyp<ushort>();
    len   = copy.poptyp<ulong>();

    std::list<std::add_pointer_t<decltype(copy)> > ll(len, &copy );
    if( !valid )
      std::for_each(ll.begin(), ll.end(), throw_away);     
    else
      std::for_each(ll.begin(), ll.end(), insert_list);     
     
  };//end of lambda: read_section

  //pop the header aways from teh copy;  
  header( std::make_index_sequence<_N>() );
  //pop all of them until valid
  while(sect_num){
     read_section( (sect_num == 1) );
     sect_num--;
  }
  //return last section read
  return list;
}

template<typename ...Ts>
bool zmsg_viewer<Ts...>::verify_message()
{
  return true;
}

// constructor
template std::tuple<std::string, std::string, std::string, std::string>
zmsg_viewer<std::string, std::string, std::string, std::string>::get_header();
template std::tuple< std::string, std::string, std::string>
zmsg_viewer<std::string, std::string, std::string>::get_header();
//get_memblk
template std::tuple<ulong,ulong,ulong, size_t, zmq::message_t>
zmsg_viewer<>::get_memblk( uint );
template std::tuple<ulong,ulong,ulong, size_t, zmq::message_t>
zmsg_viewer<std::string, std::string>::get_memblk( uint );
template std::tuple<ulong,ulong,ulong, size_t, zmq::message_t>
zmsg_viewer<std::string, std::string, std::string>::get_memblk( uint );

//get_section
template std::list<bool>
zmsg_viewer<std::string, std::string, std::string, std::string>::get_section<bool>(unsigned int);

template std::list<bool> zmsg_viewer<>::get_section<bool>(unsigned int);
template std::list<ulong> zmsg_viewer<>::get_section<ulong>(unsigned int);
template std::list<std::add_pointer_t<void> > 
zmsg_viewer<>::get_section<std::add_pointer_t<void> >(unsigned int);
template std::list<zmq::message_t > zmsg_viewer<>::get_section<zmq::message_t >(unsigned int);
template std::list<std::string> zmsg_viewer<>::get_section<std::string>(unsigned int);
template std::list<std::string> 
zmsg_viewer<std::string, std::string, std::string>::get_section<std::string>(unsigned int);
template std::list<bool> 
zmsg_viewer<std::string, std::string, std::string>::get_section<bool>(unsigned int);
template std::list<std::string> 
zmsg_viewer<std::string, std::string, std::string, std::string>::get_section<std::string>(unsigned int);
template std::list<std::string> zmsg_viewer<std::string, std::string>::get_section<std::string>(unsigned int);
template std::list<ulong> zmsg_viewer<std::string, std::string, std::string>::get_section<ulong>(unsigned int);
