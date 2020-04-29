#include <mpi_proc_utils.h>
#include <numeric>
#include <ranges>
//////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////utilities////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////
std::pair<int, std::pair<std::vector<int>, std::vector<int> >  > 
  mpi_proc_utils::form_header(std::vector<int> types, std::vector<int> sizes)
{
  int bytes =0, lbytes=0;
  
  if( types.size() != sizes.size() )
  {
    std::cout << "size inconsistency... " << std::endl;
    return {0, { {}, {} } };
  }

  for(auto i : std::ranges::views::iota((size_t)0, types.size() ) )
  {
    MPI_Type_size(types[i], &lbytes);
    bytes += lbytes*sizes[i] + sizeof(int);
  }
  
  return { bytes, {types, sizes} };
}


//////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////rank_zero_vars/////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////

void mpi_proc_utils::rank_zero_vars::init_rank(ulong start_num)
{
  claimable_rank = start_num;
}

int mpi_proc_utils::rank_zero_vars::claim_rank()
{
  return claimable_rank++;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////proc_header///////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////

bool mpi_proc_utils::proc_header::operator==( const proc_header& rhs )
{
  bool flag = 
  ( rhs.sys_tag == sys_tag) &&
  ( rhs.msg_tag == msg_tag);

  return flag;
}

std::ostream& mpi_proc_utils::operator<<(std::ostream& out, const mpi_proc_utils::proc_header& prh )
{
   out << "{ sys_tag : " << prh.sys_tag <<"," <<   
   " msg_tag : " << prh.msg_tag <<"," <<   
   " msg_src_rank : " << prh.msg_src_rank <<"," <<   
   " seq_tag : " << prh.seq_tag <<"," <<   
   " total_seqs : " << prh.total_seqs <<"," <<   
   " method : " << (int) prh.method <<"," <<   
   " num_sec : " << (int) prh.num_sections <<"}" << std::endl;
   return out;

}


//////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////mpi_proc_pkt_builder///////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////
mpi_proc_utils::mpi_proc_pkt_builder::mpi_proc_pkt_builder(
                                           mpi_proc_mid method, 
                                           int total_size,
                                           int stag,
                                           int argc, 
                                           int seq_tag,
                                           int tot_seq)
{
  int int_sz = sizeof(int);
 
  _position = 0;
  _argc = argc;
  _cur_push_index = 0;
  _total_size = total_size + 5*sizeof(int);
  _data = v_mpi_data_t( malloc( _total_size ), mpi_pack_deleter );
  
  MPI_Pack(&stag,    int_sz, MPI_BYTE, _data.get(), _total_size, &_position, MPI_COMM_WORLD);
  MPI_Pack(&seq_tag, int_sz, MPI_BYTE, _data.get(), _total_size, &_position, MPI_COMM_WORLD);
  MPI_Pack(&tot_seq, int_sz, MPI_BYTE, _data.get(), _total_size, &_position, MPI_COMM_WORLD);
  MPI_Pack(&method,  int_sz, MPI_BYTE, _data.get(), _total_size, &_position, MPI_COMM_WORLD);
  MPI_Pack(&argc,    int_sz, MPI_BYTE, _data.get(), _total_size, &_position, MPI_COMM_WORLD);

}

mpi_proc_utils::mpi_proc_pkt_builder&
mpi_proc_utils::mpi_proc_pkt_builder::push_back_hdr( std::vector<int> types, 
                                                     std::vector<int> sizes )
{
  //craete contract for this check
  if( types.size() != sizes.size() ) 
    std::cout << " types vs. sizes mismatch" << std::endl;
  if( (types.size() != _argc) || (sizes.size() != _argc) ) 
    std::cout << "types.size() or sizes.size() doesnt equal _argc" << std::endl;

  if( types.size() > 0 )
    std::cout << " builder types: " << types[0] << ", sizes: " << sizes[0] << std::endl; 
  MPI_Pack(types.data(), _argc, MPI_INT, _data.get(), _total_size, &_position, MPI_COMM_WORLD);
  MPI_Pack(sizes.data(), _argc, MPI_INT, _data.get(), _total_size, &_position, MPI_COMM_WORLD);
  
  _types = types;
  _sizes = sizes;
 
  return *this;
}


mpi_proc_utils::mpi_proc_pkt_builder&
mpi_proc_utils::mpi_proc_pkt_builder::push_back_data( void const* data )
{
  int& i = _cur_push_index;
 
  //craete contract for this check
  if( i == _sizes.size() ) 
    std::cout << "size: too many sections pushed into envelope" << std::endl;
  if( i == _types.size() ) 
    std::cout << "types: too many sections pushed into envelope" << std::endl;

  std::cout << "{ size : " << _sizes.at(i) << ", " <<
  " types : " << _types.at(i) << ", " <<
  " total_size : " << _total_size << 
  " position : " << _position << " }" << std::endl;
    
  MPI_Pack(data, _sizes.at(i), _types.at(i), 
           _data.get(), _total_size, &_position, MPI_COMM_WORLD);

  _cur_push_index++;

  return *this;
}

v_mpi_data_t mpi_proc_utils::mpi_proc_pkt_builder::finalize(  )
{
  
  return std::move(_data);
}

//////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////mpi_proc_pkt_viewer//////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////
mpi_proc_utils::mpi_proc_pkt_viewer::mpi_proc_pkt_viewer(mpi_data_t&& data, 
                                                         bool skip_unpack,
                                                         size_t sz)
{
  int position = 0;
  int nsections = 0;
  //set of data start pos
  _data_start_pos = 0;
  //MPI_get_count gets the sz
  _data = std::move(data);
  _total_size = sz;

  //extract proc_header : hold only the data nd do nothing
  if( !skip_unpack ) 
  {
     std::cout << "MPI Unpacking data" << std::endl;
     const void * ldata = _data.get();
     //unpack header data
     MPI_Unpack(ldata, sz, &position, &prochdr.sys_tag,    1, MPI_INT, MPI_COMM_WORLD);
     MPI_Unpack(ldata, sz, &position, &prochdr.seq_tag,    1, MPI_INT, MPI_COMM_WORLD);
     MPI_Unpack(ldata, sz, &position, &prochdr.total_seqs, 1, MPI_INT, MPI_COMM_WORLD);
     MPI_Unpack(ldata, sz, &position, &prochdr.method,     1, MPI_INT, MPI_COMM_WORLD);
     MPI_Unpack(ldata, sz, &position, &nsections,          1, MPI_INT, MPI_COMM_WORLD);
     prochdr.num_sections = nsections;

     if( nsections != 0 )
     {      
       prochdr.types = std::vector<int>( nsections, 0 ); 
       prochdr.sizes = std::vector<int>( nsections, 0 );
       MPI_Unpack(ldata, sz, &position, prochdr.types.data(), nsections, MPI_INT, MPI_COMM_WORLD);
       MPI_Unpack(ldata, sz, &position, prochdr.sizes.data(), nsections, MPI_INT, MPI_COMM_WORLD);
     }
       //position of the first byte of data
       //need to unpack the rest of the data during 
     _data_start_pos = position;
  }
  
}

void mpi_proc_utils::mpi_proc_pkt_viewer::set_src_and_tag( int src_rank, int tag )
{
  prochdr.msg_src_rank = src_rank;
  prochdr.msg_tag      = tag;
}

const mpi_proc_utils::proc_header& mpi_proc_utils::mpi_proc_pkt_viewer::get_proc_header( ) const
{
  return prochdr;
}

int mpi_proc_utils::mpi_proc_pkt_viewer::get_src_rank() const
{
  return prochdr.msg_src_rank;
}

mpi_proc_utils::mpi_proc_mid mpi_proc_utils::mpi_proc_pkt_viewer::get_method()
{
  return prochdr.method;
}

int mpi_proc_utils::mpi_proc_pkt_viewer::get_msg_tag() const
{
  return prochdr.msg_tag;
}

int mpi_proc_utils::mpi_proc_pkt_viewer::get_sys_tag() const
{
  return prochdr.sys_tag;
}

int mpi_proc_utils::mpi_proc_pkt_viewer::get_argc()
{
  return prochdr.num_sections;
}

std::vector<int> mpi_proc_utils::mpi_proc_pkt_viewer::get_arg_types()
{
  return prochdr.types;
}

std::vector<int> mpi_proc_utils::mpi_proc_pkt_viewer::get_arg_sizes()
{
  return prochdr.sizes;
}

template< typename T>
T  mpi_proc_utils::mpi_proc_pkt_viewer::get_single_data( ulong arg_index )
{
  T single;
  get_data( &single, arg_index);
  return single;
}

template< typename T>
void mpi_proc_utils::mpi_proc_pkt_viewer::get_data( T * data, ulong arg_index )
{
  //create a post contract for type T and prochdr.types[arg_index]
  auto start = prochdr.sizes.begin();
  std::cout << "entering get_data ..." << std::endl; 
  int new_pos = _data_start_pos + std::accumulate( start, 
                                                   std::next(start, arg_index), 0 );

  std::cout << "header sizes : " << prochdr.sizes.size() << ", " <<
               prochdr.types.size() << std::endl;

  std::cout << "total_size : " << _total_size << ", new_pos : " << new_pos << 
               ", sizes : " << prochdr.sizes.at(arg_index) << ", types : " <<
               prochdr.types.at(arg_index) << std::endl;
  
  MPI_Unpack(_data.get(), _total_size, &new_pos, data, 
             prochdr.sizes.at(arg_index), 
             prochdr.types.at(arg_index), MPI_COMM_WORLD);
  
}

template void  mpi_proc_utils::mpi_proc_pkt_viewer::get_data<int>( int *, ulong );
template void  mpi_proc_utils::mpi_proc_pkt_viewer::get_data<ulong>( ulong *, ulong );
template int   mpi_proc_utils::mpi_proc_pkt_viewer::get_single_data<int>( ulong );
template ulong mpi_proc_utils::mpi_proc_pkt_viewer::get_single_data<ulong>( ulong );

