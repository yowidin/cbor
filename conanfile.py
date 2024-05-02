from conan import ConanFile
from conan.tools.cmake import CMakeToolchain, CMake, cmake_layout
from conan.tools.build import check_min_cppstd


class Recipe(ConanFile):
    name = 'cbor'
    version = '0.0.1'

    description = 'CBOR C++ Library'
    settings = 'os', 'arch', 'compiler', 'build_type'
    options = {
        'with_boost_pfr': [True, False],

        'shared': [True, False],
        'fPIC': [True, False],
    }
    default_options = {
        'with_boost_pfr': True,

        'shared': False,
        'fPIC': True,
    }
    requires = []
    keep_imports = True
    exports_sources = 'CMakeLists.txt', 'src/*', 'include/*', 'cmake/*', 'test/*', 'LICENSE.txt'

    def config_options(self):
        if self.settings.os == "Windows":
            del self.options.fPIC

    def layout(self):
        cmake_layout(self)

    def validate(self):
        check_min_cppstd(self, "20")

    def configure(self):
        if self.options.shared:
            self.options.rm_safe("fPIC")

    def generate(self):
        tc = CMakeToolchain(self)
        tc.variables['CBOR_WITH_BOOST_PFR'] = self.options.with_boost_pfr
        tc.generate()

    def build(self):
        cmake = CMake(self)
        cmake.configure()
        cmake.build()

    def package(self):
        cmake = CMake(self)
        cmake.install()

    def package_info(self):
        self.cpp_info.libs = ["cbor"]
