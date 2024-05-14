from conan import ConanFile

class Config(ConanFile):
    settings = "os", "compiler", "build_type", "arch"
    generators = "CMakeDeps", "CMakeToolchain"

    def requirements(self):
        self.requires("sfml/2.6.1")
        self.requires("protobuf/3.21.12")
        self.requires("grpc/1.54.3")
