// vim: sw=3 ts=3 expandtab cindent
#pragma once

#include <boost/uuid/uuid.hpp>
#include <future>
#include <system_error>
#include <vector>

namespace cddd {
namespace cqrs {

template<class ErrorCodeEnum>
inline std::future<std::error_code> make_future_error_code(ErrorCodeEnum ec) {
   std::promise<std::error_code> p;
   p.set_value(make_error_code(ec));
   return p.get_future();
}


inline std::future<std::error_code> make_future_error_code() {
   std::promise<std::error_code> p;
   p.set_value({});
   return p.get_future();
}


template<class CommandType, class ArtifactType>
class command_validator : public std::enable_shared_from_this<command_validator<CommandType, ArtifactType>> {
public:
   virtual ~command_validator() noexcept = default;

   virtual std::future<std::error_code> validate(const CommandType &command,
                                                 const ArtifactType &artifact) = 0;
};


template<class CommandType, class ArtifactType>
class version_must_match final : public command_validator<CommandType, ArtifactType> {
public:
   virtual std::future<std::error_code> validate(const CommandType &command, const ArtifactType &artifact) final override {
      if (command.required_artifact_version() != artifact.version()) {
         return make_future_error_code(std::errc::result_out_of_range);
      }
   }
};


template<class CommandType, class ArtifactType, class Alloc=std::allocator<std::shared_ptr<command_validator<CommandType, ArtifactType>>>>
class composite_command_validator final : public command_validator<CommandType, ArtifactType> {
public:
   virtual std::future<std::error_code> validate(const CommandType &command,
                                                 const ArtifactType &artifact) final override {
      std::vector<std::future<std::error_code>, Alloc> results{validators.get_allocator()};
      results.reserve(validators.size());
      for_each(begin(validators), end(validators), [&](auto validator) -> void {
            results.emplace_back(validator->validate(command, *artifact));
         });

      return when_all(begin(results), end(results)).then([&results]() -> std::error_code {
            for (auto &&f : results) {
               std::error_code error = f.get();
               if (error) {
                  return error;
               }
            }
            return std::error_code{};
         });
   }

   inline void add_command_validator(std::shared_ptr<command_validator<CommandType, ArtifactType>> validator) {
      if (validator != nullptr) {
         auto e = end(validators);
         auto i = find(begin(validators), e, validator);
         if (i == e) {
            validators.push_back(validator);
         }
      }
   }

   inline void remove_command_validator(std::shared_ptr<command_validator<CommandType, ArtifactType>> validator) {
      auto e = end(validators);
      auto i = find(begin(validators), e, validator);
      if (i != e) {
         validators.erase(i);
      }
   }

private:
   std::vector<std::shared_ptr<command_validator<CommandType, ArtifactType>>, Alloc> validators;
};

}
}
