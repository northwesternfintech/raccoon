from conan import ConanFile


class Recipe(ConanFile):
    settings = "os", "compiler", "build_type", "arch"
    generators = "CMakeToolchain", "CMakeDeps", "VirtualRunEnv"

    def layout(self):
        self.folders.generators = "conan"

    def requirements(self):
        self.requires("fmt/10.0.0")  # text formatting
        self.requires("quill/2.9.2")  # logging

        self.requires("libuv/1.45.0")  # event loop
        self.requires("libcurl/8.1.2")  # web requests
        self.requires("glaze/1.2.6")  # json parsing

        self.requires("hiredis/1.1.0")  # redis client

        self.requires("argparse/2.9")  # argument parsing

    def build_requirements(self):
        self.test_requires("catch2/3.3.1")

    def configure(self):
        if self.settings.os == 'Windows':
            # Don't bother building openssl
            self.options['libcurl'].with_ssl = 'schannel'
