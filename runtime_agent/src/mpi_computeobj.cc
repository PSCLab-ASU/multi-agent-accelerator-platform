#include <mpi_computeobj.h>

void   ** MPI_ComputeObj::get_data()    const    { return data;     }
uint   *  MPI_ComputeObj::get_types()   const    { return types;    }
size_t *  MPI_ComputeObj::get_lengths() const    { return lengths;  }
ushort    MPI_ComputeObj::get_nargs()   const    { return num_args; }
MPI_ComputeObj::callback 
          MPI_ComputeObj::get_callback() { return backup_function;  }
void   *  MPI_ComputeObj::get_obj()      { return obj;      }

void      MPI_ComputeObj::set_data ( void ** dat)     { data = dat;     }
void      MPI_ComputeObj::set_types( uint * tps)      { types = tps;    }
void      MPI_ComputeObj::set_lengths( size_t * lens) { lengths = lens; }
void      MPI_ComputeObj::set_nargs( ushort numargs)  { num_args = numargs; }
void      MPI_ComputeObj::set_callback( MPI_ComputeObj::callback cb){ backup_function = cb; }
void      MPI_ComputeObj::set_obj( void * o) { obj = o; }

