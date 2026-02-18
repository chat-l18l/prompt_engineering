# prompt_engineering
example prompts for common LLM's

eerste experiment : [link naar c_coding_style.md](c_coding_style.md)

## string_view voorbeeld
Minimalistische C string_view helper (geïnspireerd door Tsoding) vind je in `include/mt_string_view.h` en `src/mt_string_view.c`. Bouw de demo met:

```
cc -std=c11 -Wall -Wextra -pedantic -Iinclude examples/mt_sv_example.c src/mt_string_view.c -o /tmp/mt_sv_example
```

## ESP32-S3 standaard ESP-IDF voorbeeld (CLI)
Een minimaal ESP-IDF command-line project met timer + LED blink + W5500 staat in `examples/esp32s3_idf_minimal`. Zie: `examples/esp32s3_idf_minimal/README.md`.

## ESP32 + PlatformIO + ESP-IDF voorbeeld
Een minimaal timer + blinking LED voorbeeld staat in `examples/esp32_idf_platformio_minimal`. Zie de lokale handleiding: `examples/esp32_idf_platformio_minimal/README.md`.
