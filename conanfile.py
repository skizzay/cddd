from pathlib import Path
from conans import ConanFile


def _CdddConan__read_contents_of_file(filename):
    license = Path(__file__).parent.joinpath(filename)
    with license.open() as fd:
        return fd.read()

class Cddd(ConanFile):
    name = "cddd"
    version = "0.1.0"
    url = "https://github.com/skizzay/cddd.git"
    settings = "os", "compiler", "build_type", "arch"
    generators = "cmake", "ycm"
    options = {"shared": [False, True], "build_tests": [True, False]}
    default_options = "shared=False", "build_tests=True"
    exports = "CMakeLists.txt", "cqrs/*", "utils/*", "messaging/*", "tests/*"


    def package(self):
        self.copy("*.h", dst="include", src="src", keep_path=True)
