// vim: sw=3 ts=3 expandtab cindent
#pragma once

#include "cqrs/artifact_store.h"
#include "cqrs/command_validator.h"
#include "utils/type_id_generator.h"
#include "utils/validation.h"
#include <boost/uuid/uuid.hpp>
#include <cassert>
#include <memory>
#include <system_error>
#include <type_traits>

namespace cddd {
namespace cqrs {
namespace details_ {

template<class ...> using void_t = void;

template<class CommandType, class=void_t<>>
class has_artifact_id final : public std::false_type {
};

template<class CommandType>
class has_artifact_id<CommandType, void_t<decltype(std::declval<const CommandType &>().artifact_id())>> final {
   using f_traits = utils::function_traits<decltype(&CommandType::artifact_id)>;
   using result_type = typename f_traits::result_type;

public:
   static constexpr bool value = f_traits::arity == 0 &&
                                 f_traits::is_const &&
                                 std::is_lvalue_reference<result_type>::value &&
                                 std::is_const<std::remove_reference_t<result_type>>::value &&
                                 noexcept(std::declval<const CommandType &>().artifact_id());
};


template<class CommandType, class ArtifactType, class=void_t<>>
struct command_executes_on_artifact {
   static_assert(std::is_void<CommandType>::value || false,
                 "CommandType must have execute_on member function taking an artifact as its sole argument.");
};


template<class CommandType>
using execute_on_t = typename utils::function_traits<decltype(&CommandType::execute_on)>::template argument<0>::type;

template<class CommandType, class ArtifactType>
struct command_executes_on_artifact<CommandType, ArtifactType, void_t<decltype(std::declval<const CommandType &>().execute_on(std::declval<ArtifactType &>()))>> {
   static constexpr bool value = utils::function_traits<decltype(&CommandType::execute_on)>::arity == 1 &&
                                 std::is_lvalue_reference<execute_on_t<CommandType>>::value &&
                                 !std::is_const<std::remove_reference_t<execute_on_t<CommandType>>>::value &&
                                 noexcept(std::declval<const CommandType &>().execute_on(std::declval<ArtifactType &>()));
};

}

typedef utils::type_id_generator::type_id command_type_id;

template<class Mixin>
class command {
public:
   virtual ~command() noexcept = default;

   inline command_type_id type() const noexcept {
      return utils::type_id_generator::get_id_for_type<Mixin>();
   }

   template<class ArtifactType>
   inline std::error_code execute(command_validator<Mixin, ArtifactType> &validator,
                                  ArtifactType &artifact) const noexcept {
      std::error_code error = validator.validate(static_cast<const Mixin &>(*this), const_cast<const ArtifactType &>(artifact)).get();
      if (!error) {
         try {
            static_cast<const Mixin *>(this)->execute_on(artifact);
         }
         catch (const std::system_error &e) {
            error = e.code();
         }
         catch (const std::invalid_argument &) {
            error = make_error_code(std::errc::invalid_argument);
         }
      }
      return error;
   }
};


template<class CommandType, class ArtifactStore>
class command_handler {
   using get_result_type = decltype(std::declval<ArtifactStore &>().get_latest(std::declval<const boost::uuids::uuid &>()));
   using artifact_type = std::decay_t<decltype(*std::declval<get_result_type>())>;

public:
   static_assert(std::is_base_of<command<CommandType>, CommandType>::value,
                 "CommandType must inherit from cddd::cqrs::command<>");
   static_assert(std::is_final<CommandType>::value,
                 "CommandType must be final.");
   static_assert(details_::has_artifact_id<CommandType>::value,
                 "CommandType must implement 'const id_type &artifact_id() const noexcept'");
#if 0
   static_assert(details_::command_executes_on_artifact<CommandType, artifact_type>::value,
                 "CommandType must implement 'std::error_code execute_on(ArtifactType &) noexcept'");
#endif

   inline command_handler(cddd::utils::not_null<std::shared_ptr<command_validator<CommandType, artifact_type>>> v,
                          cddd::utils::not_null<std::shared_ptr<ArtifactStore>> as) :
      validator{v},
      store{as}
   {
   }

   inline std::error_code handle(const CommandType &command) noexcept {
      const boost::uuids::uuid &id = command.artifact_id();
      if (id.is_nil()) {
         return make_error_code(std::errc::invalid_argument);
      }

      auto artifact = store->get_latest(id);
      std::error_code error = command.execute(*validator, *artifact);
      if (!error) {
         store->put(*artifact);
      }
      return error;
   }

private:
   std::shared_ptr<command_validator<CommandType, artifact_type>> validator;
   std::shared_ptr<ArtifactStore> store;
};

}
}
