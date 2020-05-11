#include <cu_device_manager.h>
#include <cuda.h>
#include <iostream>
#include <stdlib.h>
#include <math.h>

__global__ void found_gpu()
{
  printf("Found GPU! \n"); 
  //__syncthreads();
}

void check_error()
{
  cudaError_t err = cudaGetLastError();
  if (err != cudaSuccess)
    printf("Error: %s\n", cudaGetErrorString(err));
}

void h_found_gpu()
{
  found_gpu<<<1,1>>>();
  cudaDeviceSynchronize();
  check_error();
}

template <typename T>
__global__ void cu_pw_method(int method, T * in, T * out, T bias, size_t sz)
{
  // Get our global thread ID
  int id = blockIdx.x*blockDim.x+threadIdx.x;
  // Make sure we do not go out of bounds
  if( method == 1) if (id < sz) out[id] = in[id] + bias;
  if( method == 2) if (id < sz) out[id] = in[id] * bias;
  if( method == 3) if (id < sz) out[id] = in[id] / bias;
}

template<typename T>
void h_pw_method( int method, const T * in, T * out, T bias, size_t len)
{
  T * _in, *_out;
  int blockSize, gridSize;
  size_t bytes = len*sizeof(T);
  blockSize = 32;
  // Allocate memory for each vector on GPU
  cudaMalloc(&_in, bytes );
  cudaMalloc(&_out, bytes );
  // Copy host vectors to device
  cudaMemcpy( _in, in, bytes, cudaMemcpyHostToDevice);
  // Number of threads in each thread block
  // Number of thread blocks in grid
  gridSize = (int)ceil(len/blockSize);
  cu_pw_method<<<gridSize, blockSize>>>(method, _in, _out, bias, len );
  // Copy array back to host
  cudaMemcpy( out, _out, bytes, cudaMemcpyDeviceToHost );
  cudaDeviceSynchronize();
  // Release device memory
  cudaFree(_in);
  cudaFree(_out);
}


template void h_pw_method( int method, const float * in, float * out, float bias, size_t len);
template void h_pw_method( int method, const double * in, double * out, double bias, size_t len);
template void h_pw_method( int method, const unsigned char * in, unsigned char * out, unsigned char bias, size_t len);
template void h_pw_method( int method, const char * in, char * out, char bias, size_t len);
template void h_pw_method( int method, const int * in, int * out, int bias, size_t len);
template void h_pw_method( int method, const unsigned int * in, unsigned int * out, unsigned int bias, size_t len);
template void h_pw_method( int method, const long * in, long * out, long bias, size_t len);
template void h_pw_method( int method, const unsigned long * in, unsigned long * out, unsigned long bias, size_t len);;
template void h_pw_method( int method, const unsigned long long * in, unsigned long long * out, unsigned long long bias, size_t len);
template void h_pw_method( int method, const long long * in, long long * out, long long bias, size_t len);
