#include <mpi.h>
#include <vector>
#include <client_utils.h>


#ifndef MPIPROCUTILS
#define MPIPROCUTILS

namespace mpi_proc_utils
{
  
  std::pair<int, std::pair<std::vector<int>, std::vector<int> >  > 
    form_header(std::vector<int>, std::vector<int> );

  struct rank_zero_vars{
    int claimable_rank=1;
    int claim_rank();
    void init_rank( ulong );
  };

  enum struct mpi_proc_mid : int {
    response,
    passthrough,
    get_groupid
  };

  struct proc_header{

    int sys_tag;            //system tag
    int msg_tag;            //MPI tag
    int msg_src_rank;       //mpi source rank
    int seq_tag;            //sequence number
    int total_seqs;         //total number of sequences
    mpi_proc_mid method;    //method
    int num_sections;       //number of sections
    std::vector<int> types; //set of types
    std::vector<int> sizes; //set of sizes

    bool operator==( const proc_header& rhs);
    friend std::ostream& operator<<(std::ostream&, const mpi_proc_utils::proc_header& );
  };

  std::ostream& operator<< (std::ostream&, const mpi_proc_utils::proc_header& );

  class mpi_proc_pkt_builder
  {

    public:
      mpi_proc_pkt_builder( mpi_proc_mid, int, int, int argc =1, int seq=1, int tot_seq=1 );

      mpi_proc_pkt_builder& push_back_hdr( std::vector<int>, std::vector<int>  );
  
      mpi_proc_pkt_builder& push_back_data( void const * );

      v_mpi_data_t finalize();

      int get_byte_size() { return _total_size; }

    private:
      int _argc;
      int _cur_push_index;
      int _position;
      int _total_size;
      std::vector<int> _types; //set of types
      std::vector<int> _sizes; //set of sizes
      v_mpi_data_t _data;        
      friend class mpi_proc_impl;
   
  };

  class mpi_proc_pkt_viewer
  {
    public:

      mpi_proc_pkt_viewer( mpi_data_t&&, bool,  size_t);

      void set_src_and_tag( int, int );

      const proc_header& get_proc_header( ) const;

      int get_src_rank() const;

      int get_msg_tag() const;

      int get_sys_tag() const;

      mpi_proc_mid get_method();

      int get_argc();

      std::vector<int> get_arg_types( );
      std::vector<int> get_arg_sizes( );
      
      template< typename T>
      void get_data( T*, ulong );
 
      template< typename T>
      T get_single_data( ulong );

    private:
      proc_header prochdr;
      mpi_data_t _data;
      size_t _total_size;
      size_t _data_start_pos; //starting position of the data

  };


}

#endif

