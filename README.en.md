# ProgVoraz

Currency-change project written in C, originally created in 2014 and modernized for a current GitHub-based workflow.

## Features

- Computes change with unlimited coins.
- Computes change with limited stock.
- Supports coin-count restrictions in all main modes: `N` (maximum), `=N` (exact), `N-M` (range).
- Persists stock updates to `stock.txt`.
- Records and displays transaction history from `historial.txt`.
- Includes a native GUI on Windows, a native Swift/AppKit GUI on macOS, and a portable terminal GUI for Linux.
- Supports Spanish and English runtime modes through `--lang es|en` or `PROGVORAZ_LANG`.

## Language Support

- Spanish mode remains the default.
- English mode can be enabled with:

```bash
./progvoraz --lang en
```

or:

```bash
PROGVORAZ_LANG=en ./progvoraz
```

The runtime now accepts English currency aliases such as `dollar`, `pound`, `swiss_franc`, `canadian_dollar`, `australian_dollar`, `chinese_yuan`, and `mexican_peso`, while still preserving the original Spanish names and commands.

## Build

```bash
make debug
make run
```

Release build:

```bash
make release
```

Tests:

```bash
make test
```

GUI:

```bash
make gui
make run-gui
```

## Docker

```bash
docker compose build
docker compose run --rm progvoraz-console
docker compose run --rm progvoraz-gui-portable
docker compose run --rm progvoraz-test
```

## Relevant Files

- `main.c`: CLI entry point and non-interactive operations.
- `app_console.c`: interactive console application.
- `gui_portable.c`: portable terminal GUI.
- `gui_window.c`: native Windows GUI.
- `gui_macos.swift`: native macOS GUI.
- `progvoraz_locale.c`: shared language and command/currency alias support.
- `moneda_gestion.c`: currency and stock persistence layer.

## Additional Docs

- Spanish usage guide: [EJECUCION.md](/C:/Users/rodri/OneDrive/Documentos/GitHub/ProgVoraz/ProgVoraz/EJECUCION.md)
- Spanish technical reference: [FUNCIONES_C.md](/C:/Users/rodri/OneDrive/Documentos/GitHub/ProgVoraz/ProgVoraz/FUNCIONES_C.md)

