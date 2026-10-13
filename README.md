# gridot

[![CI](https://github.com/codersilicon/gridot/actions/workflows/ci.yml/badge.svg)](https://github.com/codersilicon/gridot/actions/workflows/ci.yml)
[![License](https://img.shields.io/badge/license-Apache--2.0-blue.svg)](LICENSE)

**GR**ido**T**: **G**et **R**id of **T**hat — find packages you no longer need.

`gridot` reads your system and reports the packages that are idle and gobbling up your
space. It never installs or removes anything: it is a reporter, not a package manager.

```console
$ gridot scan --min-size 20 --limit 5
```

## Features

- **Orphan detection** — a package is an orphan when no *installed* package pulls it in
  (respecting `Pre-Depends`, `Depends`, `Breaks`, `Replaces` and — because apt installs
  `Recommends` by default — `Recommends`).
- **Awareness** — `Essential: yes` packages and everything with `Priority: required` or
  `important` are never reported.
- **Auto-installed packages only** by default, since manual installs are intentional.
- **Size filtering** — sort and filter by installed size in MiB.
- **Tracked groups** — mark a known use case (e.g. `gaming`, `audio`) and `gridot` tells
  you which packages are `AT RISK` if any member disappears from apt.
- **Machine-readable output** via `--json` for scripting.
- **Read-only** — it reads your system state and your tracked groups, and never
  installs, removes or configures anything.

## Supported distributions

| Distribution | Status |
| --- | --- |
| Ubuntu 22.04 LTS, 24.04 LTS | CI builds and runs `gridot scan` on every push |
| Debian 12 (bookworm) | the container image builds on it |
| Debian 13 (trixie) | used for day-to-day development |
| Derivatives — Linux Mint, Pop!_OS, Raspberry Pi OS, Kali | expected to work |

More distributions are on the way. Package reading sits behind a `Backend`
interface, so another package manager can be added without touching the graph,
group or reporting layers — see [ARCHITECTURE.md](ARCHITECTURE.md#adding-a-backend).
Open an issue naming your distribution and we'll prioritise it.

## Build

```sh
# With presets (CMake >= 3.21, Ninja)
cmake --preset release
cmake --build --preset release
./build/release/gridot --help

# Or plain, out-of-source
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
```

Other presets: `debug`, `relwithdebinfo`, `asan` (ASan/UBSan), `coverage`.

## Install

```sh
sudo cmake --install build            # or: sudo make install
```

Or from a checkout with the wrapper Makefile (run `make` for the target list):

```sh
make build && sudo make install
```

## Usage

```console
gridot [scan] [options]        report orphaned packages by section
gridot track <name> <pkg>...   group a use case so its parts are tracked
gridot untrack <name>          stop tracking a group
gridot groups                  list tracked groups
```

### Scan options

| Option | Description |
| --- | --- |
| `-m, --min-size <MiB>` | only report orphans at least this large |
| `-n, --limit <N>` | show at most N orphans in the detail list |
| `-r, --root <pkg>` | treat pkg as a manual root (repeatable) |
| `--json` | machine-readable output |
| `--no-groups` | skip the tracked-groups section |
| `--no-recommends` | ignore `Recommends` edges (over-reports; apt installs them) |

### Tracked groups

```sh
gridot track gaming steam wine
gridot track -g gaming lutris
gridot groups
gridot untrack gaming
```

Members do not have to be installed. A group with a missing member is reported as
`BROKEN`, and its remaining members as `AT RISK` — the early warning that a future
package removal would break that use case.

### Exit codes

| Code | Meaning |
| --- | --- |
| `0` | success |
| `1` | runtime error (dpkg state unavailable, unreadable group store) |
| `2` | invalid usage |

## Documentation

- [USAGE.md](USAGE.md) — what counts as an orphan, groups, JSON output
- [ARCHITECTURE.md](ARCHITECTURE.md) — module map and design notes
- [CONTRIBUTING.md](CONTRIBUTING.md) — development workflow
- [SECURITY.md](SECURITY.md) — threat model and vulnerability reporting
- [CHANGELOG.md](CHANGELOG.md) — release history
- `man gridot` — the manual page

## License

Apache License 2.0 — see [LICENSE](LICENSE).
