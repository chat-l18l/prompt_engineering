# C Coding Style & LLM System Prompt

Deze gids bundelt alle afgesproken regels tot één consistente referentie. Gebruik de **Coding Style** om met collega's af te stemmen en de **System Prompt** om dezelfde regels aan een LLM mee te geven.

---

## 1) Kernprincipes (Scalable C + Zig-style errors)

- **Opaque structs & lifecycle**
  - Alleen forward declarations in headers: `typedef struct mt_xxx_s mt_xxx_t;`.
  - Constructors heten `mt_xxx_new()`; destructors gebruiken altijd een dubbele pointer `mt_xxx_destroy(mt_xxx_t **self_p)` en zetten de caller-pointer op `NULL`.
  - Geen globale state; iedere API neemt `self` als eerste argument.
- **Result types (Zig-style tagged unions)**
  - Geen magische waarden, geen `errno`, geen `-1` als error: altijd `mt_result_t`-achtige tagged unions.
  - Typische macro’s in `mt_result.h`:
    - `RESULT_OK(v)` / `RESULT_ERR(e)` (of `MT_OK_*` varianten per type).
    - Convenience voor specifieke result-structs:
      - `#define MT_OK_V(v)   ((mt_result_v_t){ .ok = true,  .u.value = (v) })`
      - `#define MT_ERR_V(e)  ((mt_result_v_t){ .ok = false, .u.err   = (e) })`
      - `#define MT_OK_D(v)   ((mt_result_d_t){ .ok = true,  .u.value = (v) })`
      - `#define MT_ERR_D(e)  ((mt_result_d_t){ .ok = false, .u.err   = (e) })`
- **Error codes (`mt_err.h`)**
  - Positieve codes; `0 = MT_OK`.
  - Algemeen blok: `MT_ERR_NULL`, `MT_ERR_OOM`, `MT_ERR_IO`, `MT_ERR_RANGE`, `MT_ERR_PARSE`, `MT_ERR_BUSY`, `MT_ERR_TIMEOUT`, `MT_ERR_NOTFOUND`, `MT_ERR_PERM`.
  - Module-specifieke codes starten vanaf `1000` (bijv. `MT_ERR_BUF_OVERFLOW`, `MT_ERR_BUF_UNDERFLOW`, `MT_ERR_EVT_LOOP`, `MT_ERR_THR_CREATE`, `MT_ERR_NET_BIND`, `MT_ERR_NET_CONNECT`).
  - Breid per module uit met eigen codes (`>= MT_ERR_CUSTOM`), zoals `MT_ERR_NOT_CONFIGURED` voor robot-modules.
  - `mt_err_str(mt_err_t err)` geeft altijd een menselijke foutboodschap terug; breid de `switch` uit met je custom codes.
  - Helper-macro’s: `#define MT_RETURN_ERR(e) return RESULT_ERR(e)` en `#define MT_RETURN_OK(v) return RESULT_OK(v)`.
- **Safety & clarity**
  - Elke publieke functie start met sanity checks (`if (!self) ...`).
  - Geen debug `printf` in core logic; logging gebeurt in de aanroepende laag of tests.
  - Geen try/catch rond imports; hou includes minimaal en lokaal.
  - Gebruik vaste nauwkeurige constanten (bijv. `#define ROBOT_PI 3.14159265358979323846`).

---

## 2) Header-template (publieke API)

```c
#pragma once
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "mt_err.h"
#include "mt_result.h"

typedef struct robot_ctx_s robot_ctx_t;  // opaque

typedef struct { double w1, w2, w3; } robot_velocity_t;
typedef struct { double dx, dy, dw; } robot_displacement_t;

typedef struct { bool ok; union { robot_velocity_t value; mt_err_t err; } u; } mt_result_v_t;
typedef struct { bool ok; union { robot_displacement_t value; mt_err_t err; } u; } mt_result_d_t;

robot_ctx_t *robot_new(void);
void         robot_destroy(robot_ctx_t **self_p);

mt_result_t   robot_configure(robot_ctx_t *self, double wheel_diameter_m, double robot_radius_m);
mt_result_v_t robot_inv_kinematics_try(const robot_ctx_t *self, double vx, double vy, double omega_deg_s);
mt_result_d_t robot_fk_try(const robot_ctx_t *self, int enc1, int enc2, int enc3);
```

---

## 3) Implementatie-template

```c
#include "robot.h"
#include <stdlib.h>
#include <math.h>
#include <assert.h>

#define ROBOT_PI 3.14159265358979323846
#define SIN_60   0.8660254037844386

struct robot_ctx_s {
    bool   configured;
    double wheel_diameter_m;
    double robot_radius_m;
    double wheel_radius_m;
};

robot_ctx_t *robot_new(void) {
    robot_ctx_t *self = calloc(1, sizeof(*self));
    return self; // caller checkt op NULL (OOM)
}

void robot_destroy(robot_ctx_t **self_p) {
    assert(self_p);
    if (*self_p) {
        free(*self_p);
        *self_p = NULL;
    }
}

mt_result_t robot_configure(robot_ctx_t *self, double wheel_diameter_m, double robot_radius_m) {
    if (!self)              return MT_RETURN_ERR(MT_ERR_NULL);
    if (wheel_diameter_m<=0 || robot_radius_m<=0)
                            return MT_RETURN_ERR(MT_ERR_RANGE);

    self->wheel_diameter_m = wheel_diameter_m;
    self->robot_radius_m   = robot_radius_m;
    self->wheel_radius_m   = wheel_diameter_m / 2.0;
    self->configured       = true;
    return MT_RETURN_OK(NULL);
}

mt_result_v_t robot_inv_kinematics_try(const robot_ctx_t *self, double vx, double vy, double omega_deg_s) {
    if (!self || !self->configured)
        return MT_ERR_V(MT_ERR_NOT_CONFIGURED);

    const double omega_rad_s = ROBOT_PI * omega_deg_s / 180.0;
    const double r = self->wheel_radius_m;
    const double R = self->robot_radius_m;

    const double w1 = (-R * omega_rad_s + vx) / r;
    const double w2 = (-R * omega_rad_s - 0.5 * vx - SIN_60 * vy) / r;
    const double w3 = (-R * omega_rad_s - 0.5 * vx + SIN_60 * vy) / r;

    return MT_OK_V((robot_velocity_t){ w1, w2, w3 });
}

mt_result_d_t robot_fk_try(const robot_ctx_t *self, int enc1, int enc2, int enc3) {
    if (!self || !self->configured)
        return MT_ERR_D(MT_ERR_NOT_CONFIGURED);

    const double r = self->wheel_radius_m;
    const double R = self->robot_radius_m;
    const double inv_det = 1.0 / (3.0 * r);

    const double vx = inv_det * r * (              0.0 * enc1 - SIN_60 * enc2 + SIN_60 * enc3);
    const double vy = inv_det * r * (              1.0 * enc1 -     0.5 * enc2 -     0.5 * enc3);
    const double vw = inv_det * R * ((double)enc1 + (double)enc2 + (double)enc3);

    return MT_OK_D((robot_displacement_t){ vx, vy, vw });
}
```

---

## 4) System Prompt (copy/paste naar je LLM)

```
Je bent "C Architect (ZeroMQ + Zig-style)". Genereer C99/C11 code die voldoet aan deze regels:

- Gebruik Scalable C patronen (Peter Hintjens): opaque structs, new/destroy lifecycle, geen globale state.
- Foutafhandeling: Zig-achtige Result-types (tagged unions); nooit errno, nooit -1/NULL als fout.
- Error codes uit mt_err.h: MT_OK = 0; algemene codes (NULL, OOM, IO, RANGE, PARSE, BUSY, TIMEOUT, NOTFOUND, PERM); module-codes vanaf 1000; custom codes >= MT_ERR_CUSTOM. Voeg per module specifieke codes toe (bv. MT_ERR_NOT_CONFIGURED) en breid mt_err_str uit.
- Result macro’s: RESULT_OK/RESULT_ERR (of MT_OK_*/MT_ERR_* varianten) voor type-safe returns; helper-macro’s MT_RETURN_OK/MT_RETURN_ERR toegestaan.
- Lifecycle: mt_xxx_new() geeft pointer-result; mt_xxx_destroy(mt_xxx_t **p) mag op NULL of ongeïnitialiseerde pointers worden aangeroepen en zet p op NULL.
- Publieke functies starten met sanity checks; geen debug-printfs in core logic.
- Geen try/catch rond imports; includes minimaal en lokaal.
- Constante definities exact (bijv. ROBOT_PI 3.14159265358979323846, SIN_60 0.8660254037844386).
- Code is thread-safe en unit-testbaar; geen verborgen globals.

Lever steeds:
1) Header (opaque types, result types, prototypes)
2) Implementatie (sanity checks, clear flow, return Result-types)
3) Korte uitleg van keuzes als dat helpt de lezer.
```

---

## 5) Quick checklist voor collega’s

- [ ] Headers tonen alleen opaque structs; implementatie verbergt internals.
- [ ] `*_destroy(**p)` zet altijd de caller-pointer op `NULL`.
- [ ] Elke functie valideert inputs en retourneert Result-types; `mt_err_str` dekt alle codes.
- [ ] Geen `printf` in core logic; logging alleen in caller/test.
- [ ] Nieuwe errors? Voeg code + string toe in `mt_err.h` (>=1000 of >=MT_ERR_CUSTOM).
- [ ] Gebruik vaste math-constanten; geen magische getallen in codepaden.
