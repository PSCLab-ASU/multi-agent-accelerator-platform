#include "base_utils.h"
#include "pico_utils.h"

#ifndef PICOCFG
#define PICOCFG

class pico_cfg
{
  public:
    pico_cfg();

    const std::string get_external_address( );
    const std::string get_owner( );
    const std::string get_repos( );
    const std::string get_filters( );

    pico_return set_external_address( std::string);
    pico_return set_owner(std::string);
    pico_return set_repos( std::string, bool);
    pico_return set_filters( std::string );

  private:
    std::mutex mu;
    std::string _external_address;
    std::string _service_owner;
    std::vector<std::string> _repos;
    std::vector<std::string> _filters;
};

#endif

