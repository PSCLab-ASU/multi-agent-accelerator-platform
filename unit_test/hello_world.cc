#include <iostream>
#include <mpi_ext.h>
#include <unistd.h>

const char add_func[8] = "VEC_ADD";
const char mult_func[9] = "VEC_MULT";

MPI_ComputeObj* backup(MPI_ComputeObj* cobj, struct runtime_ctx)
{
  std::cout <<"Sample test_pipe"<<std::endl;
  return new MPI_ComputeObj{};
}

void addition(ulong rank )
{
  const uint nargs  = 1;
  uint len=32;
  float vec[32]  = { 1,2,3,4,5,6,7,8,9,10,11,12,13,55,23,100,
                     1,2,3,4,5,6,7,8,9,10,11,12,13,55,23,100  };
  void * data[nargs]    = { vec };
  uint types [nargs]    = { MPI_FLOAT }; 
  size_t lens[nargs]    = { len };
  MPI_ComputeObj computeObj{data, types, lens, 1};
  MPI_Status status;
  //////////////////////////////////////////////////////////////////////////////////
  //////////////////////////Vector Addition/////////////////////////////////////////
  //////////////////////////////////////////////////////////////////////////////////
  printf("Sending data for addition...\n" );
  MPIX_Send(&computeObj, 1, MPIX_COMPOBJ, rank, 0, MPI_COMM_WORLD);
  printf("Recv data result of addition...\n" );
  MPIX_Recv(&computeObj, 1, MPIX_COMPOBJ, rank, 0, MPI_COMM_WORLD, &status);

}

void multiply( ulong rank )
{
  const uint nargs  = 1;
  uint len=32;
  float vec[32]  = { 1,2,3,4,5,6,7,8,9,10,11,12,13,55,23,100,
                     1,2,3,4,5,6,7,8,9,10,11,12,13,55,23,100  };
  void * data[nargs]    = { vec };
  uint types [nargs]    = { MPI_FLOAT }; 
  size_t lens[nargs]    = { len };
  MPI_ComputeObj computeObj{data, types, lens, 1};
  MPI_Status status;
  //////////////////////////////////////////////////////////////////////////////////
  //////////////////////////Vector Multiplication///////////////////////////////////
  //////////////////////////////////////////////////////////////////////////////////
  printf("Sending data for multiplication...\n" );
  MPIX_Send(&computeObj, 1, MPIX_COMPOBJ, rank, 0, MPI_COMM_WORLD);
  printf("Recv data result of multiplication...\n" );
  MPIX_Recv(&computeObj, 1, MPIX_COMPOBJ, rank, 0, MPI_COMM_WORLD, &status);


}


int main(int argc, char * argv[])
{
  ulong new_rank[2]= {0,0};
  MPI_Info info;
  MPIX_Init(&argc, &argv);

  printf("Claiming : %s\n", add_func ); 
  MPIX_Claim(add_func, backup, info, &new_rank[0]);
  printf("(Addition Rank : %i )\n", new_rank[0] ); 
  printf("Claiming : %s\n", mult_func ); 
  MPIX_Claim(mult_func, backup, info, &new_rank[1]);
  printf("(Multiplication Rank : %i )\n", new_rank[1] ); 
  //////////////////////////////////////////////////////////////////////////////////
  addition( new_rank[0] );
  multiply( new_rank[1] );
  //////////////////////////////////////////////////////////////////////////////////
  /*ushort   o_nargs   = computeObj.get_nargs();
  float ** o_data    = (float **) computeObj.get_data();
  size_t * o_lengths = computeObj.get_lengths();

  for( int i=0; i < o_nargs; i++ )
  {
    printf("Output %i recieved %i\n", i, o_lengths[i] ); 
    for(int j=0; j < o_lengths[i]; j++)
    {
      printf("data[%i] = %f\n", j, o_data[i][j] );
    } 
  } 
  */
  printf("Completed Acceleration Demo\n" ); 
  MPIX_Finalize();

  return 0;
}
