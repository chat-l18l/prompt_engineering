# prompt_engineering
example prompts for common LLM's

eerste experiment : [link naar c_coding_style.md](c_coding_style.md)

## string_view voorbeeld
Minimalistische C string_view helper (geïnspireerd door Tsoding) vind je in `include/mt_string_view.h` en `src/mt_string_view.c`. Bouw de demo met:

```
cc -std=c11 -Wall -Wextra -pedantic -Iinclude examples/mt_sv_example.c src/mt_string_view.c -o /tmp/mt_sv_example
```

## Minimal Opus LAN voice chat (C + CMake)
Voor moderne Linux desktop is **PipeWire** het meest gangbaar. Deze demo gebruikt de stabiele **ALSA API** (`default` device), die op PipeWire-systemen meestal automatisch via `pipewire-alsa` routed.

Build:

```bash
cmake -S . -B build
cmake --build build -j
```

Run op receiver host:

```bash
./build/opus_lan_chat rx 0.0.0.0 5000
```

Run op sender host:

```bash
./build/opus_lan_chat tx 192.168.1.50 5000
```

Kenmerken:
- Mono 16 kHz, 20 ms frames
- Opus VOIP mode, DTX + in-band FEC aan
- UDP voor low-latency LAN point-to-point
