//
// Created by andrew on 1/28/24.
//

#include <skizzay/s11n/buffer_reader.h>
#include <catch2/catch_all.hpp>

#include "skizzay/s11n/encode.h"
#include "skizzay/s11n/file_deser.h"
#include "skizzay/s11n/sink.h"

using namespace skizzay::s11n;

TEST_CASE("buffer reader can convert to integrals", "[buffer_reader]") {
    // TODO: Don't depend on files for unit tests.  Create a memory-based source.
    file f = file::temporary();

    sink_write(f, encode_native(std::uint32_t{0x12345678}));
    source_seek(f, 0, seek_origin::beginning);

    buffer_reader<std::endian::native, file> reader{std::move(f)};
    REQUIRE(static_cast<std::uint32_t>(reader.next()) == 0x12345678);
}