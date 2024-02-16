//
// Created by andrew on 2/13/24.
//

#pragma once
#include <exception>
#include <tuple>
#include "io_device.h"
#include "seek_origin.h"

namespace skizzay::s11n {
    template<random_access_sink... Sinks>
    class write_transaction {
    public:
        explicit write_transaction(Sinks &... sinks) noexcept
            : devices_{std::tuple{&sinks, write_position(sinks)}...} {
        }

        ~write_transaction() {
            if (!finalized_) {
                if (num_exceptions_ < std::uncaught_exceptions()) {
                    rollback();
                }
                else {
                    commit();
                }
            }
        }

        void commit() {
            std::apply([](auto &... pairs) {
                constexpr auto flush_sink = [](random_access_sink auto *device, std::signed_integral auto const) {
                    s11n::flush(*device);
                };
                (std::apply(flush_sink, pairs), ...);
            }, devices_);
            finalized_ = true;
        }

        void rollback() {
            std::apply([](auto &... pairs) {
                constexpr auto rollback_sink = [](random_access_sink auto *device,
                                                  std::signed_integral auto const &position
                ) {
                    seek_write(*device, position, seek_origin::beginning);
                };
                (std::apply(rollback_sink, pairs), ...);
            }, devices_);
            finalized_ = true;
        }

    private:
        std::tuple<std::tuple<Sinks *, std::invoke_result_t<decltype(write_position), Sinks const &> >...> devices_;
        int const num_exceptions_{std::uncaught_exceptions()};
        bool finalized_{false};
    };
} // skizzay
