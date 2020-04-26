#include <string>
#include <utility>
#include <variant>
#include <map>
#include <tuple>
#include <mutex>
#include <list>
#include <iostream>
#include "payloads.h"
#include <boost/range/algorithm.hpp>


#ifndef PENDMSGS_REG
#define PENDMSGS_REG

template <typename ... Ts>
class pending_msg_registry{

  using pending_message_t = typename std::variant<Ts...>;
  using registry_t        = typename std::vector< std::pair<std::string, pending_message_t> >;
  using registry_vt       = typename registry_t::value_type;

  public:

  
    template< typename V>
    struct find_element_t
    {
      ////////////////////////////////////////////////////
      //typedefs
      ///////////////////////////////////////////////////
      using Func = typename std::function<bool(const V &)>;
      ////////////////////////////////////////////////////
      //member variables
      ///////////////////////////////////////////////////
      std::optional<std::string> _key;
      Func _pred;
      ////////////////////////////////////////////////////
      //member methods
      ///////////////////////////////////////////////////
      find_element_t( std::optional<std::string> key = {}) 
      : _key(key)
      {
        _pred = []( const V& obj ){ return true; };
      }

      find_element_t( Func && pred, std::optional<std::string> key={}) 
      {
        _key = key;
        _pred = std::forward<Func>(pred);
      }

      bool operator()( const registry_vt & reg_entry)
      {
        bool out =false;
        auto&[ key, pmsg] = reg_entry;

        if(std::holds_alternative<V>(pmsg) )
        {
          //run the predicate
          out = _pred( std::get<V>(pmsg) );
          //finally compare optinal key
          if( _key ) { out = out && ( _key == key ); };
        }

        return out; 
      }
     
    };

    std::pair<std::string, std::string> get_ids( std::string AccId )
    {
      std::string LibId, ReqId;

      auto var_pair = boost::find_if( _pending_msgs,
                                      [&](auto& vp){ return (AccId == vp.first); } );
      if( var_pair == std::end(_pending_msgs ) )
      {
        std::cout << "No Ids Found" << std::endl;
        return std::make_pair("", "");
      }
      auto& var_item = var_pair->second;

      std::visit( [&](auto& arg) 
      {
        auto [LibId1, ReqId1 ] = arg.get_ids();
        LibId = LibId1;
        ReqId = ReqId1;
   
      }, var_item );
      
      return std::make_pair(LibId, ReqId);

    }

    template <typename T>
    std::optional<std::reference_wrapper<const T> > 
    get_pending_message(std::string id)
    {
      return read_pending_message<T>( find_element_t<T>(id) );
    }

    template <typename T>
    std::optional<T>&& move_pending_message_out(std::string id)
    {
      std::optional<T> out;
      return std::move( move_pending_message_out<T>( find_element_t<T>(id)) );
    }
  
    template<typename T, typename U = find_element_t<T> >
    std::optional< std::reference_wrapper<T> > 
    edit_pending_message( U&& lookup )
    {
      return _edit_pending_msg<T>( std::forward<U>(lookup)  ); 
    }

    template<typename T, typename U=find_element_t<T> >
    std::optional<std::reference_wrapper<const T> > 
    read_pending_message( U&& lookup )
    {
      return _edit_pending_msg<T>( std::forward<U>(lookup) );
    }

    template<typename T, typename U=find_element_t<T> >
    std::optional<T>
    move_pending_message_out( U&& lookup )
    {
      U lk = lookup;
      auto ref_data = _edit_pending_msg<T>( std::forward<U>(lookup));
      if( ref_data )
      {
        auto data = std::move( ref_data->get() );
        remove_pending_message<T>( std::move(lk) );
        return std::move(data);
      }
      else std::cout << "No message to move!!" << std::endl; 

      return {};

    }

    template<typename T, typename U=find_element_t<T> >
    void remove_pending_message( U&& lookup)
    {
      //remove element from registry
      boost::remove_if( _pending_msgs, std::forward<U>(lookup) );
    }

    void add_pending_message( std::string AccId, pending_message_t&& pmsg)
    {
      _pending_msgs.emplace_back(AccId, std::forward<pending_message_t>(pmsg) );
    }

    void add_generic_message( std::string AccId, std::string LibId, std::string ReqId )
    {
      other_pending_msg opm( LibId, ReqId );
      //AccId is created by this processa for downstream id
      //LibId is the Id given by the library
      //requester_id zmq given by the ZMQ subsys
      _pending_msgs.emplace_back(AccId, opm );

    }

    void remove_message( std::string id)
    {
      boost::remove_if( _pending_msgs,
                        [&](auto& entry){ return (id == entry.first); } );
    }

    template<typename T>
    std::list<T> _get_all_msgs_by_type( bool remove)
    {
      std::list<T> out;
      std::list<std::string> removal_list;
      //get all messages 
      for(auto vmsg : _pending_msgs )
      {
        if(auto pval = std::get_if<T>(&(vmsg.second)))
        {
          out.push_back( *pval );
          removal_list.push_back( vmsg.first );
        }      
      }
  
      if( remove ) 
          for(auto key : removal_list ) 
            remove_message( key );
          
  
      return out;
    }  
   

    void lock() { _pmsg_mu.lock(); }
    void unlock() { _pmsg_mu.unlock(); } 
    

  private:

    template< typename T, typename U = find_element_t<T> >
    std::optional<std::reference_wrapper<T> > 
    _edit_pending_msg (U&& lookup )
    {
      std::optional<std::reference_wrapper<T> > out;

      try
      {
        //look for a variant of type T and pred
        auto var_pair = boost::find_if( _pending_msgs, std::forward<U>(lookup) );
        //if it found an entry then return the reference
        if( var_pair != std::end(_pending_msgs ) )
        { 
          //return reference by copy
          out = std::ref( std::get<T>(var_pair->second ) );
        } else std::cout << "Could not find pmsg... _edit_pending_msg" << std::endl;
      } 
      catch (const std::bad_variant_access&) 
      {
        std::cout << "Inoorrect/no message type" << std::endl;
        out = {};
      }

      return out;    

    }

             //nex_key     message
    registry_t _pending_msgs;
    std::mutex _pmsg_mu;
};


#endif

