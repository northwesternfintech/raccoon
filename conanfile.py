from conan import ConanFile


class Recipe(ConanFile):
    settings = "os", "compiler", "build_type", "arch"
    generators = "CMakeToolchain", "CMakeDeps", "VirtualRunEnv"

    def layout(self):
        self.folders.generators = "conan"

    def requirements(self):
        self.requires("fmt/10.0.0")
        self.requires("quill/2.9.2")

        self.requires("libwebsockets/4.3.2")  # Brings in libuv too
        self.requires("glaze/1.2.6")

        self.requires("hiredis/1.1.0")

        self.requires("argparse/2.9")

    def build_requirements(self):
        self.test_requires("catch2/3.3.1")

    def configure(self):
        # This is for libwebsockets but for some reason doesn't work with
        # self.options['libwebsockets']
        self.options["*"].with_libuv = True
