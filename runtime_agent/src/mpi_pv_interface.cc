#include <mpi_pv_interface.h>

mpi_pv_interface::mpi_pv_interface()
: _stop_driver(false),
  _async_enabled(false),
  _mu( std::make_shared<std::mutex>() ),
  _lk( std::unique_lock(*_mu, std::defer_lock) ){}

mpi_pv_interface::mpi_pv_interface( bool async)
: _stop_driver(false),
  _async_enabled( async ),
  _mu( std::make_shared<std::mutex>() ),
  _lk( std::unique_lock(*_mu, std::defer_lock) ) {}

void mpi_pv_interface::thread_main()
{
  init_thread_components();

  while( !_stop_driver.load() )
  {
    {
      //aquire lock for application priority
      std::lock_guard<std::mutex> guard(*_mu);
      if( _stop_driver.load() ) break;
      //update system state
      system_update();

    }
  }

  std::cout <<"Shutting down thread" << std::endl;
}

void mpi_pv_interface::wait()
{
  _worker_thread->join(); 
}

void mpi_pv_interface::check_wait()
{
  if( _stop_driver.load() ) wait(); 
}

void mpi_pv_interface::stop_driver()
{ 
  _stop_driver.store(true);
}

std::shared_ptr<std::mutex>& mpi_pv_interface::get_shared_mutex()
{
  return _mu; 
}

std::unique_lock<std::mutex>& mpi_pv_interface::get_lock()
{
  return _lk;  
}

bool mpi_pv_interface::is_async()
{
  return _async_enabled;
}

void mpi_pv_interface::enable_async()
{
  _async_enabled = true;
}

void mpi_pv_interface::own_thread( std::thread&& thread )
{
  _worker_thread = std::move(thread);
}

void mpi_pv_interface::set_current_meta( const metadata& md)
{
  _meta = md; 
}
