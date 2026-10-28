# Pothos Blocks toolkit

General purpose processing blocks for the Pothos framework: event, file,
network, packet, serialize, stream and testers modules.

The topology of the Pothos core inserts `/blocks/copier` (a domain
crossing) and `/blocks/network_sink` / `/blocks/network_source` (a flow
between processes). Thus most installations of the core need this toolkit.

This branch (`pothos2`) builds against the modernized core
(https://github.com/markvin/PothosCore, branch `pothos2`). The modules use
only the public API of the core.

## Dependencies

- an installed Pothos core (`find_package(Pothos)`);
- nlohmann_json 3.11 or higher, standalone Asio 1.38 or higher,
  xsimd 14.2 or higher (all three are headers only).

CMake finds the dependencies with `find_package`. They can come from the
system packages or from Conan 2 (`conanfile.txt`).

## Build

With system packages (for example Arch Linux: `asio nlohmann-json xsimd`):

```sh
cmake -S . -B build -G Ninja -DCMAKE_PREFIX_PATH=<core prefix>
cmake --build build
ctest --test-dir build --output-on-failure
```

With Conan 2:

```sh
conan install . --output-folder=build/conan -s build_type=Release -s compiler.cppstd=23 --build=missing
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_TOOLCHAIN_FILE=build/conan/conan_toolchain.cmake \
    -DCMAKE_PREFIX_PATH=<core prefix>
cmake --build build
ctest --test-dir build --output-on-failure
```

`<core prefix>` is the installation prefix of the Pothos core. Do not set
it when the core is installed in a default location of the system.

## Tests

Each module lists its self-tests (`TESTS` of `pothos_blocks_module()`).
The test `blocks.stage` installs the modules into `build/stage`. Then
`blocks.selftests` runs each self-test with the PothosUtil of the core and
`POTHOS_PLUGIN_PATH` set to the staged modules.

## Licensing information

Use, modification and distribution is subject to the Boost Software
License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
http://www.boost.org/LICENSE_1_0.txt). The SIMD kernels in `stream/SIMD`
are BSD-3-Clause, as marked in their headers.
