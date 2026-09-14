"""
Trim MeshCore to the routing engine only.

MeshCore ships `build_as_lib.py` as its `library.json` `build.extraScript`. That
script's SRC_FILTER is additive-only -- it always compiles every `helpers/*.cpp`
(CommonCLI, BaseChatMesh, ClientACL, ESP32Board, sensors, ...) plus the RadioLib
device wrappers for every supported chip. The MeshCore migration (see
meshcoremigrationplan.md section 2) consumes the *routing engine only*: no
contacts, adverts, CLI, chat, board glue, or RTC discovery.

Overriding `SRC_FILTER` on the library build env from a project extra_script
does not work -- PlatformIO's LDF collects MeshCore's sources from the wide
filter before (or regardless of) a project `pre:`/`post:` script touching the
env. The reliable lever is `library.json` `build.srcFilter`, which
`PlatformIOLibBuilder.src_filter` checks *first*, ahead of the extraScript's env
manipulation. So this script rewrites the fetched MeshCore manifest in place:

  * drop `build.extraScript` (build_as_lib.py) -- we don't use its variant /
    display / platform knobs (no MC_VARIANT / DISPLAY_CLASS defined)
  * set `build.srcFilter` to the engine subset -- including `+<../lib/ed25519/*.c>`
    so MeshCore's vendored ed25519 compiles into libMeshCore.a. (LDF won't link
    it on its own: `Identity.cpp` includes it as `<ed_25519.h>`, and angle-bracket
    includes are not resolved to a library.)
  * put `<libdeps>/MeshCore/lib/ed25519` on the project CPPPATH so that
    `#include <ed_25519.h>` resolves.

The edit is idempotent and re-applies after `pio pkg install` refetches the dep,
so it is not a MeshCore fork -- it's app-repo build configuration.

It also injects -DLORA_SF into MeshCore's own compilation (via library.json
build.flags). RadioLibWrappers.h:55 has an unreplaced fallback
`getSpreadingFactor() { return LORA_SF; }`, so MeshCore's TU needs the macro to
exist even though every real subclass overrides it with the live radio read.
The project-side stubs for our own TUs live in RadioLibLoRaDriver.hpp; keeping
this one here means no dead modem constants in platformio.ini. Value unused; 7
matches the real SF purely so a future grep finds something sensible.
"""

import json
import os

Import("env")  # noqa: F821

_LORA_SF_FALLBACK = ["-DLORA_SF=7"]

_ENGINE_SRC_FILTER = [
    "-<*>",
    "+<Dispatcher.cpp>",
    "+<Identity.cpp>",
    "+<Mesh.cpp>",
    "+<Packet.cpp>",
    "+<Utils.cpp>",
    "+<helpers/IdentityStore.cpp>",
    "+<helpers/StaticPoolPacketManager.cpp>",
    "+<helpers/radiolib/RadioLibWrappers.cpp>",
    "+<../lib/ed25519/*.c>",   # vendored Ed25519; LDF won't pull it (angle-bracket include)
]

_mc_dir = os.path.join(
    env.subst("$PROJECT_LIBDEPS_DIR"), env.subst("$PIOENV"), "MeshCore"  # noqa: F821
)
_manifest_path = os.path.join(_mc_dir, "library.json")
_ed25519_inc = os.path.join(_mc_dir, "lib", "ed25519")

if os.path.isdir(_ed25519_inc):
    env.Append(CPPPATH=[_ed25519_inc])  # noqa: F821

if not os.path.isfile(_manifest_path):
    print("meshcore_lib_filter: %s not found yet (skipped)" % _manifest_path)
else:
    with open(_manifest_path, "r", encoding="utf-8") as fh:
        manifest = json.load(fh)

    build = manifest.setdefault("build", {})

    changed = False

    # The trimmed engine subset pulls in none of the chat/CLI/sensor helpers, so
    # RTClib / RV3028 / CayenneLPP are dead weight -- drop them so PlatformIO
    # doesn't refetch them. RadioLib + Crypto are the only real deps (plan 5.7).
    _keep_deps = {"SPI", "Wire", "jgromes/RadioLib", "rweather/Crypto"}
    deps = manifest.get("dependencies", {})
    for _name in list(deps):
        if _name not in _keep_deps:
            del deps[_name]
            changed = True

    if build.pop("extraScript", None) is not None:
        changed = True
    if build.get("flags") != _LORA_SF_FALLBACK:
        build["flags"] = _LORA_SF_FALLBACK   # see module docstring
        changed = True
    if build.get("srcFilter") != _ENGINE_SRC_FILTER:
        build["srcFilter"] = _ENGINE_SRC_FILTER
        changed = True

    if changed:
        with open(_manifest_path, "w", encoding="utf-8") as fh:
            json.dump(manifest, fh, indent=4)
        print("meshcore_lib_filter: patched %s to routing-engine subset" % _manifest_path)
    else:
        print("meshcore_lib_filter: MeshCore manifest already trimmed")
