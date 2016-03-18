#pragma once

#include "utils/type_id_generator.h"
#include "utils/validation.h"
#include <boost/uuid/uuid.hpp>
#include <type_traits>


namespace cddd {
namespace cqrs {

typedef utils::type_id_generator::type_id command_type_id;

class command {
public:
   virtual ~command() noexcept = default;

   virtual command_type_id type() const noexcept = 0;
   virtual const boost::uuids::uuid & artifact_id() const noexcept = 0;
   virtual std::size_t expected_artifact_version() const noexcept = 0;
};


template<class Mixin>
class basic_command : public command {
public:
   static_assert(std::is_base_of<basic_command<Mixin>, Mixin>::value,
                 "basic_command must be a mixin type.");

   virtual command_type_id type() const noexcept final override {
      return utils::type_id_generator::get_id_for_type<Mixin>();
   }

   virtual const boost::uuids::uuid & artifact_id() const noexcept final override {
      return id;
   }

   virtual std::size_t expected_artifact_version() const noexcept final override {
      return artifact_version;
   }

protected:
   basic_command(utils::valid_uuid<const boost::uuids::uuid &> id_, std::size_t version) noexcept :
      command{},
      id(id_),
      artifact_version{version}
   {
   }

private:
   boost::uuids::uuid id;
   std::size_t artifact_version;
};

}
}
