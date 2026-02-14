# ESP32 + ESP-IDF + PlatformIO (minimaal voorbeeld)

Dit voorbeeld laat precies twee dingen zien:

1. Een **hardware GPIO LED blink** op `GPIO2`.
2. Een **periodieke software timer** met `esp_timer` (500 ms).

De timer-callback toggelt de LED-state. Er wordt geen dynamische allocatie door de applicatiecode gebruikt; alle state is statisch.

## Structuur

```text
examples/esp32_idf_platformio_minimal/
├── platformio.ini
└── src/
    └── main.c
```

## Vereisten

- PlatformIO Core (`pio`) geïnstalleerd
- Een ESP32 board (hier geconfigureerd als `esp32dev`)
- USB-verbinding met seriële poort

## Build-proces

Ga naar de voorbeeldmap:

```bash
cd examples/esp32_idf_platformio_minimal
```

### 1) Build

```bash
pio run
```

Wat gebeurt er:
- PlatformIO pakt toolchain + ESP-IDF framework voor `espressif32`.
- `src/main.c` wordt gecompileerd.
- Link naar firmware image voor het gekozen board.

### 2) Flash

```bash
pio run --target upload
```

Optioneel poort forceren:

```bash
pio run --target upload --upload-port /dev/ttyUSB0
```

### 3) Seriële monitor

```bash
pio device monitor -b 115200
```

Verwachte logregel:

```text
I (....) app_main: blink timer started (period=500000 us)
```

Daarna blijft de timer draaien en toggelt de LED elke 500 ms.

## Aanpassen

- LED pin wijzigen: `g_led_gpio` in `src/main.c`
- Blink-periode wijzigen: `g_blink_period_us` in `src/main.c`

## Waarom dit ontwerp

- **Data-first**: compacte globale state (`g_led_state`, `g_blink_timer`).
- **Expliciete fouten**: tagged status (`app_status_t`) i.p.v. verborgen control-flow.
- **Mechanisch transparant**: één timer callback, één GPIO write per tick.
