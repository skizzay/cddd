#include <gtest/gtest.h>
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-local-typedefs"
#pragma GCC diagnostic ignored "-Wignored-qualifiers"
#include <cucumber-cpp/internal/CukeEngineImpl.hpp>
#include <cucumber-cpp/internal/connectors/wire/WireServer.hpp>
#include <cucumber-cpp/internal/connectors/wire/WireProtocol.hpp>
#pragma GCC diagnostic pop
#include <climits>
#include <cstdlib>
#include <iostream>
#include <libgen.h>
#include <sstream>
#include <thread>

namespace {

void run_cucumber_tests(int port) {
   using namespace ::cucumber::internal;
   CukeEngineImpl cukeEngine;
   JsonSpiritWireMessageCodec wireCodec;
   WireProtocolHandler protocolHandler{&wireCodec, &cukeEngine};
   // We'll leak this.  There's a bug in the destructor.
   SocketServer *server = new SocketServer(&protocolHandler);
   server->listen(port);
   server->acceptOnce();
}


std::string build_cucumber_command(int argc, char **argv, const std::string &features_directory) {
   std::ostringstream command;
   command << "cucumber";
   char **end = argv + argc;

   for (char **arg=argv; arg != end; ++arg) {
      command << ' ' << *arg;
   }

   command << " -s " << features_directory;

   return command.str();
}


std::string get_features_directory(const char *executable) {
   char real_value[PATH_MAX];
   realpath(executable, real_value);
   std::string directory = ::dirname(real_value);
   directory += "/../tests/features";
   realpath(directory.c_str(), real_value);
   return real_value;
}

}


int main(int argc, char **argv) {
   int server_result = 0;
   int client_result = 0;
   std::thread server([&server_result]() {
         try {
            run_cucumber_tests(3902);
         }
         catch(const std::exception &e) {
            std::cerr << e.what() << std::endl;
            server_result = 1;
         }
      });
   std::thread client([argc, argv, &client_result]() {
         std::string features_directory = get_features_directory(argv[0]);
         std::string command{build_cucumber_command(argc - 1, argv + 1, features_directory)};
         client_result = std::system(command.c_str());
      });

   client.join();
   server.join();

   return server_result == 0 ? client_result : server_result;
}
