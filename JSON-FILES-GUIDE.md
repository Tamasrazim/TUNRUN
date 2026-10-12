# JSON Files — Plain-English Guide

JSON cannot contain `#` or `//` comments. Adding them to a JSON file would make the build tools reject it, so this guide explains the one JSON configuration file in this repository.

- `CMakePresets.json` — stores named CMake configure/build presets so the project can use consistent build settings instead of requiring every option to be typed by hand.

The C++ headers and source files have short, plain-English comments at the top describing what each file is responsible for. The longer design documents in `docs/` explain the gameplay systems in more detail.
