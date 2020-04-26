#include <concepts>
#include <stop_token>
#include <cstdint>
#include <thread_group.h>
#include <concurrent_bounded_queue.h>
#include <boost/range/algorithm_ext/iota.hpp>

#ifndef THREADMGR
#define THREADMGR

template<class T, class U, class V, class W>
concept FuncConstraint = std::regular_invocable<T, U, V> &&
                         std::is_same_v<typename T::result_type, std::vector<W> >; 


template <std::copyable Hdr, std::movable InMsg, std::movable OutMsg, FuncConstraint<Hdr, InMsg, OutMsg> Func,  
          std::uint64_t QueueDepth>
struct thread_manager
{
  public:

    thread_manager( std::uint64_t n )
      : threads(n, [&](std::stop_token s) { process_entry(s); } ) {}

    ~thread_manager(){ threads.request_stop(); }

    virtual Hdr get_header( InMsg& item) = 0; 
    virtual Func action_ctrl( decltype(Hdr::method) ) = 0;
    
    int submit( InMsg&& msg_in){
      this->ingress_q.enqueue( std::move(msg_in) );
      return 0;
    }

    OutMsg read(){
      return std::move( this->egress_q.dequeue() );
    }
  
    std::optional<OutMsg> try_read(){
      return std::move( this->egress_q.try_dequeue() );
    }  

    std::vector<std::optional<OutMsg> > try_read_all(){
      decltype(try_read_all() ) out;
      std::optional<OutMsg> msg = 
                         std::move(this->egress_q.try_dequeue() );
      while( msg ) 
      {
        out.push_back( std::move( msg ) );
        msg = std::move( this->egress_q.try_dequeue() );
      }
      return out;
    } 

    void process_single()
    {
      auto&& msg_in  = this->ingress_q.try_dequeue();
      if( msg_in )
      {
        Hdr hdr = get_header( msg_in.value() ); 
        std::vector<OutMsg>&& msgs_out = 
          action_ctrl( hdr.method )(hdr, std::move(msg_in.value() ) );

          while( !msgs_out.empty() ) 
          {
            this->egress_q.enqueue( std::move( msgs_out.back() ) );
            msgs_out.pop_back();
          } 
            
      }

    }

    bool ingress_q_empty() { return this->ingress_q.empty(); }
    bool egress_q_empty()  { return this->egress_q.empty();  }
 
  private:
    
    void process_entry(std::stop_token s)
    {
      while(!s.stop_requested() )
      {
        process_single();
      }
      
    }
 
    concurrent_bounded_queue<InMsg,  QueueDepth> ingress_q;
    concurrent_bounded_queue<OutMsg, QueueDepth> egress_q;
    thread_group threads;

};

#endif
