#include <mpi_ext.h>
#include <client_utils.h>
#include <mpi_proc_impl.h>
#include <mpi_accel_impl.h>
#include <mpi_mix_impl.h>

std::unique_ptr<mpi_mix_impl> g_mpi_interface;

template<typename ... Ts>
mpi_return _general_router(Ts&... Targs)
{
  mpi_return mret;
  mret = g_mpi_interface->router(Targs...);
 
  return mret;
}

void ONLOAD initLibrary(void) 
{
  // Function that is called when the library is loaded
  printf("Library is initialized\n"); 
}
void ONUNLOAD cleanUpLibrary(void) 
{
  // Function that is called when the library is »closed«.
  printf("Library is exited\n"); 
}

//Complete
int EXPORT MPIX_Init(int *argc, char *** argv)
{
  //forward init for further action
  std::integral_constant<api_tags, mpi_init> tag;
 
  //get address to accelerator service and jobId (either passed in or random)
  auto[asa, jobId, host_file, repo, async] = get_init_parms( argc, argv);
  std::cout << "asa : " << asa << std::endl;
  std::cout << "job_id : " << jobId << std::endl;
  std::cout << "host file : " << host_file << std::endl;
  std::cout << "repo : " << repo << std::endl;
  std::cout << "async : " << async << std::endl;
  //used to coordinate mpi accelerator services
  //TBD work on the constructor parameter
  g_mpi_interface = std::make_unique< mpi_mix_impl>(asa, jobId, host_file, repo, async);
  metadata meta{0, true};
  //pass through init params to runtimes
  mpi_return mret = _general_router( meta, tag, argc, argv );
  return 0;
}

//Complete
int EXPORT MPIX_Test()
{
  std::integral_constant<api_tags, mpi_test> tag;
  metadata meta;

  _general_router(meta, tag );

  return 0;
}

//Complete
int EXPORT MPIX_Finalize()
{
  std::integral_constant<api_tags, mpi_finalize> tag;
  metadata meta{0, true};

  _general_router(meta, tag );

  return 0;
  
}

//Make a claim to mpirun
//mpirun makes a claim to resource manager
int EXPORT MPIX_Claim(const char * falias,  MPIX_Backup_Function bfunc, 
                      MPI_Info info,  ulong *rank)
{
  std::integral_constant<api_tags, mpi_claim> tag;
  metadata meta{0,true};

  _general_router(meta, tag, falias, bfunc, info, rank );

  return 0;
}

int EXPORT MPIX_Send(const void * buf, int cnt, MPI_Datatype dt, int dest, int mpi_tag, MPI_Comm comm)
{
  std::integral_constant<api_tags, mpi_send> tag;
  metadata meta{dest, false};

  _general_router(meta, tag, buf, cnt, dt, dest, mpi_tag, comm );

  return 0;
} 

int EXPORT MPIX_Recv(void * buf, int cnt, MPI_Datatype dt, int source, int mpi_tag, MPI_Comm comm, MPI_Status * status)
{
  std::integral_constant<api_tags, mpi_recv> tag;
  metadata meta{source, false};
  
  _general_router(meta, tag, buf, cnt, dt, source, mpi_tag, comm, status );

  return 0;
}

//Complete
int EXPORT MPIX_Comm_rank(MPI_Comm comm, int *rank)
{
  return 0;
}

//COMPLETE;
int EXPORT MPIX_Info_create(MPI_Info * info)
{
  return 0;
}
//COMPLETE
int EXPORT MPIX_Info_free(MPI_Info * info)
{
  return 0;
}

//COMPLETE;
int EXPORT MPIX_Info_set(MPI_Info info, const char * key, const char * value)
{
  return 0;
}

//COMPLETE
int EXPORT MPIX_Info_get(MPI_Info info, const char * key, int valuelen, char* value, int * flag)
{
  return 0;
}

int EXPORT MPIX_Comm_split_type(MPI_Comm comm, int split_type, int key,
    MPI_Info info, MPI_Comm *newcomm)
{ return 0; }

int EXPORT MPIX_Type_commit(MPI_Datatype *datatype)
{ return 0; }

int EXPORT MPIX_Type_contiguous(int count, MPI_Datatype oldtype,
    MPI_Datatype *newtype)
{ return 0; }

int EXPORT MPIX_Graph_create(MPI_Comm comm_old, int nnodes, const int index[],
    const int edges[], int reorder, MPI_Comm *comm_graph)
{ return 0; }

int EXPORT MPIX_Comm_create(MPI_Comm comm, MPI_Group group, MPI_Comm *newcomm)
{ return 0; }

int EXPORT MPIX_Comm_create_group(MPI_Comm comm, MPI_Group group, int tag, MPI_Comm
*newcomm)
{ return 0; }

int EXPORT MPIX_FreeCompObj( MPI_ComputeObj * cobj)
{
  return 0;
}
