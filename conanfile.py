from conan import ConanFile
from conan.tools.cmake import CMakeToolchain, CMakeDeps, cmake_layout


class NetAccess(ConanFile):
    settings = "os", "compiler", "build_type", "arch"

    requires = "qt/6.8.3"

    default_options = {
        "qt/*:shared": True,
        "qt/*:qtdeclarative": True,
        "qt/*:qtshadertools": True,
        "qt/*:widgets": False,
        "qt/*:with_pq": True,
        "qt/*:with_sqlite3": False,
        "qt/*:with_brotli": False,
        "qt/*:with_mysql": False,
        "qt/*:with_odbc": False,
        "qt/*:with_dbus": False,
        "qt/*:with_x11": False,
        "qt/*:with_libalsa": False,
        "qt/*:with_pulseaudio": False,
        "qt/*:with_gstreamer": False,
        "qt/*:with_vulkan": False,
        "qt/*:with_egl": False,
    }

    def configure(self):
        self.conf.define("tools.cmake.cmaketoolchain:generator", "Ninja")

    def layout(self):
        cmake_layout(self)

    def generate(self):
        deps = CMakeDeps(self)
        deps.generate()
        tc = CMakeToolchain(self)
        tc.user_presets_path = False
        tc.generate()
