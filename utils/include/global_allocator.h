#include <map>
#include <string>
#include <tuple>
#include <iostream>
#include <optional>
#include <memory>
#include <mutex>
#include <variant>
#include <utility>
#include <vector>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <cstring>

#pragma once

struct allocate_and_add {};
struct add_only {};

struct buffer_description
{
  
  bool operator ==( const buffer_description& rhs ) const = default;

  bool is_signed;
  bool is_float;
  int type_size;
  size_t vector_size;
  std::optional<std::pair<std::string, std::string> > key_child_id;
  std::optional<int> shmid;
  bool attached = false;;
  bool is_allocated = false;

  std::string  get_key() const;
  void         set_key( std::string );
  std::string  concat_get_key( std::optional<std::string> ) const;
  std::string  get_child_rank() const;
  size_t       get_total_bytes() const;
  void         detach() { attached = false; }
  void         attach() { attached = true;  }
  void         set_shmid( int id ){ shmid = id; }
  int          get_shmid() { return shmid.value_or(0); }
  bool         is_attached() { return attached; }

};

struct Buffer
{
  Buffer(buffer_description, allocate_and_add );  /* allocation and add */
  Buffer(buffer_description, add_only );  /* no allocation */
  void detach();
  void attach(std::optional<int> shmid = {} );
  std::shared_ptr<void> get_shared_data() { return _data; }
  buffer_description& header(){ return _header; }
  void set_header( buffer_description bd) { _header = bd; }
  void * data() { return _data.get(); }

  operator std::shared_ptr<void>(){ return _data;   }
  operator buffer_description()   { return _header; }

  buffer_description _header;
  std::shared_ptr<void> _data;
  void _show_error();
};

class global_allocator
{
 
  public : 
   
   static std::shared_ptr<global_allocator> get_global_allocator();

   Buffer allocate_global_memory( buffer_description, std::optional<std::string> suffix= {} );
 
   Buffer realloc_buffer( buffer_description, buffer_description);

   void remove_buffer( std::string );

   void attach( std::string key) { _tach( key, true ); }
   void detach( std::string key) { _tach( key, false); } 

   std::optional<Buffer> 
   get_buffer( std::variant<std::string, void *>  );

   Buffer update_or_allocate( buffer_description, std::optional<std::string> suff = {}, 
                              std::optional<void *> old_ptr = {}, bool memcpy=false);  

   std::optional<Buffer>
   update_buffer( buffer_description, void *, bool );

   void global_memcopy( std::shared_ptr<void>, void *, size_t );

   std::optional<Buffer> add_buffer( buffer_description ); 

   void unlock();

   void lock();
  
   bool is_global_buffer( std::string );
   bool is_global_buffer( void * );

  
  private : 

    global_allocator();

    void _tach( std::string, bool );

    static std::shared_ptr<global_allocator> _g_ptr;
    

    std::vector<Buffer> _allocation_registry;

    std::mutex _mu;
    std::unique_lock<std::mutex> _global_mallock;

};

//std::shared_ptr<global_allocator> g_global_allocator = NULL;
