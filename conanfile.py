from conan import ConanFile


class Recipe(ConanFile):
    settings = "os", "compiler", "build_type", "arch"
    generators = "CMakeToolchain", "CMakeDeps", "VirtualRunEnv"

    def layout(self):
        self.folders.generators = "conan"

    def requirements(self):
        self.requires("fmt/[^10.1.0]")  # text formatting
        self.requires("quill/2.9.2")  # logging

        self.requires("libuv/1.45.0")  # event loop
        self.requires("glaze/1.2.6")  # json parsing

        self.requires("hiredis/1.1.0")  # redis client

        self.requires("argparse/2.9")  # argument parsing

        # ZLib is a curl dep
        self.requires("zlib/1.2.13")

    def build_requirements(self):
        self.test_requires("gtest/1.13.0")

    def configure(self):
        pass
