#include "device_manager.h"
#include "cu_device_manager.h"
#include <type_traits>
#include <boost/range/algorithm.hpp>
#include <boost/algorithm/string.hpp>

device_manager::device_manager()
{

  auto _add_kernel = [&](std::string desc, auto func)
  {
    std::string intel_desc = std::string("hw_vid=10de:hw_pid=107d:") +
                             std::string("hw_ss_vid=10de:hw_ss_pid=094e:") +
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
  _add_kernel( "*",       &device_manager::_pw_add   ); 
  _add_kernel( "12345",   &device_manager::_pw_add   );
  _add_kernel( "123456",  &device_manager::_pw_mult  );
  _add_kernel( "1234567", &device_manager::_pw_div   );

  //Saying HI to the GPU
  h_found_gpu();
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
        //GPU call
        h_pw_method(1, fin, rbuf, (float)1, size);

      }
      else if( t_size == sizeof(double) )
      {
        std::cout << "real double..." << std::endl;
        auto rbuf = (double *) results;
        auto fin = (const double *) data;
        h_pw_method(1, fin, rbuf, (double)1, size);
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
          h_pw_method(1, fin, rbuf, (unsigned char)1, size);
        }
        else
        {
          std::cout << "signed char..." << std::endl;
          auto rbuf = (char *) results;
          auto fin  = (char *) data;
          h_pw_method(1, fin, rbuf, (char)1, size);
        }
         
      }
      else if( t_size == sizeof(int) ) 
      {
        if( sign_t == ARG_UNSIGNED )
        {
          std::cout << "unsigned int..." << std::endl;
          auto rbuf = (unsigned int *) results;
          auto fin  = (unsigned int *) data;
          h_pw_method(1, fin, rbuf, (unsigned int)1, size);
        }
        else
        {
          std::cout << "signed int..." << std::endl;
          auto rbuf = (int *) results;
          auto fin  = (int *) data;
          h_pw_method(1, fin, rbuf, (int)1, size);
        }

      }
      else if( t_size == sizeof(long) )
      {
        if( sign_t == ARG_UNSIGNED )
        {
          std::cout << "unsigned long..." << std::endl;
          auto rbuf = (unsigned long *) results;
          auto fin  = (unsigned long *) data;
          h_pw_method(1, fin, rbuf, (unsigned long)1, size);
        }
        else
        {
          std::cout << "signed long..." << std::endl;
          auto rbuf = (long *) results;
          auto fin  = (long *) data;
          h_pw_method(1, fin, rbuf, (long)1, size);
        }

      }
      else if( t_size == sizeof(long long) )
      {
        if( sign_t == ARG_UNSIGNED )
        {
          std::cout << "unsigned long long..." << std::endl;
          auto rbuf = (unsigned long long *) results;
          auto fin  = (unsigned long long *) data;
          h_pw_method(1, fin, rbuf, (unsigned long long)1, size);
        }
        else
        {
          std::cout << "signed long long..." << std::endl;
          auto rbuf = (long long *) results;
          auto fin  = (long long *) data;
          h_pw_method(1, fin, rbuf, (long long)1, size);
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

device_manager::return_type device_manager::_pw_mult ( header_type hdr, input_type input)
{ 
  std::cout << "---dm->mult..." << std::endl;
  return_type output;

  return output;
}

device_manager::return_type device_manager::_pw_div ( header_type hdr, input_type input)
{
  std::cout << "---dm->div..." << std::endl; 
  return_type output;

  return output;
}

device_manager::return_type device_manager::_default ( header_type hdr, input_type input)
{
  std::cout << "---dm->default..." << std::endl; 
  return_type output;

  return output;
}
