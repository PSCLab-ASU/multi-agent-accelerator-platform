#include <vector>
#include <thread>
#include <ranges>
#include <iostream>
#include <stop_token>
#include <boost/range/algorithm/for_each.hpp>

#ifndef THREADGH
#define THREADGH

struct thread_group
{
  public:
    thread_group(std::uint64_t n, std::invocable<std::stop_token> auto&& f){
      for(auto i : std::ranges::views::iota(0,n)) members.emplace_back(f);
    }
    
    auto size() { return members.size(); }

    void request_stop(){
      std::cout << "Requesting to stop thread group" << std::endl;
      boost::range::for_each(members, [](std::jthread& t){ t.request_stop(); } );
    }

  private:
    std::vector<std::jthread> members;
 
};


#endif


