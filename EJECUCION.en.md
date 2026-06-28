# Execution Guide

This project can run in two main operational styles:

- Interactive console mode.
- GUI mode:
  Windows native GUI, macOS native GUI, or portable terminal GUI on Linux.

## Select Runtime Language

Spanish is the default language. To run the project in English:

```bash
./progvoraz --lang en
```

or:

```bash
PROGVORAZ_LANG=en ./progvoraz
```

## Console Modes

- `a`: unlimited coins
- `b`: limited stock
- `c`: stock administration
- `h`: history
- `r`: currency summary
- `s`: stock snapshot
- `u`: restore snapshot
- `g`: global inventory report
- `j`: JSON stock export
- `x`: currency conversion

The command `gui` can be typed from interactive console flows to launch the graphical interface without leaving the current session.

## English Currency Aliases

The project keeps the original Spanish internal currency keys, but English aliases are accepted in runtime input:

- `euro`
- `dollar`, `us_dollar`
- `yen`, `japanese_yen`
- `pound`, `british_pound`
- `swiss_franc`
- `canadian_dollar`
- `australian_dollar`
- `chinese_yuan`
- `mexican_peso`

## Docker

Build:

```bash
docker compose build
```

Run console:

```bash
docker compose run --rm progvoraz-console
```

Run portable GUI:

```bash
docker compose run --rm progvoraz-gui-portable
```

Run automated tests:

```bash
docker compose run --rm progvoraz-test
```
