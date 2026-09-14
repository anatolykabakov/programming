from conan import ConanFile
from conan.tools.cmake import CMakeDeps, CMakeToolchain, cmake_layout


class NotebookAppConan(ConanFile):
    name = "notebook_app"
    version = "0.1.0"
    description = "Notebook App"
    topics = ("notebook", "app")
    settings = "os", "compiler", "build_type", "arch"
    package_type = "application"
    languages = "C++"

    options = {
        "build_testing": [True, False],
    }
    default_options = {
        "build_testing": False,
    }

    def requirements(self):
        self.requires("cpp-httplib/0.39.0")
        self.requires("jsoncpp/1.9.6")
        self.requires("spdlog/1.9.2")
        self.requires("tclap/1.2.5")
        if self.options.build_testing:
            self.test_requires("gtest/1.10.0")

    def build_requirements(self):
        if self.options.build_testing:
            self.tool_requires("gtest/1.10.0")

    def layout(self):
        cmake_layout(self)

    def generate(self):
        tc = CMakeToolchain(self)
        tc.cache_variables["BUILD_TESTS"] = self.options.build_testing
        tc.generate()

        deps = CMakeDeps(self)
        deps.generate()
