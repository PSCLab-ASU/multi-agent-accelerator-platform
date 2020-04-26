#include <map>
#include <mutex>
#include <optional>
#include <iostream>
#include <ads_state.h>

#ifndef ADSREGISTRY
#define ADSREGISTRY

class ads_registry
{
  public:

    ads_registry();

    std::optional<std::reference_wrapper<ads_state> > 
      find_adss( std::string );
    ads_state& find_create_adss( std::string, std::string, std::string, 
                                 bool&, bool create=false);
    const bool& is_all_ads_complete();
    const bool& is_all_ads_finalized();
    void erase( std::string rid) { _ads_registry.erase( rid ); }
    void lock() { _mu.lock(); }
    void unlock() { _mu.unlock(); }
    ulong size() { return _ads_registry.size(); }

  private:
     
    bool _stop;
    std::mutex _mu;
              //requester_id  state
    std::map< std::string, ads_state > _ads_registry;
};


#endif
