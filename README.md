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

## Event grouping

Packets are first assembled into a complete channel group: exactly `windows`
packets with the same channel and trigger timestamp. The group is then added to
an event atomically. This prevents a threshold-boundary match from splitting a
channel's windows across two events.

For normal external-trigger collection, an event is complete only when every
configured channel has exactly `windows` packets. The trigger-time threshold is
still used to associate complete channel groups from different channels; it is
not used to decide whether an individual packet belongs to a partially assembled
event. Incomplete groups and events are discarded after
`event_completion_time_us` with a warning.

For self-trigger collection, the configured channels are an allow-list rather
than a list required in every event. An event's serialized channel mask contains
only the channels actually observed in that event, and every observed channel is
required to contribute exactly `windows` packets. A complete group from one
channel is therefore a valid self-trigger event after the completion timeout.

## Collection strategies

`EventBuilder` delegates packet grouping to the public `CollectionStrategy`
interface. The built-in selector is `event_builder.collection_strategy`:

- `trigger_time` preserves the traditional one-output-event-per-trigger behavior.
- `time_batch` first forms normal trigger events, then combines those events into
  larger fixed-wall-clock output events.

Only output concerns are global. Each strategy owns its remaining configuration:

```json
{
  "collection_strategy": "time_batch",
  "max_events_in_buffer": 10000,
  "event_header": 48059,
  "event_trailer": 61166,
  "collection_strategies": {
    "trigger_time": {
      "channels": [0, 2],
      "trigger_type": "self",
      "windows": 8,
      "time_threshold": 1500,
      "event_completion_time_us": 10000
    },
    "time_batch": {
      "duration_us": 100000,
      "max_packets": 65535,
      "trigger_events": {
        "channels": [0, 2],
        "trigger_type": "self",
        "windows": 8,
        "time_threshold": 1500,
        "event_completion_time_us": 10000
      }
    }
  }
}
```

For `time_batch`, constituent trigger events are appended atomically: capacity
rollover never splits one trigger event between batches. Packet order and each
packet's trigger timestamp are preserved. Empty time intervals do not emit an
event, and an orderly `flush()` publishes a non-empty final partial batch.
Setting `duration_us` to zero is useful for deterministic operation and closes
one batch after each packet-collection call.

Applications can implement a new strategy by deriving from
`CollectionStrategy` and injecting it with
`EventBuilder(config, std::move(strategy))`. The strategy may use the
output-only `EventBuffer(max_events)` constructor and may keep an entirely
application-specific configuration object.

Strategy code is organized by implementation:

```text
include/nalu_event_collector/collector/strategies/
  collection_strategy.h
  collection_strategy_factory.h
  collection_strategies_config.h
  trigger_time/
    trigger_time_collection_strategy.h
    trigger_time_collection_strategy_config.h
  time_batch/
    time_batch_collection_strategy.h
    time_batch_collection_strategy_config.h
src/collector/strategies/
  trigger_time/
  time_batch/
```

The strategy dispatch occurs once per parsed packet vector, not once per
packet. The traditional trigger-time strategy therefore retains its direct
packet loop. Time batching adds one intentional packet copy when a completed
trigger event is appended to its aggregate output event.

## Tests

Build and run the deterministic unit and simulated-stream suites:

```bash
./scripts/build.sh
./build/bin/nalu_event_collector_tests
./build/bin/nalu_event_collector_simulated_stream_tests
```

The simulated suite constructs raw 74-byte board packets and exercises the
incremental parser using fragmented reads. It covers threshold-boundary event
association, a 1,000-trigger stream, fixed readout-cycle batching, and recovery
after a dropped packet.
