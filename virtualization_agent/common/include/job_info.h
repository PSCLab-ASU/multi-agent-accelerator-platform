#include "pico_utils.h"
#include <atomic>

#ifndef JOBTRACK
#define JOBTRACK

class job_info
{
  public: 
    job_info() = default;

  private:
    std::atomic_bool _alock;
};



#endif
