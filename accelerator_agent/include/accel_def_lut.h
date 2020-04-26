#include <string>
#include <utility>
#include <vector>

#ifndef ACCELDEFLUT
#define ACCELDEFLUT

typedef struct _accel_def_lut
{
  std::string as_is;
  std::string FuncName;
  std::string AliasName;
  std::string other_ids; //includes entire hardware:ssubsys:sw:ss_sw:fun...
  std::string meta_data;
} accel_def_lut;

#endif

