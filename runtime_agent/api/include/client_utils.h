#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <list>
#include <type_traits>
#include <tuple>
#include <algorithm>
#include <mutex>
#include <thread>
#include <memory>
#include <variant>
#include <iostream>
#include <map>
#include <utility>
#include <utils.h>
#include <atomic>
#include <queue>
#include <condition_variable>
#include <functional>
#include <zmsg_builder.h>
#include <mpi_computeobj.h>
#include <mpi.h>

#ifndef CLIENT_UTILS
#define CLIENT_UTILS

#define EXPORT __attribute__((visibility("default")))
#define ONLOAD __attribute__ ((constructor))
#define ONUNLOAD __attribute__ ((destructor))

#define ACCEL_INPUT_PIVOT "--"
#define ACCEL_INPUT_PIVOT_SZ 2

void mpi_pack_deleter( void * );
typedef std::function<void(void *)> mpi_pack_deleter_t;

namespace client_utils
{
  void cobj_deleter( void * );
  typedef std::function<void(void *)> cobj_deleter_t;

  enum struct client_ctrl { def, ping, init, recv, resp, claim };

  const std::map< client_ctrl, std::string> client_rolodex =
  {
    {client_ctrl::ping,  "PING"                 },
    {client_ctrl::init,  "__INIT__"             },
    {client_ctrl::def,   "__DEFAULT__"          },
    {client_ctrl::claim, "__ACCEL_UPT_CLAIM__"  },
    {client_ctrl::recv,  "__ACCEL_RECV_DATA__"  },
    {client_ctrl::resp,  "__ACCEL_RESPONSE__"   }
  };

  client_ctrl reverse_lookup( std::string );

  zmsg_builder<std::string, std::string, std::string, std::string>
    start_routed_message( client_ctrl, std::optional<client_ctrl>, 
                          std::string, std::string );

  zmsg_builder<std::string, std::string, std::string>
    reroute_waitable_message(std::string, std::string, client_ctrl, zmq::multipart_t&& );
 

  size_t compute_cobj_size( const MPI_ComputeObj * );

  std::unique_ptr<void, cobj_deleter_t>
    serialize_cobj( const MPI_ComputeObj* );

}


typedef std::unique_ptr<void, mpi_pack_deleter_t> mpi_data_t;
typedef std::unique_ptr<void, mpi_pack_deleter_t> v_mpi_data_t;


const std::map<std::string, std::string> valid_inputs
{
  {"--accel_address",""},
  {"--accel_job_id",""},
  {"--accel_async",""},
  {"--accel_repo",""},
  {"--accel_host_file",""}
};


enum api_tags {
  mpi_init,
  mpi_test,
  mpi_finalize,
  mpi_comm_rank,
  mpi_claim,
  mpi_comm_split_type,
  mpi_type_commit,
  mpi_type_contiguous,
  mpi_graph_create,
  mpi_comm_create,
  mpi_comm_create_group,
  mpi_info_create,
  mpi_info_free,
  mpi_info_set,
  mpi_info_get,
  mpi_send,
  mpi_recv

};

//(1) signed or unsigned
//(2) INT or REAL
//(3) size of datatype
typedef std::tuple<int, int> zdata_seg_t;

const std::map<uint, zdata_seg_t> g_mpi_type_conv
{
 {MPI_FLOAT,          {1, 0} },
 {MPI_CHAR,           {0, 1} },
 {MPI_DOUBLE,         {1, 0} },
 {MPI_UNSIGNED_LONG,  {0, 1} },
 {MPI_INT,            {1, 1} }

};


struct metadata
{
  bool dst_rank_exists() const;
  //order matters for aggregate initialization
  std::optional<ulong> dst_rank; 
  bool force_mix_route=false;
};

struct mpi_return
{
  mpi_return(std::optional<int> val={}, std::optional<std::string> msg={})
  {
    ret_val   = val.value_or(0);
    error_msg = msg.value_or("Success");
  };

  operator bool()
  {
    return (ret_val == 0);
  }
  mpi_return& update()
  {
    return *this;
  };

  int ret_val;
  std::string error_msg;
};

std::tuple<std::string, std::string, std::string, std::string, bool> 
get_init_parms( int * argc, char ***  argv);

bool zmq_ping( ZPING_TYPE , std::string );

#endif

