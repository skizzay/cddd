from conans import ConanFile, CMake
from os.path import join

class CdddPackageTest(ConanFile):
    settings = "os", "arch", "compiler", "build_type"
    requires = "cddd/0.1.0@skizzay/testing"
    generators = "cmake"
    options = {"shared": [False, True]}
    default_options = "shared=False", "cddd:build_tests=True"

    def config(self):
        self.options["cddd"].shared = self.options.shared

    def imports(self):
        self.copy("cddd_tests", src="bin", dst="bin")

    def build(self):
        cmake = CMake(self.settings)
        self.run("cmake %s %s" % (self.conanfile_directory, cmake.command_line))
        self.run("cmake --build %s %s" % (self.conanfile_directory, cmake.build_config))

    def test(self):
        self.run(join(".", "bin", "find_cddd_tests"))
