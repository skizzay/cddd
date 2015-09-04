#pragma once

#include "cqrs/artifact_store.h"
#include "cqrs/command.h"
#include "messaging/dispatcher.h"
#include <stdexcept>

namespace cddd {
namespace cqrs {

struct abandon_command_handling final : std::exception {
	using std::exception::exception;
};


template<class Derived, class CommandType>
class command_handler {
public:
	template<class ErrorPolicy, class DispatchingTablePolicy>
	explicit command_handler(messaging::dispatcher<ErrorPolicy, DispatchingTablePolicy> &dispatcher)
	{
		dispatcher.add_handler(
			[this](const command &cmd) {
				static_cast<Derived *>(this)->handle(static_cast<const CommandType &>(cmd));
			},
			[](const command &cmd) {
				return cmd.type() == utils::type_id_generator::get_id_for_type<CommandType>();
			});
	}
};


template<class ErrorPolicy,
         class DispatchingTablePolicy,
         class ArtifactType,
         class DomainEventSource,
         class ArtifactFactory,
         class ExecutionHandler,
         class ErrorHandler>
inline auto create_command_handler(messaging::dispatcher<ErrorPolicy, DispatchingTablePolicy> &dispatcher,
                                   artifact_store<ArtifactType, DomainEventSource, ArtifactFactory> &artifact_repository,
                                   std::size_t max_retries,
                                   ExecutionHandler execute,
                                   ErrorHandler on_error) noexcept {
   typedef messaging::message_from_argument<ExecutionHandler> CommandType;
   static_assert(std::is_base_of<basic_command<CommandType>, CommandType>::value,
			        "CommandType must be a command (hint:  inherit from basic_command<CommandType>)");

   class command_handler_implementation final : public command_handler<command_handler_implementation, CommandType> {
   public:
      command_handler_implementation(messaging::dispatcher<ErrorPolicy, DispatchingTablePolicy> &dispatcher,
                                     artifact_store<ArtifactType, DomainEventSource, ArtifactFactory> &artifact_repo,
                                     std::size_t max_retries_, ExecutionHandler execute_,
                                     ErrorHandler on_error_) noexcept :
         command_handler<command_handler_implementation, CommandType>{dispatcher},
         max_retries{max_retries_},
         artifact_repository{artifact_repo},
         execute{std::move(execute_)},
         on_error{std::move(on_error_)}
      {
      }

      inline void handle(const CommandType &cmd) noexcept {
         try {
            std::size_t artifact_version = cmd.expected_artifact_version();

            for (std::size_t attempts_remaining = max_retries; 0 != attempts_remaining; --attempts_remaining) {
               try {
                  auto object = artifact_repository.get(cmd.artifact_id(), artifact_version);
                  if (object == nullptr) {
                     break;
                  }
                  execute(cmd, *object);
                  artifact_repository.put(*object);
               }
               catch (const std::exception &e) {
                  artifact_version = on_error(cmd, e);
               }
            }
         }
         catch (const abandon_command_handling &) {
         }
      }

      private:
         std::size_t max_retries;
         artifact_store<ArtifactType, DomainEventSource, ArtifactFactory> &artifact_repository;
         ExecutionHandler execute;
         ErrorHandler on_error;
   };

   return command_handler_implementation{dispatcher, artifact_repository, max_retries, std::move(execute), std::move(on_error)};
}

}
}
