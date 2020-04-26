#include <boost/thread/shared_mutex.hpp>
#include <tuple>
#include <string>
#include <vector>
#include <map>
#include <iostream>

#ifndef ADSSTATE
#define ADSSTATE

class ads_state
{ 
  //active, accelerator AccId, node_index
  using claim_header = std::tuple<bool, std::string, ulong>;
  enum LOCK_STATE { NO_STATE, SH_STATE, EX_STATE };

  public:

    ads_state();

    //initing with one claim
    ads_state(std::string, std::string);
  
    void activate()   { _active = true;  }
    void deactivate() { _active = false; }
    void finalized() { _final = true; }

    std::string get_accel_id( std::string );
    ulong get_nex_id( std::string );

    const bool& isActive() const { return _active; };
    const bool& isFinal() const { return _final; };
                    /*driver claim*/
    void add_claim( std::string, std::string );
                /*driver claim*//*nexus claim*/
    void update_claim( std::string,
                    bool act=true, ulong nid=0 );

    void activate_claim( std::string );

    void deactivate_claim(std::string );

    //defined if a claim is active
    bool isCActive( std::string );
    //defined if a claim is pending HW
    bool isCPending( std::string );
    //defined if numbers are claimed deactivated
    bool isCClaimed( std::string );

    ulong get_node_index( std::string );

    std::string get_accel_claim( std::string );
   
    void lock_shared() {  
      _mu.lock_shared();
    }

    void unlock_shared(){
        _mu.unlock_shared();
    }

    void lock() { 
      _mu.lock();
    }

    void unlock(){
      _mu.unlock();
    }
    
        
  private:

    LOCK_STATE lt;
    bool _active;
    bool _final;

    std::optional< std::reference_wrapper<claim_header> >
      _get_claim_header( std::string );

    boost::upgrade_mutex _mu;
    //       claimId_from_driver claimId_from_nexus
    std::map<std::string, claim_header > _claim_covr_lookup; 

};


#endif

