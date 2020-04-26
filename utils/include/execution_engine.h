#include <atomic>
#include <queue>
#include <functional>
#include "thread_manager.h"

#ifndef EXECENG
#define EXECENG

//HEAVY NOTE: If you want to work with a threadpool make sure your
//DerivedObj is threadsafe!!!

template<typename T>
concept Exists = requires (T a) {
    a.get_header(); // "InputType Does not have get_header"
};

template<typename T>
concept HasExecDecl = requires 
        { 
          typename T::ExecInputType; 
          typename T::ExecOutputType; 
          typename T::ExecCommandType; 
        };

template<HasExecDecl DerivedObj, std::uint64_t QD> 
class execution_engine : 
  public thread_manager<typename DerivedObj::ExecInputType::second_type::type::header, 
                        typename DerivedObj::ExecInputType, 
                        typename DerivedObj::ExecOutputType, typename DerivedObj::ExecCommandType, QD> 
{
  
  public:

    using CommandType      = DerivedObj::ExecCommandType;
    using InputType        = DerivedObj::ExecInputType;
    using OutputType       = DerivedObj::ExecOutputType;
    using HeaderType       = InputType::second_type::type::header;
    using LookupType       = HeaderType::LookupType;
    using ThreadManager    = thread_manager<HeaderType, InputType, OutputType, CommandType, QD>;

    using output_func_type = std::vector<OutputType>;

    execution_engine( ulong num_threads=0 ) 
    : ThreadManager(num_threads)
    {
      //this->_check_input_type_header<InputType>( );
    }

    template<typename DObj>
    void add_execution_method(LookupType key, DObj * pobj, auto func)
    {
      auto method = std::bind(func, pobj, 
                              std::placeholders::_1, std::placeholders::_2 );

      if( pobj != NULL ) _function_map.emplace( key, method );

      else std::cout << "error adding function" << std::endl;
    } 

    HeaderType get_header( InputType& in )
    {
      return in.second.get().get_header();
    }

    CommandType action_ctrl( LookupType key)
    {
      for( auto entry : _function_map )
        if( entry.first == key ) return entry.second;

      return _function_map.at( LookupType() ); 
    }

    bool any_execution_remaining()
    {
      return this->ingress_q_empty() && this->egress_q_empty() && !_exec_in_progress;
    }
  
    void exec_in_progress()     { _exec_in_progress = true; }  
    void exec_not_in_progress() { _exec_in_progress = false; }  

  private:

    template<Exists T> constexpr
    void _check_input_type_header(){}

    std::atomic_bool _exec_in_progress;
    std::map< LookupType, CommandType> _function_map;
   
};

#endif

