#include <global_allocator.h>
#include <boost/range/algorithm.hpp>
//#include <boost/range/algorithm/find_if.hpp>
//#include <boost/range/algorithm/remove_if.hpp>
#include <errno.h>

////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////
std::shared_ptr<global_allocator> global_allocator::_g_ptr;

////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////Buffer Description/////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////

std::string buffer_description::get_key() const
{
  if( key_child_id ) return key_child_id->first;
  else return "";
}

void buffer_description::set_key( std::string key)
{
   if( key_child_id )
     key_child_id = {key, key_child_id->second };
   else key_child_id = {key, ""}; 
}

std::string buffer_description::concat_get_key(std::optional<std::string> suffix) const
{
  if( suffix ) return get_key() + "_" + suffix.value();
  else return get_key();
}

std::string buffer_description::get_child_rank() const
{
  if( key_child_id ) return key_child_id->second;
  else return "";
}

size_t buffer_description::get_total_bytes() const
{
  return type_size*vector_size;
}

////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////Buffer/////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////
void Buffer::_show_error()
{
  char buffer[ 256 ];
  char * errorMsg = strerror_r( errno, buffer, 256 ); // GNU-specific version, Linux default
  printf("Error %s", errorMsg); //return value has to be used since buffer might not be modified

}

Buffer::Buffer( buffer_description bdesc, allocate_and_add )
{
  std::string key_str = bdesc.get_key();
  std::cout << "Allocating new buffer ... " << std::endl;
  
  //ftok to generate unique key
  key_t key = ftok(key_str.c_str(),65);
  // shmget returns an identifier in shmid
  int shmid = shmget(key, bdesc.get_total_bytes(), 0666|IPC_CREAT|IPC_EXCL );
  if( shmid == -1)
  {
    _show_error();
    throw std::runtime_error("failed to create new shared memory..." );
  }
  //guarantee that buffer aren't mis-charterized as attach by user
  bdesc.detach();
  //move description to buffer
  _header = bdesc;
  //attach buffer
  attach( shmid ); 

}

Buffer::Buffer( buffer_description bdesc, add_only )
{
  std::string key_str = bdesc.get_key();

  std::cout << "Adding buffer ... " << std::endl;
  //ftok to generate unique key
  key_t key = ftok(key_str.c_str(),65);
  // shmget returns an identifier in shmid
  int shmid = shmget(key, bdesc.get_total_bytes(), 0666 );

  if( shmid == -1)
  {
    _show_error();
    throw std::runtime_error("failed to get shared memory..." );
  }
  //guarantee that buffer aren't mis-charterized as attach by user
  bdesc.detach();
  //move description to buffer
  _header = bdesc;
  //attach buffer
  attach( shmid ); 

}

void Buffer::attach( std::optional<int> shmid )
{
  
  auto _deleter = [shmid](void * global_ptr)
  {
    std::cout << "Deleteing buffer ... " << std::endl;
    shmdt( global_ptr );
    shmctl( shmid.value(), IPC_RMID, NULL );
      
  };

  if( !_header.is_attached() )
  {
    int id = shmid.value_or( _header.get_shmid() );
    void * new_data = shmat(id, nullptr, 0);
    
    if( new_data != ( (void *) -1) )
    {
      auto data = std::shared_ptr<void>( new_data, _deleter );
      _data.swap( data );
      //set shared memory id
      _header.set_shmid( id );
      //detach upon failure
      _header.attach();
    }
    else 
    {
      _header.detach();
      std::cout << " Could not attach memory : " << header().get_key() << std::endl;
    }
    
  } 
  else std::cout << "buffer already attached" << std::endl;

}

void Buffer::detach()
{
  if( _header.is_attached() )
  {
    //std::cout << "Detaching buffer ... " << std::endl;
    shmdt( _data.get() );
    _header.detach();
  }
  else std::cout << " Buffer already detached " << std::endl;
} 

////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////Global Allocator/////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////

global_allocator::global_allocator()
: _global_mallock( _mu, std::defer_lock)
{
 //empty ctor
}

std::shared_ptr<global_allocator> 
global_allocator::get_global_allocator()
{
  
  struct make_shared_enabler : public global_allocator {};

  if( _g_ptr ) return _g_ptr;
  //else _g_ptr = std::shared_ptr<global_allocator>(new global_allocator);
  else _g_ptr = std::make_shared<make_shared_enabler>( );

  return _g_ptr;
}

void global_allocator::_tach( std::string key, bool attach )
{
  auto iter = boost::range::find_if(_allocation_registry, [&](auto& alloc_entry) 
  {
    return ( key == alloc_entry.header().get_key() );       
  } );

  if( iter != _allocation_registry.end() )
  {
    if( attach ) iter->attach();
    else iter->detach();
  } 
  else std::cout << "Attach/Detached failed : could no find " << key << std::endl;

}

void global_allocator::remove_buffer( std::string key )
{
  auto find_by_key = [&]( Buffer entry )
  {
    return ( key == entry.header().get_key() );       
  };

  //remove entry from registry
  boost::remove_if( _allocation_registry, find_by_key);
}

Buffer
global_allocator::allocate_global_memory( buffer_description old_buff_desc, 
                                          std::optional<std::string> suffix )
{
  std::string key_str = old_buff_desc.concat_get_key( suffix );
  
  //both should refturn references
  auto selected_buffer  = get_buffer( key_str );
 
  if( !selected_buffer )  //no allocation exists
  {
    old_buff_desc.set_key( key_str );
    return _allocation_registry.emplace_back( old_buff_desc, allocate_and_add{} );
    
  }
  else 
  {
    std::cout << "Allocation already exists" << std::endl;
    throw std::range_error("Allocation already exists" );
  }
  
}

Buffer
global_allocator::update_or_allocate( buffer_description new_buffer_desc, 
                                      std::optional<std::string> suffix, 
                                      std::optional<void *> old_ptr, bool memcopy )
{
  if( old_ptr )
  {
    if( is_global_buffer( old_ptr.value() ) )
    {
      return update_buffer(new_buffer_desc, old_ptr.value(), memcopy).value();
    }
    else
    {
      Buffer new_buffer = allocate_global_memory( new_buffer_desc, suffix );

      size_t numBytes = new_buffer.header().get_total_bytes();

      if( memcopy ) global_memcopy(new_buffer.get_shared_data(), old_ptr.value(), numBytes );

      return new_buffer;
    }
  }
  else
  {
    std::cout << "Allocating new buffer : " << new_buffer_desc.get_key() << std::endl; 
    Buffer new_buffer = allocate_global_memory( new_buffer_desc, suffix );
    return new_buffer;
  }
}

 
void global_allocator::global_memcopy(std::shared_ptr<void> dst, void * src, size_t num_bytes)
{
  std::memcpy ( dst.get(), src, num_bytes );
}


std::optional<Buffer>
global_allocator::update_buffer( buffer_description new_buffer_desc, void * old_ptr, bool memcopy)
{
  //both should refturn references
  auto selected_buffer  = get_buffer( old_ptr );

  if( selected_buffer && selected_buffer->data() )
  {
    if( new_buffer_desc != selected_buffer->header() )
    {
      auto new_buffer = realloc_buffer( selected_buffer->header(), new_buffer_desc);

      auto numBytes = new_buffer_desc.get_total_bytes(); 
      if( memcopy ) global_memcopy(new_buffer.get_shared_data(), old_ptr, numBytes );
      return new_buffer;
    }
    else 
    {
      std::cout << "No change to buffer" << std::endl;
      return {};
    }
  }
  else return {};
  
}

Buffer
global_allocator::realloc_buffer( buffer_description old_buffer_desc, buffer_description new_buffer_desc )
{
  //old buffer has the correct key or child Id
  //new one may not
 throw std::runtime_error("_realloc_buffer no implemented");
}


std::optional<Buffer> 
global_allocator::get_buffer( std::variant<std::string, void * > lookup )
{
  Buffer * selected_buffer;
  bool exists = false;

  std::visit([&]( auto val )
  {
    using T = std::decay_t<decltype(val)>;
    if constexpr (std::is_same_v<T, std::string> )
    {
      auto iter = boost::range::find_if(this->_allocation_registry, [&](auto buffer )
      {
        return ( val == buffer.header().get_key() );
      });

      exists = ( iter != this->_allocation_registry.end() );
      if( exists ) *selected_buffer = *iter;

    }
    else
    {
      auto iter = boost::range::find_if(this->_allocation_registry, [&](auto buffer )
      {
        return ( val == buffer.data() );
      });

      exists = ( iter != this->_allocation_registry.end() );
      if( exists ) *selected_buffer = *iter;
    }

  }, lookup );
 
  std::cout << "Getting buffer : " << exists << std::endl; 
  if( !exists ) return {};
  else return *selected_buffer;

}

std::optional<Buffer> 
global_allocator::add_buffer( buffer_description buffer_desc)
{
  auto iter = boost::range::find_if( _allocation_registry,
  [&](auto buffer){
    return (buffer_desc == buffer.header() );
  });

  if( iter == std::end(_allocation_registry) )
  {
    auto buffer = _allocation_registry.emplace_back( buffer_desc, add_only{} );
    return buffer;
  }
  else
  {
    return *iter;
  }
  
}

bool global_allocator::is_global_buffer( std::string key)
{

  return bool( get_buffer( key ) );

}

bool global_allocator::is_global_buffer( void * g_ptr)
{

  return bool(get_buffer( g_ptr ) );

}


void global_allocator::lock()
{
  _global_mallock.lock();
}

void global_allocator::unlock()
{
  _global_mallock.unlock();
}
