#include <chrono>
#include <utils.h>
#include <queue>
#include <ticket_mutex.h>
#include <counting_semaphore.h>
#include <mutex>

#ifndef CCBOUNDQ
#define CCBOUNDQ

template <typename T, std::uint64_t QueueDepth>
struct concurrent_bounded_queue
{
  public:
    constexpr concurrent_bounded_queue() = default;

    void enqueue( std::convertible_to<T> auto&& i)
    {
      //get a mutex
      std::scoped_lock l(items_mtx);
      //decrement the remamining space by one
      remaining_space.acquire();
      {
        //add the data into the queue
        items.emplace(std::forward<decltype(i)>(i));
      }
      items_produced.release();
    }

    T dequeue()
    {
      std::scoped_lock l( items_mtx );
      //decrement the item produces count by 1
      items_produced.acquire();
      //pop the data out of the Q
      T tmp = pop();
      //Add a spot to the remaining space
      remaining_space.release();
      return std::move(tmp);
    }

    std::optional<T> try_dequeue()
    {
      std::scoped_lock l( items_mtx );
      if(items_produced.try_acquire() )
      {
        T tmp = pop();
        remaining_space.release();
        return std::move(tmp);

      } else return {};
    }

    bool empty() { return items.empty(); }

  private:

    T pop()
    {
      std::optional<T> tmp;
      //std::scoped_lock l( items_mtx );
      std::cout << "Number of elements in items : " << items.size() << std::endl;
      tmp = std::move( items.front() );
      items.pop();
      std::cout << "completed pop : " << items.size() << std::endl;
      return std::move(*tmp);
    }

    std::queue<T> items;
    std::mutex items_mtx; //NEED to upgrade this to a ticketed mutex
    std::counting_semaphore<QueueDepth> items_produced{0};
    std::counting_semaphore<QueueDepth> remaining_space{QueueDepth};

};

#endif
