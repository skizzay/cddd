//
// Created by andrew on 1/28/24.
//

#ifndef MEMORY_BUFFER_H
#define MEMORY_BUFFER_H
#include "seek_origin.h"

// ReSharper disable once CppInconsistentNaming
namespace skizzay::s11n { namespace memory_buffer_details {
        template<typename Impl>
            requires std::is_class_v<Impl> && std::is_same_v<Impl, std::remove_cvref_t<Impl> >
        class sink_base {
        public:
            using offset_type = std::char_traits<char>::off_type;

            [[nodiscard]] offset_type sink_position() const noexcept {
                return position_;
            }

            void sink_seek(offset_type const p, seek_origin const origin) {
                switch (origin) {
                    case seek_origin::beginning: {
                        if (p < 0) {
                            throw std::ios_base::failure{"seek before beginning"};
                        }
                        if (p > impl_capacity()) {
                            throw std::ios_base::failure{"seek past end"};
                        }
                        position_ = p;
                        break;
                    case seek_origin::current:
                        if (position_ + p < 0) {
                            throw std::ios_base::failure{"seek before beginning"};
                        }
                        if (position_ + p > impl_capacity()) {
                            throw std::ios_base::failure{"seek past end"};
                        }
                        position_ += p;
                        break;
                    case seek_origin::end:
                        if
                        position_ = size_ + p;
                        break;
                    }
                }
            }

        private:
            constexpr std::size_t impl_capacity() const noexcept {
                return static_cast<Impl const &>(*this).sink_capacity();
            }
            offset_type position_{};
        };
    }

    class memory_buffer_sink final : memory_buffer_details::sink_base<memory_buffer_sink> {
    };
} // skizzay

#endif //MEMORY_BUFFER_H
