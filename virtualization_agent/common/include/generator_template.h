#include "pico_utils.h"
#include "base_utils.h"

#ifndef GENTEMPL
#define GENTEMPL

template< typename ... Ts>
class unique_generator
{
  public: 
    using gen_types = std::variant< std::shared_ptr<Ts>... >;

    unique_generator();

    template<typename  T >
    T generate(  );  //transactionID

  private:

    ulong _generate_tid();

    //member variable
    std::atomic_ulong running_id;
};


template<typename ... Ts>
class persistent_generator : unique_generator<Ts...>
{
  public:
   using gen_types = typename unique_generator<Ts...>::gen_types;

   persistent_generator(); 

   template<typename T>
   std::shared_ptr<T>& generate();

   bool try_remove( ulong );

   gen_types& get_entry( ulong, pico_return& );

  private:
    //adding object to _holders 
    template<typename T>
    std::shared_ptr<T>& _add_entry(const T&);

    //members
    std::mutex _alock;
 
    std::map<ulong, gen_types> _holders;
};

template<typename ... Ts>
class pktgen_interface : public persistent_generator<Ts...>
{
  public:
   using pg = persistent_generator<Ts...>;
   using gen_types = typename persistent_generator<Ts...>::gen_types;
   using pg::generate, pg::try_remove, pg::get_entry;

   pktgen_interface() : persistent_generator<Ts...>() {}
   ~pktgen_interface() { std::cout << "Deleting pktgen_interface" << std::endl; }

   std::pair<pico_utils::pico_ctrl, std::string> get_method_from_tid( ulong tid );

   target_sock_type get_source_from_tid( ulong tid );

   template<typename SourceT, typename DestT = SourceT>
   std::shared_ptr<DestT>& opt_generate( std::optional<SourceT>&& source={});

};

#endif
