#include "device_manager.h"
#include <type_traits>
#include <boost/range/algorithm.hpp>
#include <boost/algorithm/string.hpp>

#include <stdio.h>
#include <stdlib.h>
#include "mkl.h"

#define LOOP_COUNT 1

device_manager::device_manager()
{

  auto _add_kernel = [&](std::string desc, auto func)
  {
    std::string intel_desc = std::string("hw_vid=8086:hw_pid=2f80:") +
                             std::string("hw_ss_vid=8086:hw_ss_pid=2f80:") +
                             std::string("sw_vid=*:sw_pid=*:") +
                             std::string("sw_fid=[INSERT_FUNC_ID]:sw_clid=*:sw_verid=*");
    //substite template
    intel_desc = boost::replace_first_copy(intel_desc, "[INSERT_FUNC_ID]", desc);
    //add to manifest
    this->add_kernel_desc( intel_desc );
    //add execution unit
    this->add_execution_method( lookup_type(intel_desc), this, func);
  };

  add_execution_method( lookup_type("sw_fid=default"), this, &device_manager::_default );

  //device manager should check if hw existsa
  //and add the * fid if hardware exists
	_add_kernel( "*",       &device_manager::_default    ); 
  _add_kernel( "0",       &device_manager::_pw_gemm  );
  _add_kernel( "1",       &device_manager::_pw_add  );
}

void device_manager::heartbeat()
{
  // std::cout << "entering device_manager::heartbeat()" << std::endl;

}

device_manager::return_type device_manager::_pw_gemm ( header_type hdr, input_type input)
{
  std::cout << "---dm->gemm..." << std::endl;

  auto& in_payload = input.second.get(); 
  unsigned num_in_args = in_payload.get_size();

  std::vector<arg_element_t> payload_data;

  for(int i=0; i<num_in_args; i++){
    payload_data.push_back(in_payload.pop_arg());
  }	
	
  int dim[3];

  for(int i = 2; i>=0; i--) 
  {
    auto[header, data] = payload_data[i];
    auto[sign_t, data_t, t_size, size] = header;
    dim[i] = *((int *) data);
  }
  
  int in_size[2];
  in_size[0] = dim[0] * dim[1];
  in_size[1] = dim[1] * dim[2];
  
  float* in_buf[2];
  
  unsigned index = num_in_args-1;
  
  for(int i = 0; i < 2; i++) 
  {
    auto[header, data] = payload_data[index--];
    auto[sign_t, data_t, t_size, size] = header;
	
	in_buf[i] = (float *)mkl_malloc( in_size[i] * sizeof( float ), 64 );
    for(int j = 0; j < in_size[i]; j++) in_buf[i][j] = *((float *) data + j);
  }
  
  
  float alpha, beta;
  alpha = 1.0; beta = 0.0; 
	
  float* out_buf= (float *)mkl_malloc( dim[0] * dim[2] * sizeof( float ), 64 );
	
  double s_initial, s_end, s_elapsed;
		
  s_initial = dsecnd();
  for (int r = 0; r < LOOP_COUNT; r++) {
    cblas_sgemm(CblasRowMajor, CblasNoTrans, CblasNoTrans, dim[0], dim[2], dim[1], alpha, in_buf[0], dim[1], in_buf[1], dim[2], beta, out_buf, dim[2]);
  }
	
  s_end = dsecnd();
  s_elapsed = (s_end - s_initial) / LOOP_COUNT;

  printf (" == Matrix multiplication using Intel(R) MKL dgemm completed == \n"
    " == at %.5f milliseconds == \n\n", (s_elapsed * 1000));  

  return_type output;
  recv_payload rp;  

  void * results = nullptr;

  rp.set_tid( input.first );
  rp.set_nargs( 1 );

  auto header = std::make_tuple(ARG_SIGNED, ARG_REAL, 4, dim[0] * dim[2]);
  results = rp.allocate_back( header ).get();
  
  for(int i=0; i < dim[0] * dim[2]; i++ ) *((float *)results + i) = out_buf[i];

  output.push_back( std::move( rp ) );
  
  mkl_free(in_buf[0]);
  mkl_free(in_buf[1]);
  mkl_free(out_buf);

  return output;
}

device_manager::return_type device_manager::_default ( header_type hdr, input_type input)
{
  std::cout << "---dm->default..." << std::endl; 
  return_type output;

  return output;
}

device_manager::return_type device_manager::_pw_add ( header_type hdr, input_type input)
{
  std::cout << "---dm->add..." << std::endl;

  return_type output;
  recv_payload rp;  

  void * results = nullptr;
  bool valid_type = true;

  auto& payload = input.second.get(); 
  rp.set_tid( input.first );
  rp.set_nargs( 1 );

  if( payload.get_size() == 1) 
  {
    auto[header, data ] = payload.pop_arg();
    auto[sign_t, data_t, t_size, size] = header;
    
    std::cout << "size = "<< size << std::endl;
   
    results = rp.allocate_back( header ).get();

    if( data_t == ARG_REAL )
    {
      if( t_size == sizeof(float) )
      {
        std::cout << "real float..." << std::endl;
        auto rbuf = (float *) results;
        auto fin = (const float *) data;
        for(int i=0; i < size; i++ ) rbuf[i] = fin[i] + 1;

      }
      else if( t_size == sizeof(double) )
      {
        std::cout << "real double..." << std::endl;
        auto rbuf = (double *) results;
        auto fin = (const double *) data;
        for(int i=0; i < size; i++ ) rbuf[i] = fin[i] + 1;
      } 
      else std::cout << "real type size unsupported..." << std::endl;
    
    } 
    else if( data_t == ARG_INT )
    {
      if( t_size == sizeof(char) )
      {
        if( sign_t == ARG_UNSIGNED )
        {
          std::cout << "unsigned char..." << std::endl;
          auto rbuf = (unsigned char *) results;
          auto fin  = (unsigned char *) data;
          for(int i=0; i < size; i++ ) rbuf[i] = fin[i] + 1;
        }
        else
        {
          std::cout << "signed char..." << std::endl;
          auto rbuf = (char *) results;
          auto fin  = (char *) data;
          for(int i=0; i < size; i++ ) rbuf[i] = fin[i] + 1;
        }
         
      }
      else if( t_size == sizeof(int) ) 
      {
        if( sign_t == ARG_UNSIGNED )
        {
          std::cout << "unsigned int..." << std::endl;
          auto rbuf = (unsigned int *) results;
          auto fin  = (unsigned int *) data;
          for(int i=0; i < size; i++ ) rbuf[i] = fin[i] + 1;
        }
        else
        {
          std::cout << "signed int..." << std::endl;
          auto rbuf = (int *) results;
          auto fin  = (int *) data;
          for(int i=0; i < size; i++ ) rbuf[i] = fin[i] + 1;
        }

      }
      else if( t_size == sizeof(long) )
      {
        if( sign_t == ARG_UNSIGNED )
        {
          std::cout << "unsigned long..." << std::endl;
          auto rbuf = (unsigned long *) results;
          auto fin  = (unsigned long *) data;
          for(int i=0; i < size; i++ ) rbuf[i] = fin[i] + 1;
        }
        else
        {
          std::cout << "signed long..." << std::endl;
          auto rbuf = (long *) results;
          auto fin  = (long *) data;
          for(int i=0; i < size; i++ ) rbuf[i] = fin[i] + 1;
        }

      }
      else if( t_size == sizeof(long long) )
      {
        if( sign_t == ARG_UNSIGNED )
        {
          std::cout << "unsigned long long..." << std::endl;
          auto rbuf = (unsigned long long *) results;
          auto fin  = (unsigned long long *) data;
          for(int i=0; i < size; i++ ) rbuf[i] = fin[i] + 1;
        }
        else
        {
          std::cout << "signed long long..." << std::endl;
          auto rbuf = (long long *) results;
          auto fin  = (long long *) data;
          for(int i=0; i < size; i++ ) rbuf[i] = fin[i] + 1;
        }

      }
      else std::cout << "integer type size unsupported..." << std::endl;
 
    }
    else 
    { 
      valid_type = false;
      std::cout << "Could not determine type..." << std::endl;
    }

  }
  else 
  {
     valid_type = false;
     std::cout << "Size is incorrect: expected 1, got " << payload.get_size() << std::endl; 
  }

  //add to the output
  output.push_back( std::move( rp ) );

  return output;
 
}