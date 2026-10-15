# gridot

[![CI](https://github.com/codersilicon/gridot/actions/workflows/ci.yml/badge.svg)](https://github.com/codersilicon/gridot/actions/workflows/ci.yml)
[![CodeQL](https://github.com/codersilicon/gridot/actions/workflows/codeql.yml/badge.svg)](https://github.com/codersilicon/gridot/actions/workflows/codeql.yml)
[![License](https://img.shields.io/badge/license-Apache--2.0-blue.svg)](LICENSE)
[![C++20](https://img.shields.io/badge/C%2B%2B-20-00599C?logo=cplusplus&logoColor=white)](https://isocpp.org/)
[![CMake](https://img.shields.io/badge/CMake-%E2%89%A5%203.16-064F8C?logo=cmake&logoColor=white)](https://cmake.org/)
[![Platform](https://img.shields.io/badge/platform-Linux-lightgrey?logo=linux&logoColor=black)](#supported-package-managers)
[![Docs](https://img.shields.io/badge/docs-man%20page-blue.svg)](#documentation)

**GR**ido**T** — **G**et **R**id **o**f **T**hat.

Your Linux box quietly fills up with packages that nothing needs anymore. You
installed a game once, and its libraries are still sitting there — taking space,
used by nothing, in no menu. `apt autoremove` will delete them for you, but it
won't say what it took, it won't ask first, and it has no idea that half of your
"gaming" setup is now hanging by a thread.

`gridot` answers two questions, then stops:

- **What is safe to drop?** — packages that are installed, auto-installed, and
  no longer pulled in by anything else.
- **What would I miss?** — for a use case you care about (`gaming`, `audio`,
  whatever you define), which of its packages are at risk if one member goes away.

It only reads. Nothing is installed, removed or configured.

```console
$ gridot scan --min-size 20 --limit 5
```

## Features

- **Orphan detection** — a package is an orphan when no *installed* package pulls it in
  (on apt: `Pre-Depends`, `Depends`, `Recommends`; on dnf: `Requires`, `Recommends` plus
  file-based requires like `/usr/bin/gpg2`).
- **Awareness** — `Essential: yes` packages and everything with `Priority: required` or
  `important` are never reported.
- **Auto-installed packages only** by default, since manual installs are intentional.
- **Size filtering** — sort and filter by installed size in MiB.
- **Tracked groups** — mark a known use case (e.g. `gaming`, `audio`) and `gridot` tells
  you which packages are `AT RISK` if any member disappears from apt.
- **Machine-readable output** via `--json` for scripting.
- **Read-only** — it reads your system state and your tracked groups, and never
  installs, removes or configures anything.

## Supported package managers

gridot sits on top of a `Backend` that reads the package manager's database —
the dependency graph, the auto-install flags and (on apt) the install history
all come from there.

| Package manager | Status |
| --- | --- |
| apt / dpkg (Debian, Ubuntu) | Supported |
| dnf / yum / rpm (Fedora, RHEL, CentOS) | Supported(Experimental) |
| emerge (Gentoo) | Not supported |
| pacman (Arch) | Not supported |
| zypper (openSUSE), apk (Alpine), nix | Not supported |

`Backend` sits between the package manager and the graph, group and reporting
layers, so another package manager can be added without touching the rest —
see [ARCHITECTURE.md](ARCHITECTURE.md#adding-a-backend). Open an issue naming
the package manager you want and we'll prioritise it.

**Works on** Debian and Ubuntu, plus derivatives that share their apt/dpkg
stack: Linux Mint, Pop!_OS, Raspberry Pi OS and Kali — and on Fedora, RHEL,
CentOS and their derivatives via dnf/yum. CI builds against Ubuntu 22.04/24.04
and Fedora; development happens on Debian 13.

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
