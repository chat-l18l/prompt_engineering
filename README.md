# prompt_engineering
example prompts for common LLM's

eerste experiment : [link naar c_coding_style.md](c_coding_style.md)

## string_view voorbeeld
Minimalistische C string_view helper (geïnspireerd door Tsoding) vind je in `include/mt_string_view.h` en `src/mt_string_view.c`. Bouw de demo met:

```
cc -std=c11 -Wall -Wextra -pedantic -Iinclude examples/mt_sv_example.c src/mt_string_view.c -o /tmp/mt_sv_example
```
## projectstructuur embedded robotica
Vragen en een uitvoerbaar plan voor een GitHub-project met ESP32-P4, NVIDIA Orin, missie, navigatie, firmware, bring-up tests en integratietests staan in [`docs/github_project_structure.md`](docs/github_project_structure.md).

## WireGuard naast Tailscale

Een beheerbare multi-user WireGuard-opzet naast een bestaande Tailscale-installatie op Ubuntu 24.04 staat in [`docs/ubuntu_wireguard_with_tailscale.md`](docs/ubuntu_wireguard_with_tailscale.md).
