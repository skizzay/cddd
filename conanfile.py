from conans import ConanFile, CMake, GCC
from conans.util.files import mkdir
from os import getcwd, rename
from os.path import join, exists, dirname, abspath

class Cddd(ConanFile):
    name = "cddd"
    version = "0.1.0"
    url = "https://github.com/skizzay/cddd.git"
    requires = "Boost/1.60.0@lasote/stable", "ranges/3.0.0@ericniebler/testing"
    settings = "os", "compiler", "build_type", "arch"
    generators = "cmake", "ycm"
    options = {"shared": [False, True], "build_tests": [True, False]}
    default_options = "shared=False"
    exports = "CMakeLists.txt", "cqrs/*", "utils/*", "messaging/*", "tests/*"

    def config(self):
        # Debug builds will build the tests by default
        if not hasattr(self.options, "build_tests"):
            self.options["build_tests"] = self.settings.build_type == "Debug"
        self.output.info("We are%s building tests" % ("" if self.options.build_tests else " not"))

    def requirements(self):
        if self.options.build_tests:
            self.requires("FakeIt/2.0.3@skizzay/testing")
            self.options["FakeIt"].framework = "standalone"
            self.requires("kerchow/1.0.1@skizzay/stable")
            self.options["kerchow"].shared = self.options.shared
            self.requires("gtest/1.7.0@lasote/stable")
            self.options["gtest"].shared = self.options.shared

    def build(self):
        cmake = CMake(self.settings)
        self.output.info("cmake %s %s %s" % (self._directory_of_self, cmake.command_line, self._extra_cmake_flags))
        self.run("cmake %s %s %s" % (self._directory_of_self, cmake.command_line, self._extra_cmake_flags))
        self.output.info("cmake --build %s %s" % (getcwd(), cmake.build_config))
        self.run("cmake --build %s %s" % (getcwd(), cmake.build_config))

        if self.options.build_tests:
            self.run("ctest")

    def package(self):
        # Copying headers
        self.copy(pattern="*.h", dst=join("utils"), src=join("utils"))
        self.copy(pattern="*.h", dst=join("messaging"), src=join("messaging"))
        self.copy(pattern="*.h", dst=join("cqrs"), src=join("cqrs"))

        # Copying static libs
        self.copy(pattern="*.a", dst="lib", src=".", keep_path=False)

        # Copying dynamic libs
        self.copy(pattern="*.lib", dst="lib", src=".", keep_path=False)
        self.copy(pattern="*.dll", dst="bin", src=".", keep_path=False)
        self.copy(pattern="*.so*", dst="lib", src=".", keep_path=False)
        self.copy(pattern="*.dylib*", dst="lib", src=".", keep_path=False)      

    def package_info(self):
        self.cpp_info.libs = ["cddd_utils"]
        self.cpp_info.includedirs = [self.conanfile_directory]
        if self.settings.compiler == "gcc":
            self._setup_gcc()

    @property
    def _build_tests_flag(self):
        return "-DBUILD_TESTS=TRUE" if self.options.build_tests else ""

    @property
    def _link_type_flag(self):
        return "-DBUILD_SHARED_LIBS=ON" if self.options.shared else ""

    @property
    def _lto_flag(self):
        return "-DGCC_LINK_TIME_OPTIMIZATION=ON" if self._lto_enabled else ""

    @property
    def _extra_cmake_flags(self):
        return " ".join([x for x in [self._build_tests_flag, self._link_type_flag, self._lto_flag] if x])

    @property
    def _lto_enabled(self):
        return self.settings.compiler == "gcc" and \
                self.settings.build_type == "Release" and \
                not self.options.shared

    @property
    def _directory_of_self(self):
        return dirname(abspath(__file__))

    def _setup_gcc(self):
        self.cpp_info.cppflags.append("-std=c++1z")
        if self._lto_enabled:
            self.cpp_info.cppflags.extend(["-flto=jobserver", "-fno-fat-lto-objects"])
