# nalu_event_collector

`nalu_event_collector` is a small C++ library for:

- receiving raw Nalu UDP payloads
- parsing them into packets
- grouping packets into events
- exposing timing and buffer statistics to a host application

The repo is structured as:

- library source under `include/nalu_event_collector/...` and `src/...`
- standalone example app under `apps/examples/collector_demo`

## Build

Build the library:

```bash
./scripts/build.sh
```

Install the library locally:

```bash
./scripts/install.sh --prefix /desired/prefix
```

## CPM / CMake usage

This project is intended to be consumable from another CMake application with CPM.

Example:

```cmake
include(cmake/CPM.cmake)

CPMAddPackage(
  NAME nalu_event_collector
  GIT_REPOSITORY <your repo url>
  GIT_TAG feature/modularity-cleanup
)

target_link_libraries(your_target PRIVATE nalu_event_collector::nalu_event_collector)
```

Installed-package usage also works:

```cmake
find_package(nalu_event_collector REQUIRED)
target_link_libraries(your_target PRIVATE nalu_event_collector::nalu_event_collector)
```

## Public headers

The main public surface is:

- `nalu_event_collector/collector/collector.h`
- `nalu_event_collector/config/collector_config.h`
- `nalu_event_collector/logging/logging.h`

Other modules are grouped under:

- `collector/`
- `config/`
- `data/`
- `network/`
- `parsing/`
- `timing/`

## Example app

The example app is standalone and is not part of the top-level library build.

Build it against the local source tree:

```bash
./apps/examples/collector_demo/scripts/build.sh
```

Run it:

```bash
./apps/examples/collector_demo/scripts/run.sh
```

Or through the top-level dispatcher:

```bash
./scripts/run.sh collector_demo
```

To build the app against an installed package instead of the local source tree:

```bash
CMAKE_PREFIX_PATH=/path/to/install ./apps/examples/collector_demo/scripts/build.sh --installed
```

## Notes

- The example app is only a smoke/demo application. It assumes live board traffic and is not part of the library package.
- Logging is handled with `spdlog`.
- No hardware-facing runtime validation is included in this repo; packet/event behavior still depends on upstream board configuration and UDP traffic.
