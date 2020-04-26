#include <optional>
#include <string>
#include <map>

struct ncache_header
{
  std::string id; //unique id per nexus
  std::string vid;
  std::string pid;
  std::string ss_vid;
  std::string ss_pid;
  std::string sw_vid;
  std::string sw_pid;
  std::string sw_fid;
  std::string sw_verid;
  std::string reprog;
  std::optional<std::string> owner;
  std::optional<std::string> location;
  
};

class ncache_registry
{
  public:
    ncache_registry(){}


  private:

    std::map<std::string, ncache_header> nxcaches;
  
};



