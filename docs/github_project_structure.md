# GitHub projectstructuur voor embedded Linux, missie, navigatie en firmware

Deze notitie is bedoeld als startpunt om een monorepo of set repositories op te zetten voor code op ESP32-P4, NVIDIA Orin, missie-logica, navigatie, firmware, bring-up tests en integratietests.

De stijl volgt data-oriented design en zero-overhead C/C++: platte modules, expliciete data-eigenaars, geen verborgen heap-allocaties in runtime-kritieke paden, simpele build targets en duidelijke hardwaregrenzen.

---

## Eerst beantwoorden: beslisvragen

Beantwoord deze vragen voordat de definitieve structuur wordt vastgezet.

### 1. Repositorymodel

1. Wil je één monorepo voor alle software, of aparte repositories per target/product?
2. Moeten ESP32-P4, Orin en missie/navigatie altijd op dezelfde commit getest kunnen worden?
3. Wil je externe dependencies vendoren in de repo, pinnen via submodules, of ophalen via package managers/toolchains?

### 2. Product- en targetgrenzen

1. Welke fysieke units bestaan er: ESP32-P4 board, Orin computer, sensormodules, motorcontroller, ground station?
2. Welke code draait waar: bare-metal/RTOS op ESP32-P4, Linux userspace op Orin, kernel modules, containers, scripts?
3. Welke interfaces zijn harde contracten: UART, CAN, Ethernet, SPI, shared memory, UDP, DDS, custom binary protocol?

### 3. Build en toolchains

1. Welke buildsystemen wil je gebruiken: CMake, Meson, Make, ESP-IDF, Zephyr, Bazel, of een combinatie?
2. Moeten builds hermetisch/reproduceerbaar zijn via Docker/Nix/devcontainer?
3. Welke compilerflags zijn verplicht voor C/C++: `-Wall -Wextra -Werror`, sanitizers, LTO, cross-compile sysroots?

### 4. Runtime-architectuur

1. Is Orin één proces met event-loop, meerdere processen met IPC, of containers per service?
2. Welke processen zijn safety-critical of realtime-achtig, en welke mogen crash-only/restartable zijn?
3. Welke data moet zero-copy of bounded-copy blijven: beelden, lidar, audio, navigatie-state, telemetry?

### 5. Teststrategie

1. Welke tests moeten zonder hardware draaien: unit tests, protocol parsers, simulatie, log replay?
2. Welke tests vereisen hardware-in-the-loop: ESP32-P4 bring-up, sensor loopback, motor safety, end-to-end missie?
3. Moeten integratietests deterministisch zijn op opgenomen datasets/logs?

### 6. Release en operations

1. Wat is een release-artifact: firmware image, Debian package, container image, SD-card image, OTA bundle?
2. Hoe worden configuraties beheerd: per robot, per missie, per labopstelling, per omgeving?
3. Welke telemetry/logging is nodig om een veldprobleem offline te reconstrueren?

---

## Voorstel: monorepo als standaard

Kies standaard een monorepo als ESP32-P4, Orin, missie en navigatie samen moeten evolueren. Dat geeft één commit-hash voor een complete robotstaat, simpele cross-target integratietests en minder contractdrift tussen firmware en Linux userspace.

Kies pas meerdere repositories als teams volledig onafhankelijk releasen, externe open-source delen apart gepubliceerd worden, of security/IP-grenzen dat vereisen.

---

## Voorgestelde top-level structuur

```text
robot/
  README.md
  LICENSE
  .gitignore
  .editorconfig
  docs/
    architecture/
    decisions/
    protocols/
    bringup/
    operations/
  tools/
    ci/
    dev/
    flash/
    log/
  third_party/
  targets/
    esp32p4/
    orin/
  libs/
    cbase/
    protocol/
    math/
    ringbuf/
  services/
    mission/
    navigation/
    telemetry/
    supervisor/
  firmware/
    common/
    boards/
  tests/
    unit/
    integration/
    hil/
    fixtures/
  configs/
    robot/
    mission/
    lab/
  data/
    README.md
```

### Betekenis per map

- `docs/`: architectuur, ADR's, protocoldocumentatie, bring-up runbooks en operationele procedures.
- `tools/`: kleine host-tools voor CI, flashing, loganalyse, simulatie en developer workflows.
- `third_party/`: alleen gepinde of vendored code; elke dependency krijgt eigenaar, versie en licentie-notitie.
- `targets/esp32p4/`: targetspecifieke ESP32-P4 build, linkerconfiguratie, boardconfig en entrypoints.
- `targets/orin/`: Orin-specifieke packaging, systemd units, containerfiles, udev rules en deployment scripts.
- `libs/`: platformonafhankelijke low-level libraries met vaste buffers, tagged unions en expliciete ownershipregels.
- `services/`: Linux userspace processen voor missie, navigatie, telemetry en supervisorlogica.
- `firmware/`: firmwarecode die niet in `targets/` thuishoort, zoals board drivers en gedeelde HAL-achtige code.
- `tests/`: unit tests, integratietests, hardware-in-the-loop tests en vaste fixtures/logs.
- `configs/`: configuratie per robot, missie en lab; geen secrets committen.
- `data/`: alleen kleine voorbeelddata of pointers naar externe datasets; grote logs buiten Git bewaren.

---

## C/C++ module-indeling binnen `libs/` en `services/`

Gebruik platte, mechanisch transparante modules:

```text
libs/ringbuf/
  include/robot_ringbuf.h
  src/robot_ringbuf.c
  tests/ringbuf_test.c
  README.md

services/navigation/
  include/nav_state.h
  src/nav_state.c
  src/nav_main.c
  tests/nav_state_test.c
  README.md
```

Richtlijnen:

- Eén publieke header per module waar mogelijk.
- `include/` bevat alleen API-contracten; interne layout staat in `src/` als dat nodig is.
- Geen runtime-heap in hot paths; gebruik vaste buffers, arenas of caller-owned memory.
- Resultaten gebruiken tagged unions: `struct { enum tag; union { value; err; }; }`.
- Namen volgen `prefix_action_subject`, bijvoorbeeld `nav_update_state` en `net_sock_read`.
- Protocoltypes zijn plain-old-data waar mogelijk en hebben expliciete endian- en alignmentregels.

---

## Branches, PR's en eigenaarschap

Begin simpel:

- `main` is altijd groen en bevat alleen geïntegreerde code.
- Feature branches heten `area/topic`, bijvoorbeeld `nav/ekf-replay`, `fw/esp32p4-uart-dma` of `ci/orin-cross-build`.
- Elke PR noemt target-impact: `esp32p4`, `orin`, `mission`, `navigation`, `firmware`, `tests`, `docs`.
- Gebruik `CODEOWNERS` voor hardware- en safety-kritieke delen.
- Gebruik ADR's in `docs/decisions/` voor keuzes die later duur zijn om terug te draaien.

---

## CI-pijplijn in lagen

Start met snelle checks en voeg hardwarelagen later toe:

1. Formatting en static checks.
2. Host unit tests zonder hardware.
3. Cross-compile voor ESP32-P4 en Orin.
4. Simulator/log-replay tests.
5. Integratietests met fake devices of loopback.
6. Hardware-in-the-loop bring-up tests op gelabelde runners.
7. Release-artifact build en smoke test.

Elke laag moet expliciet aangeven welke targets geraakt worden. Hardwaretests mogen traag zijn, maar moeten reproduceerbare logs en artifacts bewaren.

---

## Uitvoeringsplan na jouw antwoorden

1. Kies definitief monorepo of multi-repo en leg dit vast in een ADR.
2. Maak de top-level mappen aan met korte `README.md` per map.
3. Voeg `.editorconfig`, `.gitignore`, `CODEOWNERS` en PR-template toe.
4. Definieer de eerste buildmatrix: host, ESP32-P4 en Orin.
5. Plaats bestaande code in `targets/`, `libs/`, `services/` en `tests/` zonder inhoudelijk te refactoren.
6. Maak protocol- en configuratiecontracten expliciet in `docs/protocols/` en `configs/`.
7. Voeg minimale CI toe: format check, host unit tests en cross-compile skeleton.
8. Voeg daarna integratie- en HIL-tests toe met vaste fixtures en duidelijke hardwarelabels.
9. Schrijf ADR's voor concurrency-model, IPC/protocolkeuzes, memory policy en release-artifacts.
10. Pas daarna code aan naar de zero-overhead C/C++ stijl, module voor module, met meetbare tests.

---

## Eerste aanbevolen keuze

Als er geen harde reden is voor meerdere repositories: start met een monorepo, maar behandel elk target en elke service als een zelfstandig product met eigen README, tests en build target. Dat geeft de minste frictie tijdens bring-up en integratie, terwijl de codegrenzen toch expliciet blijven.
