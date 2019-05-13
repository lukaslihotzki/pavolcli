# PulseAudio Volume CLI

Simple CLI to control the default sink's volume using these commands:
- `!`: toggle mute
- `[`: lower volume -5%
- `]`: raise volume +5%
- `<`: lower volume -5% unbounded (same as `[`)
- `>`: raise volume +5% unbounded (over 100%)

## Build
You can use Makefile `make` or CMake.

## License
This project is licensed under the terms of the ISC license.
