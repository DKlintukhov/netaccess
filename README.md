# netaccess

Client-server application for managing access to network resources.

- **Client**: Qt Quick (QML) desktop application
- **Server**: Qt Network TCP server backed by PostgreSQL
- **Stack**: C++20, Qt 6.8, PostgreSQL

## Requirements

- CMake >= 3.31
- Conan 2.x
- A C++20 compiler (MSVC on Windows, GCC/Clang on Linux)
- Git

## PostgreSQL

PostgreSQL is part of the stack — the server uses it for storage. Building needs only the libpq client library, which Conan pulls automatically (Qt QPSQL driver). To run the application, install a PostgreSQL server (native or Docker) and create a database and user for it:

```console
psql -U postgres -c "CREATE USER netaccess WITH PASSWORD 'netaccess';"
psql -U postgres -c "CREATE DATABASE netaccess OWNER netaccess;"
```

Qt's QPSQL driver supports PostgreSQL 7.3+ — any recent release (e.g. 17) works.

## Build

`conan install` builds one configuration per run — the one from the active profile (`Release` by default). Each configuration needs its own build folder: Qt is compiled per configuration and the toolchain pins the MSVC runtime (`/MD` for Release, `/MDd` for Debug); mixing them causes CRT conflicts. `cmake_layout()` puts each install into `build/<build_type>/generators/`.

The project uses **explicit CMake commands, no presets**. The generator is **Ninja** (set in `conanfile.py`; overridable with `-c tools.cmake.cmaketoolchain:generator=...`).

### Choosing a compiler (toolchains)

The toolchain is defined entirely by the Conan profile. `conan profile detect` creates the `default` profile (MSVC on Windows, GCC on Linux); pass another to build with a different compiler — examples in `profiles/`:

```console
conan install . --build=missing -pr=profiles/windows-msvc      # MSVC (default on Windows)
conan install . --build=missing -pr=profiles/windows-mingw     # MinGW-w64 (GCC on PATH)
conan install . --build=missing -pr=profiles/windows-clang-cl  # LLVM clang-cl
```

- **MSVC** — install the **"Desktop development with C++"** workload. `cl.exe` is activated as described in the Windows note below.
- **MinGW-w64** — install [WinLibs](https://winlibs.com/) or MSYS2 (`ucrt64`), add its `bin` to `PATH` (e.g. `C:\msys64\ucrt64\bin`); adjust `compiler.version` in `profiles/windows-mingw`.
- **clang-cl** — add the **"C++ Clang tools for Windows"** VS component; adjust `compiler.version` in `profiles/windows-clang-cl`.

### Release

```console
# 1. Detect the Conan profile (once) — Qt requires C++17+, so pin cppstd
conan profile detect
conan profile update settings.compiler.cppstd=20 default

# 2. Install dependencies (Qt is built from source on the first run)
conan install . --build=missing

# 3. Configure, build, test
cmake -S . -B build/Release -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_TOOLCHAIN_FILE=build/Release/generators/conan_toolchain.cmake
cmake --build build/Release
ctest --test-dir build/Release --output-on-failure
```

> On **Windows** Ninja needs `cl.exe` on `PATH` — and it must be the **x64** environment (x86 cannot link against the x64 Qt binaries: `LNK4272`, unresolved `Qt6*` symbols). `conan install` does not need `cl`, but every `cmake`/`ninja` invocation does. Easiest: open the **x64 Native Tools Command Prompt for VS 2026** and run the commands there. From PowerShell, chain activation and build in one `cmd` call:
>
> ```powershell
> cmd /c "call build\Release\generators\conanvcvars.bat && cmake -S . -B build/Release -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_TOOLCHAIN_FILE=build/Release/generators/conan_toolchain.cmake && cmake --build build/Release"
> ```
>
> Configuring without `cl` on `PATH` leaves the cache broken — delete the build folder and re-run `conan install` before configuring again.

### Debug

Same pattern with its own install and folder:

```console
conan install . -s build_type=Debug --build=missing
cmake -S . -B build/Debug -G Ninja -DCMAKE_BUILD_TYPE=Debug -DCMAKE_TOOLCHAIN_FILE=build/Debug/generators/conan_toolchain.cmake
cmake --build build/Debug
ctest --test-dir build/Debug --output-on-failure
```

### POSIX (Linux / macOS / FreeBSD) — system Qt, no Conan

```console
sudo apt install ninja-build qt6-base-dev libpq-dev   # example (Debian/Ubuntu)
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build
ctest --test-dir build --output-on-failure
```

Add `-DCMAKE_PREFIX_PATH=<Qt prefix>` if `find_package(Qt6)` does not find the Qt installation.

### Notes

- One `conan install` = one configuration; Qt is rebuilt per configuration and per compiler, so build only what you need. Sanitizers are enabled in Debug builds (ASan on MSVC, ASan+UBSan on GCC/Clang).
- Qt is **ABI-locked to the compiler** — switching the Conan profile recompiles Qt from source (takes a while).
- Layout: executables in `<build>/<config>/bin`, libraries in `<build>/<config>/lib` — e.g. `build/Release/bin/netaccess_server` (`.exe` on Windows), `build/Release/lib/libnetaccess_common.a` / `netaccess_common.lib`.

## Run

```console
./build/Release/bin/netaccess_server.exe
./build/Release/bin/netaccess_client.exe
```

## Test

```console
ctest --test-dir build/Release --output-on-failure
```

Unit tests use the Qt Test framework and are registered with CTest. For the Debug build run `ctest --test-dir build/Debug --output-on-failure` instead.

## Tooling

- **clang-format** — formatting check:

  ```console
  cmake --build build/Release --target clang-format-check
  ```

  The target is a no-op until there are sources to check and `clang-format`
  is installed.

- **clang-tidy** — static analysis during the build (opt-in):

  ```console
  cmake -S . -B build/Release -DNETACCESS_ENABLE_CLANG_TIDY=ON
  ```

- **clangd** — language server; consumes `compile_commands.json`, written by the Ninja generator into the active build directory (`build/Release/`).

- **Sanitizers** — ASan (MSVC) / ASan+UBSan (GCC/Clang) in Debug builds; disable with `-DNETACCESS_ENABLE_SANITIZERS=OFF`.

- **Warnings as errors** — on by default; disable with `-DNETACCESS_WARNINGS_AS_ERRORS=OFF`.

- **Compiler profiles** — `profiles/` holds example Conan profiles (`windows-msvc`, `windows-mingw`,
  `windows-clang-cl`); pass one with `-pr=profiles/<name>` to `conan install` to select the toolchain.

## Project layout

```
CMakeLists.txt          # top-level build script
profiles/               # example Conan compiler profiles (windows-msvc, -mingw, -clang-cl)
cmake/                  # shared CMake modules (warnings, sanitizers, tooling)
conanfile.py            # dependencies (Qt 6.8), Ninja generator, cmake_layout
.github/workflows/      # CI (Windows + Conan, Ubuntu + system Qt)
src/
  common/               # shared protocol/DTO library
  server/               # server application
  client/               # Qt Quick client
tests/                  # Qt Test unit tests
```

## License

[MIT](LICENSE)
