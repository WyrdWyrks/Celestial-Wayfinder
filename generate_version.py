import os
import re
import json

CONFIG_FILE = "version_config.json"
OUTPUT_FILE = "../esp32-utilities/include/Utilities/VersionUtils.h"
TEMPLATE_FILE = "version_template.h"

# Load major, minor, patch
with open(CONFIG_FILE, "r") as f:
    version_data = json.load(f)

major = version_data["major"]
minor = version_data["minor"]
patch = version_data["patch"]
version_string = f"{major}.{minor}.{patch}"

# An env in platformio.ini can report a different version than
# version_config.json (e.g. a beta build), via:
#   custom_firmware_version = 5.0.0-beta
# MAJOR.MINOR.PATCH with an optional -prerelease/+build suffix; the numeric
# parts feed the FIRMWARE_VERSION_MAJOR/MINOR/PATCH macros.
try:
    Import("env")  # noqa: F821 -- provided when run as a PlatformIO extra script
    override = env.GetProjectOption("custom_firmware_version", "").strip()  # noqa: F821
except NameError:  # run directly, e.g. CI's "Generate Version Header" step
    override = ""

if override:
    match = re.fullmatch(r"v?(\d+)\.(\d+)\.(\d+)([-+][0-9A-Za-z.+-]+)?", override)
    if not match:
        raise SystemExit(
            f"custom_firmware_version = {override!r} is not MAJOR.MINOR.PATCH[-suffix], e.g. 5.0.0-beta"
        )
    major, minor, patch = (int(g) for g in match.groups()[:3])
    version_string = override.lstrip("v")

# Load and substitute template
with open(TEMPLATE_FILE, "r") as f:
    template = f.read()

output = (
    template
    .replace("__VERSION_MAJOR__", str(major))
    .replace("__VERSION_MINOR__", str(minor))
    .replace("__VERSION_PATCH__", str(patch))
    .replace("__VERSION_STRING__", version_string)
)

os.makedirs(os.path.dirname(OUTPUT_FILE), exist_ok=True)

with open(OUTPUT_FILE, "w") as f:
    f.write(output)

print(f"Generated version.h: {version_string}")

# Write version.json to build directory
BUILD_DIR = os.path.join("build", "version.json")
os.makedirs(os.path.dirname(BUILD_DIR), exist_ok=True)

with open(BUILD_DIR, "w") as f:
    json.dump({
        "major": major,
        "minor": minor,
        "patch": patch,
        "version": version_string
    }, f, indent=4)

