#include <vector>
#include <map>
#include <tuple>
#include <iostream>
#include <optional>
#include <type_traits>


#ifndef BATCHENG
#define BATCHENG

enum batch_status { SUCCESS, OUT_OF_BOUND, COMPLETE, INCOMPLETE };  

template<typename T> concept Batch_Item =
requires {
    ( T::sequence_number ) && 
    ( std::is_integral_v<decltype(T::sequence_number)>);
    ( T::total_number    ) &&
    ( std::is_integral_v<decltype(T::total_number)>);
};

template <typename Key, Batch_Item Item >
  requires std::equality_comparable<Key> //remember to overload equals bool operator==(const type&) const
class batch_engine
{
  public: 
    batch_engine() {};
    
    batch_status submit_item( const Key& , Item&& ) { return batch_status::SUCCESS; }
    std::vector<Item> get_folder( Key, bool remove_complete = false ) {}
    bool is_folder_complete( Key ) { return true; } 
    //move an entire folger to get serviced
    std::vector<Item> pop_complete( std::optional<Key> & ) { return {}; }

  private:
       
    //remove entire folder from cabinet
    void _remove_folder( const Key& ) {}

    std::multimap<Key, Item> _message_cabinet;
};

#endif
