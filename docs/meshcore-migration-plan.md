# MeshCore migration plan

Replacing the hand-rolled LoRa flood mesh with [MeshCore](https://github.com/meshcore-dev/MeshCore) (MIT).

> **Self-contained brief.** Everything needed to execute this is below, including verified
> MeshCore internals so you don't have to re-derive them. Work happens on the **`meshcore`**
> branch in **both** repos (`esp32-utilities` and `Celestial-Wayfinder`).

---

## 1. Why

`Celestial-Wayfinder` runs a hand-written controlled-flood mesh: ~831 LOC of engine
(`LoraManager.hpp` + `LoraUtilities.hpp` in `esp32-utilities`), a 200-LOC SX1276 driver in
the app, and ~1,670 LOC of UI on top. Four defects are structural, not incidental:

- **Dedup is broken by design.** `std::unordered_map<sender, last_msgID>` holds exactly one
  msgID per sender and is never pruned. An A→B→A msgID alternation re-floods forever, and
  the map grows without bound as senders accumulate.
- **No airtime control.** Broadcast at TTL 3, no duty-cycle awareness, reliability by blind
  repetition — `Num Broadcasts` (default 3) means every ping is transmitted three times.
- **Unauthenticated crypto.** AES-128-CBC + PKCS7 with no MAC; any relay can flip ciphertext
  bits undetected. "Wrong chatroom" is detected by PKCS7 padding failure — a ~1/256 accident
  filter, not authentication.
- **Fragile type dispatch.** Message type is an FNV-1a hash over *sorted payload key names*,
  never transmitted, hand-synced with a string literal (`schemaHash("abgnors")`). Two types
  with the same key set are indistinguishable.

MeshCore fixes all four: bounded packet-hash dedup, encrypt-then-MAC over a shared group
secret, an explicit typed packet header, and airtime budgeting.

**Estimate: ~9–13 focused days**, all-in, across both repos and all three hardware revs.

For calibration: fixing dedup and crypto *in place* is ~3–5 days and touches no radio code.
MeshCore is worth it for owning less protocol, not as the cheapest path to correctness.

---

## 2. Scope — locked, do not re-litigate

| Decision | |
|---|---|
| **Routing engine only** | Closed network. No interop with public MeshCore nodes, so no obligation to match their regional radio presets (US MeshCore is 910.525 MHz SF7/BW62.5/CR5; we stay on our own plan). |
| **RadioLib** | Replaces the `Blake-Ballew/arduino-LoRa` fork. |
| **Broadcast only** | Group channel, single `Channel Key`. **No** contacts, adverts, ECDH, directed routing, or ACKs. |
| **No UI changes** | ~1,670 LOC of LoRa-touching UI stays as-is. `LoraModule::Utilities` façade signatures preserved byte-identical. |
| **No MeshCore fork** | See §6.3. |
| **Keep `LoraModule::Manager`** | Same name, same file — it gains `: mesh::Mesh` and loses its routing internals. |

**Out of scope:** contacts, adverts, per-peer ECDH, `createDatagram`, ACKs, directed/path
routing, TRACE, MULTIPART, REQ/RESPONSE, multiple channels, RPC-over-LoRa.

Revisiting the contacts/ACK cluster later (~4–6 days) is what would turn `"N echoes"` into a
real `"Delivered"` and make `@Name#TAG` an addressed send rather than a string prefix.

**TRACE specifically cannot be added without that cluster** — flood routing rejects it:

```cpp
if (packet->getPayloadType() == PAYLOAD_TYPE_TRACE) {
  MESH_DEBUG_PRINTLN("%s Mesh::sendFlood(): TRACE type not suspported"
```

It requires `sendDirect()` against an established path. Noted so it isn't re-researched.

---

## 3. Environment

Build locally — a cloud session **cannot** compile this: the egress policy returns 403 on
CONNECT to `api.registry.platformio.org` and `dl.registry.platformio.org`, so the
`espressif32@6.13.0` platform and IDF 5.5 toolchain can't be fetched. GitHub is reachable,
but platform packages resolve through the blocked registry.

Relevant existing config:

- `Celestial-Wayfinder/platformio.ini` — `framework = espidf`, `platform = espressif32@6.13.0`,
  `platform_packages = platformio/framework-espidf @ 3.50503.0`, `lib_ldf_mode = deep`,
  `lib_compat_mode = off`, `-std=gnu++17`.
- `Celestial-Wayfinder/src/idf_component.yml` — `espressif/arduino-esp32: ^3.3.10`
  (Arduino as an IDF managed component, so `Arduino.h`, `millis()`, `SPIClass`, `LittleFS`
  are all genuinely available).
- `sdkconfig.defaults` — `CONFIG_FREERTOS_HZ=1000`; **no** `CONFIG_FREERTOS_USE_TICKLESS_IDLE`,
  **no** `CONFIG_PM_ENABLE`.
- Envs: `local-library-v1`, `local-library-v2` (esp32dev), `hardware-v3`,
  `hardware-v3-tag-connect` (esp32-s3-devkitm-1). `-D HARDWARE_VERSION=1|2|3`.
- `esp32-utilities` is consumed via `symlink://../esp32-utilities`; **`library.json` governs
  its include paths**, not its `platformio.ini` (whose include list is stale).

---

## 4. Architecture

```
CELESTIAL-WAYFINDER (app)
  BootstrapLora, per HW rev          ← pins, SPI, TX power, frequency
      ↓ constructs
  RadioLibLoRaDriver                 ← : CustomSX1276Wrapper. RadioLib SX1276,
      ↓ passed to                       SF7/BW500/CR4:8. Replaces ArduinoLoRaDriver.
ESP32-UTILITIES (library)
  LoraModule::Manager                ← : mesh::Mesh. Group datagrams, forwarding.
  LoraModule::MeshTables               : SimpleMeshTables. Dedup + echo counting.
      ↓ preserved seams
  LoraModule::Utilities              ← SendMessage / MessageTypeReceived / MyLastBroadcast
      ↓ UNCHANGED
CELESTIAL-WAYFINDER (app)
  WayfinderLoraState, HomeWindow, ViewMessageState, …   (~1,670 LOC)
```

**Naming rule.** `esp32-utilities` is the generic reusable library and must **not** carry
app-specific names — no `Wayfinder*` classes in it. Library types stay in the existing
`LoraModule::` namespace; app-side types keep the app's conventions.

**The load-bearing constraint.** Keep these `LoraModule::Utilities` signatures byte-identical
so the entire UI layer compiles untouched:

```cpp
static bool SendMessage(std::shared_ptr<LoraMessageInterface> msg);
static EventHandler<std::shared_ptr<LoraMessageInterface>, bool>& MessageTypeReceived(uint32_t schemaGuid);
static bool RegisterMessageType(uint32_t schemaGuid, MessageCreator creator);
static std::shared_ptr<LoraMessageInterface> MyLastBroadcast();
static bool MyLastBroadcastExists();
static EventHandler<>& MyLastBroadcastChanged();
static uint32_t GetEchoCount();  static void IncrementEchoCount();
static void GenerateDefaultSettings(std::vector<std::shared_ptr<FilesystemModule::SettingsInterface>>&);
static void UpdateSettings(JsonDocument&);
```

Consumers (do not modify): `WayfinderLoraState.hpp` (406), `HomeWindow.hpp` (626, the only
send site, L277), `ViewMessageState.hpp` (388), `DisplaySentMessageState.hpp` (256, echo UI
at L100-105), `RepeatMessageState.hpp` (129), `SavedStatusMsgListState.hpp` (147),
`HomeState.hpp` (150), `CompassUtils.h` (537), `BootstrapLeds.hpp` (`_WireTraceFlows`,
L355-378), `SaveStatusMessageWindow.hpp` (52).

---

## 5. Verified MeshCore internals

Read from the actual source. Trust these over any summary.

### 5.1 Group channel — how separation works

```cpp
class GroupChannel {          // PATH_HASH_SIZE = 1
public:
  uint8_t hash[PATH_HASH_SIZE];
  uint8_t secret[PUB_KEY_SIZE];   // 32
};
```

Payload layout for `PAYLOAD_TYPE_GRP_DATA` (0x06):

| offset | bytes | contents |
|---|---|---|
| 0 | 1 | `channel.hash` — **cleartext** |
| 1 | 2 | MAC (`CIPHER_MAC_SIZE`) |
| 3 | … | AES-128 ciphertext (`CIPHER_KEY_SIZE` = 16) |

`MAX_GROUP_DATA_LENGTH` = `MAX_PACKET_PAYLOAD`(184) − `CIPHER_BLOCK_SIZE`(16) − 3 = **165**.

Send: `memcpy(&payload[len], channel.hash, PATH_HASH_SIZE)` then
`Utils::encryptThenMAC(channel.secret, …)`.
Receive: read the hash byte → `searchChannelsByHash(&channel_hash, channels, 4)` →
try `Utils::MACThenDecrypt` on each candidate until one returns `len > 0`.

**Two-layer separation:** the cleartext byte is only a *selector* (1 byte ⇒ collisions
expected by design, hence up to 4 candidates); the real separation is encrypt-then-MAC.
The 2-byte MAC is ~1/65536 false-accept — ample against collisions, weak against a
determined forger, but strictly better than today's zero authentication.

### 5.2 Dedup and the echo counter — **fully resolved**

```cpp
// Packet.cpp — hashes TYPE + PAYLOAD only; path excluded (except TRACE).
// ⇒ a relayed copy hashes identically. This is why dedup works across hops.
void Packet::calculatePacketHash(uint8_t* hash) const {
  SHA256 sha;
  uint8_t t = getPayloadType();
  sha.update(&t, 1);
  if (t == PAYLOAD_TYPE_TRACE) { sha.update(&path_len, sizeof(path_len)); }
  sha.update(payload, payload_len);
  sha.finalize(hash, MAX_HASH_SIZE);
}
```

`calculatePacketHash` is **public**. `SimpleMeshTables::wasSeen()` is **virtual**. And
critically, `Mesh::sendFlood()` (Mesh.cpp:651 and four sibling send paths) does:

```cpp
_tables->markSeen(packet); // mark this packet as already sent in case it is rebroadcast back to us
```

So your own outbound packet is in the table before it leaves. **Every echo returns
`wasSeen() == true`, starting with the first.** The counter is therefore:

```cpp
namespace LoraModule {
  class MeshTables : public SimpleMeshTables {
  public:
    void setOwnOutbound(const mesh::Packet* p) { p->calculatePacketHash(_own); _has_own = true; }

    bool wasSeen(const mesh::Packet* packet) override {
      bool seen = SimpleMeshTables::wasSeen(packet);
      if (seen && _has_own) {
        uint8_t h[MAX_HASH_SIZE];
        packet->calculatePacketHash(h);
        if (memcmp(h, _own, MAX_HASH_SIZE) == 0) { Utilities::IncrementEchoCount(); }
      }
      return seen;
    }
  private:
    uint8_t _own[MAX_HASH_SIZE]; bool _has_own = false;
  };
}
```

Call `setOwnOutbound(pkt)` on the packet created for the local user's own ping, before
`sendFlood()`.

Storage is a circular buffer, `MAX_PACKET_HASHES` = `128+32` = **160** hashes ×
`MAX_HASH_SIZE`(8) = 1280 bytes, fixed. That is the fix for the unbounded-map defect.
`getNumFloodDups()` / `getNumDirectDups()` are free extra stats.

*(Minor: if 160 other packets pass through between send and echo, the entry is evicted and
that echo is missed. Irrelevant at this traffic level.)*

### 5.3 Forwarding / node roles

```cpp
bool Mesh::allowPacketForward(const mesh::Packet* packet) {
  return false;  // by default, Transport NOT enabled
}
```

MeshCore's Companion/Repeater split is firmware packaging, **not protocol** — its own
`simple_secure_chat` client example forwards everything, and shipped firmware exposes it as
`set repeat {on|off}`. Override to `return true`, gated on a new
`FilesystemModule::BoolSetting("Repeat", true)` (`SettingsInterface.hpp:309`).

⚠️ **The default is `false` — forget the override and you get a silent one-hop network.**

⚠️ **Port the RSSI backoff.** MeshCore's default has no signal-strength term:

```cpp
uint32_t Mesh::getRetransmitDelay(const mesh::Packet* packet) {
  uint32_t t = (_radio->getEstAirtimeFor(packet->getRawLength()) * 52 / 50) / 2;
  return _rng->nextInt(0, 5)*t;
}
```

The current logic (`LoraManager.hpp:132-152`) maps RSSI [-130,-80] → [0,2000] ms *inverted*
so weak-signal/distant nodes relay first, plus `rand() % 500` jitter because co-located
devices otherwise clamp to identical values and transmit simultaneously. That is genuinely
ahead of the default — reimplement it in `getRetransmitDelay()`.

### 5.4 Airtime budget

`duty_cycle = 1.0f / (1.0f + getAirtimeBudgetFactor())`; bucket caps at
`getDutyCycleWindowMs()` (default 3600000) × duty_cycle and refills at `elapsed × duty_cycle`.
Drain is actual airtime on send completion. Gate before TX:

```cpp
uint32_t est_airtime = _radio->getEstAirtimeFor(MAX_TRANS_UNIT);   // note: MAX, not actual
if (tx_budget_ms < est_airtime / MIN_TX_BUDGET_AIRTIME_DIV) {      // DIV = 2
  next_tx_time = futureMillis((unsigned long)(needed / duty_cycle));
  return;                                                          // delayed, never dropped
}
```

RX airtime is tracked but **not** charged. At SF7/BW500/CR4:8 a 255-byte packet is ~158 ms
and a real ~110-byte ping ~72 ms, so factor 1.0 (50%) allows roughly 7 packets/sec sustained
— far beyond need. US 902–928 has no regulatory duty cycle (FCC 15.247 imposes dwell time),
so this is politeness, not compliance. `getAirtimeBudgetFactor()` is app-supplied; 1.0 is the
conventional start.

### 5.5 Radio wrapper — interrupts are preserved

```cpp
static volatile uint8_t state = STATE_IDLE;

static ICACHE_RAM_ATTR void setFlag(void) { state |= STATE_INT_READY; }

void RadioLibWrapper::begin() {
  _radio->setPacketReceivedAction(setFlag);  // this is also SentComplete interrupt
  ...
}
```

Same ISR-does-no-SPI discipline as the current `onDio0Deferred` fork. `recvRaw()` and
`isSendComplete()` gate on the flag. **DIO0 covers both RX-done and TX-done.**

⚠️ **`CustomSX1276::std_init()` takes pins and radio params from compile-time macros**
(`P_LORA_SCLK/MISO/MOSI`, `LORA_FREQ/BW/SF/CR/TX_POWER`), which clashes with the runtime
per-`HARDWARE_VERSION` bootstrap pattern. **Do not use `std_init()`** — construct
`CustomSX1276(new Module(cs, irq, rst, gpio, spi))` with runtime pins and call RadioLib's
`begin(freq, bw, sf, cr, syncWord, power, preambleLen)` plus `setCRC(1)` directly.

### 5.6 `Dispatcher::loop()`

Never blocks, sleeps, or spawns anything. Handles **at most one inbound and one outbound
packet per call** — it does not drain queues. While a transmission is in flight it
early-returns without servicing inbound. Includes a free stuck-radio watchdog: `_err_flags |=
ERR_EVENT_STARTRX_TIMEOUT` if the radio sits out of RX mode for 8 s.

### 5.7 Useful shipped helpers

- `helpers/IdentityStore.h` — `load()`/`save()` a `mesh::LocalIdentity` to LittleFS. **Use
  this** rather than hand-rolling NVS persistence.
- `helpers/ArduinoHelpers.h` — `StdRNG : mesh::RNG`, `ArduinoMillis : mesh::MillisecondClock`,
  `VolatileRTCClock : mesh::RTCClock`.
- `helpers/StaticPoolPacketManager.h`, `helpers/ESP32Board.h`.
- MeshCore `lib_deps`: RadioLib (pinned commit) + `rweather/Crypto @ ^0.4.0`.

---

## 6. Phases

### Phase 0 — Spike: build + bare radio (2–3 days) ⚠️ riskiest

The unknown is MeshCore (Arduino-oriented) under `framework = espidf`. Partly de-risked by
the `arduino-esp32 ^3.3.10` managed component; residual risk is MeshCore's `helpers/`
assuming Arduino core **2.x** while this project is on **3.x** (IDF 5.5).

1. Establish a **green baseline build first** (`pio run -e hardware-v3`) before changing anything.
2. Add MeshCore + RadioLib to `lib_deps`; get it to compile. Resolve `#ifdef` fallout.
3. RadioLib SX1276 on a V3 board, bare TX/RX, no mesh. Confirm RadioLib accepts the existing
   `BootstrapMicrocontroller::SpiBus()` `SPIClass` instance.

**Exit:** two V3 boards exchanging raw packets at SF7/BW500/CR4:8.
**If this blows up, stop and reconsider** — everything downstream depends on it.

### Phase 1 — MeshCore skeleton (3–4 days)

- `RadioLibLoRaDriver : CustomSX1276Wrapper` in
  `Celestial-Wayfinder/include/HelperClasses/LoRaDriver/`. SF7 / BW 500 kHz / CR 4:8 /
  preamble 12 / CRC on; frequency from the existing `LoraModule::ChannelToHz()`.
  Runtime pins per §5.5 — no `std_init()`.
- `LoraModule::Manager : mesh::Mesh` (in place, `include/ModuleManagers/LoraManager.hpp`) and
  `LoraModule::MeshTables : SimpleMeshTables`, wired with `StdRNG`,
  `StaticPoolPacketManager`, and an `RTCClock` bridged to `System_Utils::GetCurrentUTC()`
  (reuses the existing TimeSource registry rather than `VolatileRTCClock`).
- `mesh::MainBoard` for ESP32, or MeshCore's `ESP32Board` if it doesn't collide with
  `BootstrapMicrocontroller`.
- Ed25519 identity via `IdentityStore` on the existing LittleFS. Derive the legacy 32-bit
  `System_Utils::DeviceID` from the first 4 bytes of the pubkey — this keeps
  `PingMessage::SenderTag()`'s `"#BEEF"` and every `uint32_t sender`-keyed map working
  unchanged, and removes the collision risk in today's *user-editable* `"UserID"` setting.
- Wait discipline — §6.3.

**Exit:** two boards, hardcoded group datagram round-trip. **Measure idle current draw.**

### Phase 2 — PingMessage over the group channel (2–3 days)

- Derive the `GroupChannel` secret from `Channel Key`: **keep `EncryptionUtils::DeriveKey`'s
  PBKDF2-HMAC-SHA256** (10k iters, fixed salt `"CelestialWayfinder-LoRa-v1"`), widened
  16→32 bytes so the same passphrase yields the same secret across devices. Derive `hash`
  deterministically from the secret (e.g. first byte of its SHA-256). Single channel ⇒
  `searchChannelsByHash()` is ~5 LOC.
- `PingMessage` rides as `createGroupDatagram(PAYLOAD_TYPE_GRP_DATA, channel, data, len)` +
  `onGroupDataRecv`, sent with `sendFlood`. Payload stays msgpack, prefixed with a **1-byte
  type tag** replacing `schemaHash`. It encodes to ~74 bytes — comfortably inside 165, so
  lat/lng stay `double`.
- `LoraMessageInterface` keeps `serializePayload`/`deserializePayload`/`clone`/
  `GetPrintableInformation`; loses `SchemaGuid`, `serialize(JsonDocument&)`,
  `deserialize(JsonDocument&)`.
- Rewrite `LoraModule::Utilities` internals behind the unchanged façade. Delete
  `ReadBaseFields`, `RelayMessage`, `RecordRouting`, `MessageExists`, `RoutingMap`, and the
  AES paths in `SerializeMessage`/`DeserializeMessage`.
- Echo counter per §5.2.
- Settings: `Channel Key` stays, `LoRa Channel` stays (`RequestChannel`/`TakePendingChannel`
  collapses to a direct call now one task owns the radio). **Remove `Num Broadcasts`** —
  MeshCore owns retry and airtime. **Add `BoolSetting("Repeat", true)`**. Override
  `getRetransmitDelay()` per §5.3.

**Exit:** full app functionality; 3 boards, A/C out of range, B relaying; B forwards once not
repeatedly; echo count increments; `Repeat = off` on B stops C hearing A.

⚠️ **Also verify:** a packet whose channel doesn't match should still be **relayed** —
forwarding is decided at the route layer, and `searchChannelsByHash` returning 0 only skips
the callback. This preserves today's deliberate "relay other chatrooms' traffic" behaviour.
Test with two devices on different `Channel Key`s and a third relaying between them.

### Phase 3 — All three hardware revs (1–2 days)

| | CS | RST | DIO0 | TX pwr | SPI |
|---|---|---|---|---|---|
| V1 | 15 | **-1** | 18 | 20 | `SPIClass(HSPI)` via `BootstrapMicrocontroller::SpiBus()` |
| V2 | 15 | **-1** | 18 | 23 | same |
| V3 | 39 | 38 | 48 | 23 | SCK 40 / MISO 42 / MOSI 41 |

⚠️ **V1/V2 have no reset pin wired.** Pass `RADIOLIB_NC` and verify `SX1276::begin()`
survives without a hardware reset on this silicon. Second-riskiest unknown after Phase 0.

**Exit:** `pio run -e local-library-v1 -e local-library-v2 -e hardware-v3` clean; one board of
each rev on the same mesh.

### Phase 4 — Cleanup (1 day)

- Delete `ArduinoLoRaDriver.h` (200), `LoraDriverInterface.h` (31), the AES-CBC/PKCS7 half of
  `EncryptionUtils.hpp` (~120 of 166 — **`DeriveKey` stays**), and the already-dead
  `UnreadMessageState.hpp` (176 LOC — calls removed APIs, not compiled, construction
  commented out at `HomeWindow.hpp:61` and `:117`).
- Drop `https://github.com/Blake-Ballew/arduino-LoRa.git` from `lib_deps`.
- Fix `esp32-utilities/CLAUDE.md` — it documents **RadioHead** and
  `MessageBase::MessageFactory`, neither of which exists in the codebase today. Replace with
  the MeshCore architecture.

### LOC delta

| | LOC |
|---|---|
| Deleted | ~1,250 |
| Added | ~600–800 |
| Modified | ~200 (bootstraps, settings, façade internals) |
| **UI untouched** | **~1,670** |

---

## 6.3 Wait discipline

One FreeRTOS task calling `mesh.loop()` replaces **both** `RadioTask` and `SendQueueTask`;
the `System_Utils::registerQueue` send queue disappears (MeshCore owns its own priority
queue). The app's `LoRaReceiveISR` (`EventDeclarations.cpp:262-271`) goes away — RadioLib
owns DIO0 now.

`Dispatcher::loop()` never blocks and its outbound scheduling is *time*-driven, so the task
cannot sleep on `portMAX_DELAY`:

```cpp
for (;;) {
    ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(timeoutUntilNextDeadline()));
    mesh.loop();   // call 2–3× — one packet per direction per call
}
```

Two wake sources, both to implement:

1. **Timeout** — earlier of `Dispatcher::next_tx_time` (protected; `LoraModule::Manager`
   subclasses it) and the outbound queue head's `scheduled_for`, capped at **~20 ms**. Use
   MeshCore's `millisHasNowPassed`/`futureMillis` idiom for `millis()` rollover safety.
2. **App-initiated send** — `LoraUtils::SendMessage()` runs on the display/RPC task and must
   `xTaskNotifyGive(meshTaskHandle)` after enqueueing. Same pattern as today's
   `SendQueueTask → RadioTask` notify. Easy to forget; the symptom is a send that sits until
   the next tick.

**No ISR wake, and no fork.** `setFlag`/`state` are file-scope in `RadioLibWrappers.cpp`, and
DIO0 belongs to RadioLib so a second `attachInterrupt` can't chain (ESP32 allows one handler
per GPIO). A subclass *could* re-register its own ISR, but `recvRaw()`/`isSendComplete()`
read that flag, so you'd also reimplement the `STATE_IDLE/RX/TX/INT_READY` machine,
`startRecv()` and the stats — ~60–100 LOC of duplicated radio state, worse than a fork.

**The ~20 ms tick costs essentially nothing here:** `CONFIG_FREERTOS_HZ=1000` with no
tickless idle and no `CONFIG_PM_ENABLE` means the CPU already wakes 1000×/s unconditionally,
so 50 Hz from one task is ~5% more scheduler activity. Residual cost is ≤20 ms RX latency;
the SX1276 FIFO holds one packet, but a second arrival inside 20 ms is usually a *duplicate*
relay that `wasSeen()` discards anyway.

⚠️ Revisit only if `CONFIG_PM_ENABLE` + tickless idle are ever enabled for battery life —
that is a device-wide project (every task on `vTaskDelay(2000)`, plus NimBLE and WiFi), not a
LoRa decision.

---

## 7. Fleet migration

Wire format changes completely — **old and new firmware cannot talk**, and there is no
gradual rollout that keeps a mixed fleet working.

Recommend a **flag day over the existing OTA path**: `CompassUtils.h:347-350` already
registers `BeginOTA` / `UploadOTAChunk` / `EndOTA` RPCs over serial/WiFi/BLE, so devices can
be updated from the companion app without a mixed-protocol period.

Do **not** build a dual-stack or a build-flag engine selector — two radio drivers contending
for one SPI bus and one modem config is worse than a coordinated update. Archive the current
firmware image for rollback.

---

## 8. Verification

| Phase | Bench | Check |
|---|---|---|
| 0 | 2 × V3 | Baseline `pio run -e hardware-v3` green *first*; then raw RadioLib TX/RX in serial log |
| 1 | 2 × V3 | Group datagram round-trip; **idle current draw vs. baseline** |
| 2 | 3 × any | A→C with B relaying; B forwards once not repeatedly; echo count increments; `Repeat = off` on B silences C; cross-channel relay still works |
| 3 | 1 × each rev | All envs build; cross-rev mesh join |

No RPC-over-LoRa exists, so mesh testing is bench-and-logs via the existing `ESP_LOG` tags
(`LoraManager`, `LoraUtils`).

## 9. Risks, ordered

1. **MeshCore under `framework = espidf`** (Phase 0). Unquantified until the build is tried.
   Worst case forces `framework = arduino, espidf`, which touches far more than LoRa.
2. **RadioLib SX1276 on V1/V2 with no reset pin** (Phase 3).
3. **Flag day** across three hardware revs in the field.

*(The echo-counter hook was a risk until verified — §5.2 resolves it.)*
