#include "payload_details.h"
#include "utils.h" 

template<typename T>
misc_payload<T>::misc_payload(const misc_payload::v_type& data)
{
  vdata = data;
}

register_payload::register_payload(std::string filters)
{
  auto vec = split_str<','>(filters);
  std::transform( vec.begin(), vec.end(), hw_desc.begin(),
                  []( const std::string& vidpids ) -> components
  {
    return components( vidpids );
  } );    
}

register_payload::components::components(std::string vidpids)
{
  auto vec = split_str<':'>(vidpids);
  auto size = vec.size(); 

  switch ( size )
  {
    case 4: vid    = vec[0];
            pid    = vec[1];
            ss_vid = vec[2];
            ss_pid = vec[3];
            break;
    case 3: vid    = vec[0];
            pid    = vec[1];
            ss_vid = vec[2];
            break;
    case 2: vid    = vec[0];
            pid    = vec[1];
            break;
    case 1: vid    = vec[0];
            break;
 
  }

}

std::string register_payload::components::stringify()
{
  return vid + ":" + pid + ":" + ss_vid + ":" + ss_pid;
}

std::vector<std::string> register_payload::transform()
{
  std::vector<std::string> new_vector;
  std::transform( hw_desc.begin(), hw_desc.end(), new_vector.begin(),
                  []( components& comp )
                  { return comp.stringify(); } );

  return new_vector;
}

identify_payload::identify_payload( const bool& bAppend, 
                                    const typename misc_string_payload::v_type& dirs )
: Append(bAppend)
{
  set_data( dirs );
}

send_payload::send_payload( arg_headers_t&& headers, base_type_t && base_data ) 
: arg_definition( std::forward<base_type_t>( base_data ) ) 
{
  std::cout << "creating send_payload ... " << std::endl;
  auto& data = get_const_bdata();
  set_nargs( headers.size() );  
 
  if( headers.size() != data.size() )
    std::cout << "-- Unbalanced send_payload ... " << std::endl;

  //create elements
  for( auto i : iota(0, get_nargs() ) )
  {
    //std::cout << "type = " << types[i] << ", size = " << sizes[i] << std::endl;
    //for(int j=0; j < sizes[i]; j++ ) std::cout << (((const float *) vpdata[i])[j]) << std::endl;  
    push_arg({ headers[i], data[i].data() });

  }
}

send_payload::header send_payload::get_header() const
{
  return _hdr;
}

//this first ushort is the number of args
//which keeps getting updated every allocate_back;
recv_payload::recv_payload( ) : arg_definition({}){}

/*void * recv_payload::allocate_back( arg_header_t argh )
{
  auto[sign_t, data_t, t_size, size] = argh;
  //step 1: get base data
  auto& base_data = get_bdata();
  //step 2: push new arg into base data
  base_data.emplace_back(malloc( t_size * size), recv_deleter);
  //step 3: recalculate all arg pointers
  //step 4: update first ushort which equals number of args
  return malloc(0);
}
*/
template misc_payload<std::string>::misc_payload( const std::vector<std::string>& );
template misc_payload<string_pair>::misc_payload( const std::vector<string_pair>& );
