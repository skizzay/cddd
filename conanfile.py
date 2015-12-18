from conans import ConanFile, CMake
from conans.util.files import mkdir
from os import getcwd, rename
from os.path import join, exists, dirname, abspath

class Cddd(ConanFile):
    name = "cddd"
    version = "0.1"
    url = "https://github.com/skizzay/cddd.git"
    settings = "os", "compiler", "build_type", "arch"
    generators = "cmake"
    options = {"static": [True, False], "enable_testing": [True, False]}
    default_options = "static=True"

    def config(self):
        if (self.settings.build_type == "Debug"):
            self.options.enable_testing = True

        if (self.options.enable_testing):
            self.requires("gtest/1.7.0@lasote/stable")

    def build(self):
        cmake = CMake(self.settings)
        link_type = "-DBUILD_SHARED_LIBS=ON" if not self.options.static else ""
        lto_option = "-DGCC_LINK_TIME_OPTIMIZATION=ON" if self.lto_enabled else ""
        enable_testing = "-DBUILD_TESTS=ON" if self.options.enable_testing else ""
        extra_flags = " ".join([x for x in [link_type, lto_option, enable_testing] if x])
        cwd = getcwd()
        build_dir = join(cwd, "build", str(self.settings.build_type))

        if not exists(build_dir):
            mkdir(build_dir)
            print "cd %s && cmake %s %s %s" % (build_dir, cmake.command_line, extra_flags, cwd)
            self.run("cd %s && cmake %s %s %s" % (build_dir, cmake.command_line, extra_flags, cwd))
        print "cmake --build %s %s" % (build_dir, cmake.build_config)
        self.run("cmake --build %s %s" % (build_dir, cmake.build_config))

    def package_info(self):
        self.cpp_info.libs.append("cddd")
        if self.settings.compiler == "Gcc":
            self._setup_gcc()

    @property
    def lto_enabled(self):
        return self.settings.compiler == "gcc" and \
                self.settings.build_type == "Release" and \
                self.options.static

    @property
    def _directory_of_self(self):
        return dirname(abspath(__file__))


    def _setup_gcc(self):
        self.cpp_info.cppflags.append("-Wall")
        self.cpp_info.cppflags.append("-Wextra")
        self.cpp_info.cppflags.append("-Werror")
        if self.settings.compiler.version.major >= 5:
            self.cpp_info.cppflags.append("-std=c++1z")
