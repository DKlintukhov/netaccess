# netaccess

> **Русский** | [English](#english)

Клиент-серверное приложение для управления доступом к сетевым ресурсам (ABAC).

- **Клиент**: настольное приложение Qt Quick (QML)
- **Сервер**: TCP-сервер на Qt Network, хранилище — PostgreSQL
- **Стек**: C++20, Qt 6.8, PostgreSQL

## Требования

- CMake >= 3.31
- Conan 2.x
- Компилятор C++20 (MSVC на Windows, GCC/Clang на Linux/macOS)
- Git

## Установка зависимостей

Сборка возможна двумя способами:

1. **Через Conan** — Qt и libpq собираются/подтягиваются автоматически (рекомендуется для Windows и macOS).
2. **С системным Qt** — используются пакеты из дистрибутива (быстрее, не нужен Conan; подходит для Linux).

### Debian / Ubuntu

```console
sudo apt update
sudo apt install cmake ninja-build g++ \
                 qt6-base-dev qt6-declarative-dev \
                 libqt6sql6-psql libpq-dev qt6-l10n-tools
```

- `qt6-base-dev` — модули Qt6 Core/Gui/Network/Sql (заголовки и библиотеки).
- `qt6-declarative-dev` — модули Qt6 Qml/Quick для клиента.
- `libqt6sql6-psql` — драйвер PostgreSQL (QPSQL) для модуля Qt SQL (нужен при запуске).
- `libpq-dev` — клиентская библиотека и заголовки PostgreSQL.
- `qt6-l10n-tools` — инструменты переводов `lupdate`/`lrelease` (для локализации RU/EN).
- `ninja-build` + `cmake` — генератор Ninja и CMake.

### macOS (Homebrew)

```console
brew install cmake ninja qt@6 libpq qt-postgresql
```

- `qt@6` — ставится в keg-only-префикс (`$(brew --prefix qt@6)`), поэтому его нужно
  явно передать CMake через `-DCMAKE_PREFIX_PATH`, см. ниже.
- `libpq` — клиентская библиотека PostgreSQL.
- `qt-postgresql` — драйвер QPSQL для модуля Qt SQL (формула Homebrew, без неё
  драйвер в системном Qt отсутствует).

Если нужен сам сервер PostgreSQL — `brew install postgresql@17`.

### PostgreSQL

PostgreSQL — часть стека: сервер использует его как хранилище. Для сборки достаточно
клиентской библиотеки libpq (Conan подтягивает её автоматически — Qt QPSQL-драйвер).
Для запуска приложения установите сервер PostgreSQL (нативно или в Docker) и создайте
базу и пользователя:

```console
psql -U postgres -c "CREATE USER netaccess WITH PASSWORD 'netaccess';"
psql -U postgres -c "CREATE DATABASE netaccess OWNER netaccess;"
```

Драйвер QPSQL поддерживает PostgreSQL 7.3+ — подойдёт любая актуальная версия (например, 17).

## Сборка

`conan install` собирает одну конфигурацию за раз — ту, что задана активным профилем
(`Release` по умолчанию). Для каждой конфигурации нужна отдельная папка сборки: Qt
компилируется для каждой конфигурации, а тулчейн привязывает рантайм MSVC (`/MD` для
Release, `/MDd` для Debug); их смешение вызывает конфликты CRT. `cmake_layout()` кладёт
каждую установку в `build/<build_type>/generators/`.

Проект использует **явные команды CMake, без пресетов**. Генератор — **Ninja**
(задан в `conanfile.py`; можно переопределить через `-c tools.cmake.cmaketoolchain:generator=...`).

### Выбор компилятора (тулчейны)

Тулчейн полностью определяется профилем Conan. `conan profile detect` создаёт профиль
`default` (MSVC на Windows, GCC на Linux); чтобы собрать другим компилятором, передайте
другой профиль — примеры в `profiles/`:

```console
conan install . --build=missing -pr=profiles/windows-msvc      # MSVC (по умолчанию на Windows)
conan install . --build=missing -pr=profiles/windows-mingw     # MinGW-w64 (GCC в PATH)
conan install . --build=missing -pr=profiles/windows-clang-cl  # LLVM clang-cl
```

- **MSVC** — установите нагрузку **«Разработка классических приложений на C++»**. `cl.exe`
  активируется так, как описано в примечании про Windows ниже.
- **MinGW-w64** — установите [WinLibs](https://winlibs.com/) или MSYS2 (`ucrt64`), добавьте
  его `bin` в `PATH` (например, `C:\msys64\ucrt64\bin`); поправьте `compiler.version` в
  `profiles/windows-mingw`.
- **clang-cl** — добавьте компонент VS **«C++ Clang tools for Windows»**; поправьте
  `compiler.version` в `profiles/windows-clang-cl`.

### Release

```console
# 1. Определяем профиль Conan (один раз)
conan profile detect

# 2. Устанавливаем зависимости (Qt в первый раз собирается из исходников).
#    profile detect ставит cppstd=14 на MSVC, а Qt требует C++17+ — зафиксируем его.
conan install . --build=missing -s compiler.cppstd=20

# 3. Конфигурируем, собираем, тестируем
cmake -S . -B build/Release -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_TOOLCHAIN_FILE=build/Release/generators/conan_toolchain.cmake
cmake --build build/Release
ctest --test-dir build/Release --output-on-failure
```

> На **Windows** Ninja требует `cl.exe` в `PATH` — и это обязательно окружение **x64**
> (x86 не сможет слинковаться с x64-бинарниками Qt: `LNK4272`, неразрешённые символы `Qt6*`).
> Для `conan install` `cl` не нужен, но каждая команда `cmake`/`ninja` — нуждается. Проще всего:
> откройте **x64 Native Tools Command Prompt for VS 2026** и выполняйте команды там. Из PowerShell
> можно совместить активацию и сборку в одном вызове `cmd`:
>
> ```powershell
> cmd /c "call build\Release\generators\conanvcvars.bat && cmake -S . -B build/Release -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_TOOLCHAIN_FILE=build/Release/generators/conan_toolchain.cmake && cmake --build build/Release"
> ```
>
> Если конфигурировать без `cl` в `PATH`, кэш останется битым — удалите папку сборки и
> повторите `conan install` перед повторным конфигурированием.

### Debug

Та же схема, со своей установкой и папкой:

```console
conan install . -s build_type=Debug -s compiler.cppstd=20 --build=missing
cmake -S . -B build/Debug -G Ninja -DCMAKE_BUILD_TYPE=Debug -DCMAKE_TOOLCHAIN_FILE=build/Debug/generators/conan_toolchain.cmake
cmake --build build/Debug
ctest --test-dir build/Debug --output-on-failure
```

### POSIX (Linux / macOS / FreeBSD) — системный Qt, без Conan

Сначала установите зависимости (см. раздел «Установка зависимостей» выше), затем:

```console
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build
ctest --test-dir build --output-on-failure
```

Добавьте `-DCMAKE_PREFIX_PATH=<префикс Qt>`, если `find_package(Qt6)` не находит установку Qt:

- **Debian/Ubuntu** — путь указывать обычно не нужно (Qt в системных путях).
- **macOS (Homebrew)** — обязательно:

  ```console
  cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_PREFIX_PATH="$(brew --prefix qt@6)"
  ```

  При необходимости укажите путь к libpq драйверу: `-DPostgreSQL_ROOT="$(brew --prefix libpq)"`.

### Примечания

- Один `conan install` = одна конфигурация; Qt пересобирается для каждой конфигурации и
  каждого компилятора, поэтому собирайте только то, что нужно. Санитайзеры включены в
  Debug-сборках (ASan на MSVC, ASan+UBSan на GCC/Clang).
- Qt **жёстко привязан к компилятору по ABI** — смена профиля Conan пересобирает Qt из
  исходников (займёт время).
- Раскладка: исполняемые файлы в `<build>/<config>/bin`, библиотеки в `<build>/<config>/lib` —
  например, `build/Release/bin/netaccess_server` (`.exe` на Windows),
  `build/Release/lib/libnetaccess_common.a` / `netaccess_common.lib`.

## Запуск

```console
./build/Release/bin/netaccess_server.exe
./build/Release/bin/netaccess_client.exe
```

## Тестирование

```console
ctest --test-dir build/Release --output-on-failure
```

Модульные тесты используют Qt Test framework и регистрируются в CTest. Для Debug-сборки
выполняйте `ctest --test-dir build/Debug --output-on-failure`.

## Инструменты

- **clang-format** — проверка форматирования:

  ```console
  cmake --build build/Release --target clang-format-check
  ```

  Цель неактивна, пока нет исходников для проверки и не установлен `clang-format`.

- **clang-tidy** — статический анализ во время сборки (по желанию):

  ```console
  cmake -S . -B build/Release -DNETACCESS_ENABLE_CLANG_TIDY=ON
  ```

- **clangd** — language server; использует `compile_commands.json`, который генератор Ninja
  пишет в активную папку сборки (`build/Release/`).

- **Санитайзеры** — ASan (MSVC) / ASan+UBSan (GCC/Clang) в Debug-сборках; отключение:
  `-DNETACCESS_ENABLE_SANITIZERS=OFF`.

- **Предупреждения как ошибки** — включено по умолчанию; отключение:
  `-DNETACCESS_WARNINGS_AS_ERRORS=OFF`.

- **Профили компилятора** — в `profiles/` примеры профилей Conan (`windows-msvc`,
  `windows-mingw`, `windows-clang-cl`); передавайте один через `-pr=profiles/<name>`
  в `conan install`, чтобы выбрать тулчейн.

## Структура проекта

```
CMakeLists.txt          # верхнеуровневый скрипт сборки
profiles/               # примеры профилей Conan (windows-msvc, -mingw, -clang-cl)
cmake/                  # общие модули CMake (предупреждения, санитайзеры, инструменты)
conanfile.py            # зависимости (Qt 6.8), генератор Ninja, cmake_layout
.github/workflows/      # CI (Windows + Conan, Ubuntu + системный Qt)
src/
  common/               # общая библиотека: протокол/DTO
  server/               # серверное приложение
  client/               # клиент Qt Quick
tests/                  # модульные тесты Qt Test
```

## Лицензия

[MIT](LICENSE)

---

# English

# netaccess

Client-server application for managing access to network resources (ABAC).

- **Client**: Qt Quick (QML) desktop application
- **Server**: Qt Network TCP server backed by PostgreSQL
- **Stack**: C++20, Qt 6.8, PostgreSQL

## Requirements

- CMake >= 3.31
- Conan 2.x
- A C++20 compiler (MSVC on Windows, GCC/Clang on Linux/macOS)
- Git

## Installing dependencies

There are two ways to build:

1. **Via Conan** — Qt and libpq are pulled/built automatically (recommended for Windows and macOS).
2. **With system Qt** — packages from the OS distribution (faster, no Conan needed; fits Linux).

### Debian / Ubuntu

```console
sudo apt update
sudo apt install cmake ninja-build g++ \
                 qt6-base-dev qt6-declarative-dev \
                 libqt6sql6-psql libpq-dev qt6-l10n-tools
```

- `qt6-base-dev` — Qt6 Core/Gui/Network/Sql modules (headers and libraries).
- `qt6-declarative-dev` — Qt6 Qml/Quick modules for the client.
- `libqt6sql6-psql` — PostgreSQL driver (QPSQL) for the Qt SQL module (needed at runtime).
- `libpq-dev` — PostgreSQL client library and headers.
- `qt6-l10n-tools` — translation tools `lupdate`/`lrelease` (for RU/EN localization).
- `ninja-build` + `cmake` — Ninja generator and CMake.

### macOS (Homebrew)

```console
brew install cmake ninja qt@6 libpq qt-postgresql
```

- `qt@6` — installed into a keg-only prefix (`$(brew --prefix qt@6)`), so pass it to CMake
  explicitly via `-DCMAKE_PREFIX_PATH` (see below).
- `libpq` — PostgreSQL client library.
- `qt-postgresql` — QPSQL driver for the Qt SQL module (a Homebrew formula; without it the
  driver is absent from system Qt).

For a local PostgreSQL server — `brew install postgresql@17`.

### PostgreSQL

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
# 1. Detect the Conan profile (once)
conan profile detect

# 2. Install dependencies (Qt is built from source on the first run).
#    profile detect sets cppstd=14 on MSVC, but Qt requires C++17+ — pin it.
conan install . --build=missing -s compiler.cppstd=20

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
conan install . -s build_type=Debug -s compiler.cppstd=20 --build=missing
cmake -S . -B build/Debug -G Ninja -DCMAKE_BUILD_TYPE=Debug -DCMAKE_TOOLCHAIN_FILE=build/Debug/generators/conan_toolchain.cmake
cmake --build build/Debug
ctest --test-dir build/Debug --output-on-failure
```

### POSIX (Linux / macOS / FreeBSD) — system Qt, no Conan

First install the dependencies (see "Installing dependencies" above), then:

```console
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build
ctest --test-dir build --output-on-failure
```

Add `-DCMAKE_PREFIX_PATH=<Qt prefix>` if `find_package(Qt6)` does not find the Qt installation:

- **Debian/Ubuntu** — usually not needed (Qt is in system paths).
- **macOS (Homebrew)** — required:

  ```console
  cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_PREFIX_PATH="$(brew --prefix qt@6)"
  ```

  If needed, point at libpq: `-DPostgreSQL_ROOT="$(brew --prefix libpq)"`.

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
