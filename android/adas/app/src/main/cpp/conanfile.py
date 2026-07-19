from conan import ConanFile
from conan.tools.cmake import CMakeDeps, CMakeToolchain


class AdasCppConan(ConanFile):
    name = "adas-cpp"
    version = "0.1.0"
    package_type = "application"
    settings = "os", "arch", "compiler", "build_type"
    options = {"tests": [True, False]}
    default_options = {
        "tests": False,
        "zeromq/*:encryption": False,
        # Avoid libudev/system (and match Android); build from recipe if binary download fails
        "libusb/*:enable_udev": False,
    }

    def requirements(self):
        self.requires("protobuf/3.21.12")
        self.requires("cppzmq/4.10.0")
        self.requires("libusb/1.0.26")
        if self.options.tests:
            self.requires("gtest/1.14.0")

    def build_requirements(self):
        # Host protoc for protobuf_generate_cpp when cross-compiling
        self.tool_requires("protobuf/3.21.12")

    def generate(self):
        tc = CMakeToolchain(self)
        tc.variables["BUILD_FOR_ANDROID"] = self.settings.os == "Android"
        tc.variables["BUILD_TESTING"] = bool(self.options.tests)
        tc.generate()
        CMakeDeps(self).generate()
