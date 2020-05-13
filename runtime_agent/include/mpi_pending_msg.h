#include <string>
#include <utility>
#include <variant>
#include <map>
#include <tuple>
#include <mutex> 
#include <list> 
#include <zmq.hpp>
#include <zmq_addon.hpp>
#include <pending_msg_reg.h>

#ifndef MPIEXT_PENDMSGS
#define MPIEXT_PENDMSGS

template<bool>
struct mpi_sendrecv_pmsg : _base_pending_msg
{
  //copy
  mpi_sendrecv_pmsg( const mpi_sendrecv_pmsg& rhs)
  : _base_pending_msg(rhs.get_rid(), rhs.get_kid() )
  { 
    try{
      std::cout << " mpi_sendrecv_pmsg copy ctor" << std::endl;
      _bactive = rhs.is_active();
      _key     = rhs.get_key();
      _rank    = rhs.get_rank();  
      _tag     = rhs.get_tag();
      _ozmsg   = rhs.copy_data();
    }
    catch(...) { std::cout << "thrown exception from copy mpi_sendrecv_pmsg" << std::endl; }
  }

  //move
  mpi_sendrecv_pmsg( mpi_sendrecv_pmsg&& rhs)
  : _base_pending_msg(rhs.get_rid(), rhs.get_kid() )
  {
    try{
      _bactive = rhs.is_active();
      _key     = rhs.get_key();
      _rank    = rhs.get_rank();  
      _tag     = rhs.get_tag();
      _ozmsg   = rhs.move_data();
    }
    catch(...)
    {
      std::cout<< "thrown exception from mpi_sendrecv_pmsg" << std::endl;
    }
  }
  //assignment
  mpi_sendrecv_pmsg& operator=( const mpi_sendrecv_pmsg& rhs)
  { 
    try{
      std::cout << " mpi_sendrecv_pmsg assignment operator" << std::endl;
      auto[ kid, rid ] = rhs.get_ids();
      set_rid( rid );
      set_kid( kid );
      _bactive = rhs.is_active();
      _key     = rhs.get_key();
      _rank    = rhs.get_rank();  
      _tag     = rhs.get_tag();
      _ozmsg   = rhs.copy_data();
    }
    catch(...) { std::cout << "thrown exception from assignment mpi_sendrecv_pmsg" << std::endl; }
    return *this;
  }

  mpi_sendrecv_pmsg( std::string key, int rank, int tag )
  : _base_pending_msg( "", "" ) 
  {
    _key       = key;
    _rank      = rank;
    _tag       = tag;
    _bactive   = false;
  }

  mpi_sendrecv_pmsg( std::string key, int rank, int tag, zmq::multipart_t&& msg )
  : _base_pending_msg( "", "" ) 
  {
    _bactive   = false;
    _key       = key;
    _rank      = rank;
    _tag       = tag;
    _ozmsg     = std::forward<zmq::multipart_t>(msg);
  }

  std::tuple<int, int> get_rank_tag( ) const { return {_rank, _tag }; }
  int          get_tag ( )  const { return _tag;  }
  int          get_rank( )  const { return _rank; }
  std::string  get_key ( )  const { return _key;  }
 
  std::optional<zmq::multipart_t>
  copy_data( ) const 
  { 
    std::cout << "is _ozmsg.empty() = " << _ozmsg->empty() << std::endl;
    std::cout << " _ozmsg.size () = " << _ozmsg->size() << std::endl;
    std::cout << "is _ozmsg payload = " << (bool)_ozmsg << std::endl;
    std::cout << " copying data... in mpi_sendrecv_pmsg " << std::endl;
    if( _ozmsg ) return _ozmsg->clone(); 
    else return {};
  }  

  std::optional<zmq::multipart_t>&&
  move_data( ) { return std::move( _ozmsg ); }  

  void move_data ( zmq::multipart_t && zmsg )
  {
    _ozmsg = std::forward<zmq::multipart_t>(zmsg);
  }
  
  std::optional<zmq::multipart_t> & 
  get_data ( ) { return _ozmsg; } 

  void deactivate() { _bactive = false; }
  void activate()   { _bactive = true;  }
  bool is_active() const  { return _bactive;  }
 
  std::string _key;
  int   _rank;
  int   _tag;
  bool _bactive; 
  std::optional<zmq::multipart_t> _ozmsg;

};


typedef mpi_sendrecv_pmsg<true>  mpi_send_pmsg;
typedef mpi_sendrecv_pmsg<false> mpi_recv_pmsg;

#define MPIPM mpi_send_pmsg, mpi_recv_pmsg, other_pending_msg

class mpi_pending_msg_registry 
: public pending_msg_registry< MPIPM >
{
  public:

    template <typename T>
    using find_pmsg_t = typename pending_msg_registry< MPIPM >::find_element_t<T>;  
   
    mpi_pending_msg_registry(){}

    template <typename T>
    static find_pmsg_t<T>::Func
    get_rank_tag_pred( const std::pair<int,int>& rank_tag, std::optional<std::string> key={} )
    {
      
      return  [&]( const T& pmsg )->bool 
              { 
                 printf("--client:pred src=%i:%i, tag=%i:%i\n", 
                        rank_tag.first, pmsg.get_rank(), 
                        rank_tag.second, pmsg.get_tag() );

                 return ( (rank_tag.first  == pmsg.get_rank() ) &&
                          (rank_tag.second == pmsg.get_tag()  ) );
              };
    }
    
    void add_send_pmsg( std::string key, int rank, int tag, zmq::multipart_t&& zmsg )
    {
      //step 1: create a send message 
      std::cout << "Creating mpi_send_pmsg..." << std::endl;
      auto spmsg = mpi_send_pmsg( key, rank, tag, std::forward<zmq::multipart_t>(zmsg) );
      //step 2) create a empty recv message
      std::cout << "Creating mpi_recv_pmsg..." << std::endl;
      auto rpmsg = mpi_recv_pmsg( key, rank, tag);
      //step 3) add send then recv
      std::cout << "Adding send message..." << std::endl;
      add_pending_message( key, std::move(spmsg) );     
      std::cout << "Adding recv message..." << std::endl;
      add_pending_message( key, std::move(rpmsg) );
      std::cout << "Completed add_send_pmsg..." << std::endl;
    }
 
    auto read_recv_pmsg( std::optional<std::pair<int, int> > rank_tag ={},
                         std::optional<std::string> key ={} )
    { 
      auto find_pmsg = _generate_pmsg_finder<mpi_recv_pmsg>(key, rank_tag );
      return read_pending_message<mpi_recv_pmsg>( std::move(find_pmsg) );  
    }
 
    
    std::optional<std::reference_wrapper<mpi_recv_pmsg> > 
    edit_recv_pmsg( std::optional<std::pair<int, int> > rank_tag ={},
                    std::optional<std::string> key ={} )
    {
      auto find_pmsg = _generate_pmsg_finder<mpi_recv_pmsg>(key, rank_tag );
      return edit_pending_message<mpi_recv_pmsg>( std::move(find_pmsg) );  
    }
 
    std::optional< mpi_send_pmsg > move_send_pmsg( std::string key )
    { 
      std::optional<std::pair<int, int> > empty_rank_tag;
      auto find_pmsg = _generate_pmsg_finder<mpi_send_pmsg>(key, empty_rank_tag);
      return move_pending_message_out<mpi_send_pmsg>( std::move(find_pmsg) );
    }

    std::optional< mpi_recv_pmsg >&& move_recv_pmsg( std::optional<
                                                       std::pair<int, int> > rank_tag ={}, 
                                                     std::optional<std::string> key={} )
    {
      auto find_pmsg = _generate_pmsg_finder<mpi_recv_pmsg>(key, rank_tag );
      return move_pending_message_out<mpi_recv_pmsg>( std::move(find_pmsg) );  
    }
 
  private:

    template< typename T>
    find_pmsg_t<T> _generate_pmsg_finder( const std::optional<std::string>& key ,
                                          const std::optional<
                                           std::pair<int, int> > & rank_tag  )
    {
     
      if( key && rank_tag )
      {
        //get a predicate
        auto pred = get_rank_tag_pred<T>( rank_tag.value() );
        //create a find_pmsg
        return find_pmsg_t<T>(std::move(pred), key );
      }
      else if( rank_tag )
      {
        //get a predicate
        auto pred = get_rank_tag_pred<T>( rank_tag.value() );
        //create a find_pmsg
        return find_pmsg_t<T>(std::move(pred) );
      }
      else return find_pmsg_t<T>(key);
             
    }                        

};
 


#endif

