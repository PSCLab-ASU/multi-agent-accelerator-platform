#include <sys/types.h>
#include <unistd.h>
#include <runtime_ctx.h>
#include <mpi_computeobj.h>
#include <mpi.h>

extern "C"
{
  //use C function overload in the future TBD
  typedef MPI_ComputeObj::callback MPIX_Backup_Function;

  //wip
  int MPIX_Init(int *argc, char ***argv);
  int MPIX_Finalize();
  int MPIX_Test();
  int MPIX_Send(const void *, int, MPI_Datatype, int, int, MPI_Comm );
  int MPIX_Recv(void *,       int, MPI_Datatype, int, int, MPI_Comm, MPI_Status* );
  int MPIX_Claim (const char *, MPIX_Backup_Function, MPI_Info, ulong *);
  
  //backlisted
  int MPIX_Comm_rank(MPI_Comm, int *);

  int MPIX_Comm_split_type(MPI_Comm comm, int split_type, int key,
                          MPI_Info info, MPI_Comm *newcomm);

  int MPIX_Type_commit(MPI_Datatype *datatype);

  int MPIX_Type_contiguous(int count, MPI_Datatype oldtype,
                          MPI_Datatype *newtype);

  int MPIX_Graph_create(MPI_Comm comm_old, int nnodes, const int index[],
                       const int edges[], int reorder, MPI_Comm *comm_graph);

  int MPIX_Comm_create(MPI_Comm comm, MPI_Group group, MPI_Comm *newcomm);

  int MPIX_Comm_create_group(MPI_Comm comm, MPI_Group group, int tag, MPI_Comm
                            *newcomm);

  int MPIX_Info_create(MPI_Info *info);
  int MPIX_Info_free(MPI_Info *info);

  int MPIX_Info_set(MPI_Info info, const char *key, const char *value);
  int MPIX_Info_get(MPI_Info info, const char *key, int valuelen, char *value,
                   int *flag);
  int MPIX_FreeCompObj( MPI_ComputeObj *);
}
