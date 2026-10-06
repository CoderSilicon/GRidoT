# gridot

**GR**ido**T**: **G**et **R**id of **T**hat — findpackages you no longer need.

`gridot` reads your system and finds the pkgs that are idle and gobbling up your space. It never installs or removes anything: it is just a reporter, not a
package manager.

```
$ gridot scan --min-size 20 --limit 5
```

## Features

- **Orphan** — a package is an orphan when no *installed* package pulls it in
  (respecting `Pre-Depends`, `Depends`, `Breaks`, `Replaces`, and — because apt installs
  `Recommends` by default — `Recommends`).
- **Awareness** — `Essential: yes` packages and everything with
  `Priority: required` / `important` are never reported.
- **Auto-installed packages only** by default, since manual installs are intentional.
- **Size filtering** — sort and filter by installed size in MiB.
- **Tracked groups** — mark a known use case (e.g. `gaming`, `audio`) and `gridot` tells
  you which packages are `AT RISK` if any member disappears from apt.
- **Machine-readable output** via `--json` for scripting.
- Read-only: it shells out to `dpkg-query` and reads the tracked-groups file; it never
  mutates system state.

## Build

```sh
# One-off, out-of-source
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j

# Or with presets (CMake >= 3.21, Ninja)
cmake --preset release
cmake --build --preset release
./build/release/gridot --help
```

Other presets: `debug`, `relwithdebinfo`, `asan` (ASan/UBSan), `coverage`.

## Install

```sh
sudo cmake --install build            # or: sudo make install
```

Or build straight from a checkout:

```sh
make && sudo make install PREFIX=/usr/local
```

## Usage

```
gridot [scan] [options]        report orphaned packages by section
gridot track <name> <pkg>...   group a use case so its parts are tracked
gridot untrack <name>         stop tracking a group
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

## Exit codes

| Code | Meaning |
| --- | --- |
| `0` | success |
| `1` | runtime error (dpkg state unavailable, unreadable group store) |
| `2` | invalid usage |

## Development

```sh
make format      # clang-format + cmake-format
make lint        # clang-tidy
make debug       # debug build via presets
make clean
```

See [CONTRIBUTING.md](CONTRIBUTING.md) and [ARCHITECTURE.md](docs/ARCHITECTURE.md).

## License

Apache License 2.0 — see [LICENSE](LICENSE).
