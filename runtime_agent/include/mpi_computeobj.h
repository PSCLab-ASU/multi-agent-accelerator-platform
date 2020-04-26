#include <sys/types.h>
#include <runtime_ctx.h>

#ifndef MPICOMPOBJ
#define MPICOMPOBJ

struct MPI_ComputeMap {};

struct MPI_ComputeObj
{
  typedef MPI_ComputeObj*(*callback)(MPI_ComputeObj*, struct runtime_ctx);

  void   ** data;
  uint   *  types;
  size_t *  lengths;
  ushort    num_args;
  callback  backup_function;
  void   *  obj;
  
  //accessors
  void   ** get_data()    const;
  uint   *  get_types()   const;
  size_t *  get_lengths() const;
  ushort    get_nargs()   const;
  callback  get_callback();
  void   *  get_obj();

  //mutators
  void set_data     ( void  ** );
  void set_types    ( uint   * );
  void set_lengths  ( size_t * );
  void set_nargs    ( ushort   );
  void set_callback ( callback );
  void set_obj      ( void   * ); 

  //structured routines
  //input utilities
  /*void insert_input( uint, const void *, uint, size_t);
  void push_input( const void *, uint, size_t);
  void insert_full_input( uint, const void *, uint);
  void push_full_input( const void *, uint ); 
  //output methods
  void * get_output( uint, uint*, size_t* );
  void * pop_output( uint*, size*);
  //higher level functions
  void freeze();
  MPI_ComputeObj copy();
  MPI_ComputeObj copy_freeze();
  MPI_ComputeObj freeze_copy();
  //transfer methods
  //              dst obj        this.out / dst.in
  void transfer( MPI_ComputeObj, uint,       uint );
  //fuse methods
  MPI_ComputeObj fuse_compute( MPI_ComputeObj, MPI_ComputObj, MPI_ComputeMap );
  _cnode * state;
  */
  
};


#endif
