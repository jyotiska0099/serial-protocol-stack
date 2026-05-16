# Serial Protocol Stack

[![Build and Test](https://github.com/jyotiska0099/serial-protocol-stack/actions/workflows/ci.yml/badge.svg)](https://github.com/YOUR_USERNAME/serial-protocol-stack/actions/workflows/ci.yml)

A layered serial communication protocol stack built from scratch in C++17. Designed to teach real embedded protocol architecture — no external protocol libraries, every layer implemented from first principles.

---

## Table of Contents

- [Architecture](#architecture)
- [Wire Format](#wire-format)
- [Project Structure](#project-structure)
- [Building](#building)
- [Running Tests](#running-tests)
- [Layer Guide](#layer-guide)
- [Swapping the Physical Layer](#swapping-the-physical-layer)
- [Design Decisions](#design-decisions)

---

## Architecture

The stack is composed of five independent layers. Each layer talks only to the interface directly below it — nothing reaches across layers.

```
┌─────────────────────────────────────────────┐
│           Application Layer                 │  Command Pattern dispatcher
│   msg_id → CommandHandler registry          │  routes frames to handlers
├─────────────────────────────────────────────┤
│             Session Layer                   │  ARQ state machine
│   ACK / NACK · retransmit · seq numbers     │  IDLE → SENDING → WAITING_FOR_ACK
├─────────────────────────────────────────────┤
│              CRC Layer                      │  CRC-16/CCITT integrity check
│   [ payload | CRC_HI | CRC_LO ]            │  table-driven, built from scratch
├─────────────────────────────────────────────┤
│             Framing Layer                   │  PPP/HDLC byte stuffing
│   [ START | LEN | stuffed payload | END ]  │  escape sequences for 0x7E/7F/7D
├─────────────────────────────────────────────┤
│        IPhysicalLayer  (interface)          │  pure-virtual — swappable at will
│        SocketPhysicalLayer (test impl)      │  POSIX socketpair, no hardware needed
└─────────────────────────────────────────────┘
```

---

## Wire Format

Reading bottom-up, each layer wraps the one above it:

```
0x7E | LEN | SEQ | TYPE | MSG_ID | APP_PAYLOAD | CRC_HI | CRC_LO | 0x7F
 ↑      ↑     ↑     ↑       ↑           ↑            ↑                ↑
 │      │     └─────┴───────┴───────────┴────────────┘                │
 │      │               Session + Application fields                   │
 │      └── Framing: length of stuffed content                        │
 └── Framing: START byte                                  END byte ───┘
```

| Field | Size | Added by |
|---|---|---|
| `START` / `END` | 1 byte each | Framing Layer |
| `LEN` | 1 byte | Framing Layer |
| `SEQ` | 1 byte | Session Layer |
| `TYPE` | 1 byte | Session Layer (`DATA` / `ACK` / `NACK`) |
| `MSG_ID` | 1 byte | Application Layer |
| `APP_PAYLOAD` | 0–128 bytes | Application Layer |
| `CRC_HI` / `CRC_LO` | 2 bytes | CRC Layer |

---

## Project Structure

```
.
├── src/
│   ├── physical/
│   │   ├── IPhysicalLayer.h          ← pure-virtual interface (only thing upper layers see)
│   │   ├── SocketPhysicalLayer.h
│   │   └── SocketPhysicalLayer.cpp   ← POSIX socketpair implementation
│   ├── framing/
│   │   ├── FramingLayer.h
│   │   └── FramingLayer.cpp          ← START/END byte framing + PPP escape sequences
│   ├── crc/
│   │   ├── CrcLayer.h
│   │   └── CrcLayer.cpp              ← CRC-16/CCITT, table-driven from scratch
│   ├── session/
│   │   ├── SessionLayer.h
│   │   └── SessionLayer.cpp          ← ARQ state machine: ACK/NACK/retransmit
│   └── application/
│       ├── ApplicationLayer.h
│       └── ApplicationLayer.cpp      ← Command Pattern dispatcher
├── tests/
│   ├── PhysicalLayerTest.cpp         ← socket I/O, timeout, large payloads
│   ├── FramingLayerTest.cpp          ← framing, byte stuffing, error cases
│   ├── CrcLayerTest.cpp              ← known vectors, single-bit/burst error detection
│   ├── SessionLayerTest.cpp          ← happy path, retransmit, sequence numbers
│   ├── ApplicationLayerTest.cpp      ← dispatch, unknown ID, payload size guards
│   └── IntegrationTest.cpp           ← full stack end-to-end round-trips
├── .github/
│   └── workflows/
│       └── ci.yml                    ← GitHub Actions: build + test on every push
├── CMakeLists.txt
└── configure.sh                      ← convenience wrapper for cmake invocation
```

---

## Building

**Prerequisites:** CMake 3.20+, GCC (via Homebrew), Ninja, internet access (GoogleTest is fetched automatically via `FetchContent`)

```bash
# Install dependencies (macOS)
brew install gcc ninja cmake

# Configure (first time or after wiping build/)
./configure.sh

# Build
cmake --build build --parallel
```

`configure.sh` is a thin wrapper that sets the correct compiler and build type:

```bash
cmake -B build -G Ninja \
  -DCMAKE_BUILD_TYPE=Debug \
  -DCMAKE_CXX_COMPILER=$(brew --prefix gcc)/bin/g++-15
```

---

## Running Tests

```bash
cd build && ctest --output-on-failure
```

Expected output:

```
Test  #1:  PhysicalLayerTest.SendAndReceive               Passed
Test  #2:  PhysicalLayerTest.ReceiveTimesOut              Passed
Test  #3:  PhysicalLayerTest.AvailableReflectsWaitingBytes Passed
Test  #4:  PhysicalLayerTest.LargePayload                 Passed
Test  #5:  FramingLayerTest.SimpleRoundTrip               Passed
...
Test #30:  IntegrationTest.UnknownCommandFullStack         Passed

100% tests passed, 0 tests failed out of 30
```

CI runs automatically on every push and pull request via GitHub Actions.

---

## Layer Guide

### Physical Layer — `IPhysicalLayer`

The only thing upper layers ever see. Defines three methods:

```cpp
bool   send(const uint8_t* data, size_t len);
size_t receive(uint8_t* buf, size_t len, std::chrono::milliseconds timeout);
size_t available();
```

`SocketPhysicalLayer` implements this using a POSIX `socketpair` — two connected file descriptors that act as the two ends of a wire, usable on any POSIX system without hardware.

---

### Framing Layer — `FramingLayer`

Turns a raw byte stream into discrete messages by wrapping each payload in:

```
0x7E | LEN | ...stuffed payload... | 0x7F
```

Special bytes in the payload are escaped using PPP/HDLC byte stuffing so they are never mistaken for frame boundaries:

| Original byte | Transmitted as |
|---|---|
| `0x7E` (START) | `0x7D 0x5E` |
| `0x7F` (END) | `0x7D 0x5F` |
| `0x7D` (ESC) | `0x7D 0x5D` |

---

### CRC Layer — `CrcLayer`

Appends a 2-byte CRC-16/CCITT checksum to every payload. The receiver recomputes and compares — any single-bit error or typical burst error is caught and the frame is discarded.

Uses a precomputed 256-entry lookup table (one XOR + one table lookup per byte) rather than bit-by-bit polynomial division.

Standard test vector: `CRC("123456789") == 0x29B1`.

---

### Session Layer — `SessionLayer`

Implements stop-and-wait ARQ via a state machine:

```
IDLE ──send()──► SENDING ──► WAITING_FOR_ACK
                                    │
                     ┌──────────────┤
                ACK  │         NACK │  timeout
                     ▼              ▼
                    IDLE     RETRANSMITTING ──► WAITING_FOR_ACK
                                    │
                            max retries (3) hit
                                    ▼
                                IDLE (returns false)
```

Each frame carries an 8-bit sequence number echoed in the ACK/NACK, so the sender knows exactly which frame is being acknowledged.

---

### Application Layer — `ApplicationLayer`

Maintains a `msg_id → CommandHandler` registry. On receive, looks up the handler and calls it with the payload. Returns `0xFF` for unregistered message IDs. Enforces a 128-byte maximum payload size.

```cpp
app.register_command(0x01, [](const std::vector<uint8_t>& payload) {
    // process payload, return response
    return response;
});
```

---

## Swapping the Physical Layer

To use a real UART instead of sockets, implement `IPhysicalLayer`:

```cpp
class UartPhysicalLayer : public proto::IPhysicalLayer {
public:
    explicit UartPhysicalLayer(const char* device, int baud);

    bool send(const uint8_t* data, size_t len) override;
    size_t receive(uint8_t* buf, size_t len,
                   std::chrono::milliseconds timeout) override;
    size_t available() override;

private:
    int fd_; // open("/dev/ttyUSB0", O_RDWR | O_NOCTTY)
};
```

Nothing above `IPhysicalLayer` changes — the framing, CRC, session, and application layers are completely unaware of the physical medium.

---

## Design Decisions

| Decision | Choice | Alternative | Why |
|---|---|---|---|
| Byte stuffing | PPP/HDLC escape | COBS | Simpler to debug; COBS has better worst-case overhead for high-throughput binary |
| CRC width | CRC-16/CCITT | CRC-32 | 2 bytes overhead vs 4; adequate for ≤64KB frames |
| ARQ scheme | Stop-and-wait | Sliding window | Correct for low-latency links; sliding window needed for high-throughput pipes |
| Protocol states | State machine | Nested if/else | Illegal transitions are structurally impossible |
| Physical abstraction | Pure-virtual interface | Templates | Runtime polymorphism enables mock injection without recompilation |
| Sequence numbers | 8-bit wrapping | 16-bit | Sufficient for stop-and-wait; a sliding window implementation would need more |
| Test physical layer | POSIX socketpair | Loopback UART | Works on any POSIX host, no hardware, no driver setup |