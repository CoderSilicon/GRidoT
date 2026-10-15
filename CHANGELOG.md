# Changelog

All notable changes to this project are documented here. The format follows
[Keep a Changelog](https://keepachangelog.com/en/1.1.0/) and this project adheres to
[Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

### Added

- `RpmBackend`: support for dnf/yum/rpm (Fedora, RHEL, CentOS). `gridot` now detects the
  package manager automatically (apt first, then dnf/yum) and uses the matching backend.
  Install reasons come from `dnf repoquery` (or the yumdb as a fallback), file-based
  requires like `/usr/bin/gpg2` are resolved via each package's file list, and `dnf`,
  kernel and `protected.d` packages are never reported.

### Fixed

- Group store path documented as `$XDG_CONFIG_HOME/gridot/groups.tsv` (it was previously
  described as an `XDG_DATA_HOME` file that the code never wrote).
- Repository links now all point at `github.com/codersilicon/gridot`.
- The container runtime stage installs `apt` explicitly instead of relying on the base
  image providing it; `gridot scan` needs it to read auto-install state.
- `.clang-format` and `.cmake-format.yaml` were rejected by clang-format 18 and
  cmake-format 0.6.13, which made the formatting CI job fail; both configs are now valid
  and the tree is format-clean.
- The formatting CI job referenced two GitHub Actions that do not exist.
- The container image build failed on Debian bookworm's gcc 12.2 with a false-positive
  `-Werror=restrict` reported inside `std::string` concatenation; the diagnostic is now
  suppressed for GCC < 13 only, leaving gcc 13+ untouched.

### Changed

- README gained a *Supported package managers* section (apt/dpkg supported,
  other package managers not yet), with the distros it works on listed below it.
- Distro-specific wording dropped from the project tagline (`CMakeLists.txt`,
  `man/gridot.1`, OCI image label) and from the README feature list.
- Docker image label now declares `Apache-2.0`, matching `LICENSE`.
- `CMakePresets.json` gained `relwithdebinfo` and `coverage` build presets.

## [0.1.0] - 2026-10-06

### Added

- `gridot scan` — report packages that are installed, auto-installed and no longer
  depended on by anything, grouped by dpkg section.
- `-m/--min-size` filter and `-n/--limit` cap for the orphan detail list.
- `-r/--root` to treat manually installed packages as roots.
- `--json` output for scripting.
- `--no-recommends` to ignore `Recommends` edges (over-reports by design).
- `gridot track` / `gridot track -g` / `gridot untrack` / `gridot groups` for
  use-case tracking, with `OK`, `AT RISK` and `BROKEN` group status.
- `--no-groups` to skip the tracked-groups section.
- CMake presets for debug, release, ASan/UBSan and coverage builds.
- Man page (`man/gridot.1`) and README.

### Notes

- `Recommends` edges are honoured by default because
  `APT::Install-Recommends` is `true` on Debian and Ubuntu; ignoring them
  over-reports heavily.
- gridot is read-only. It shells out to `dpkg-query` and never installs or removes
  anything.

[Unreleased]: https://github.com/codersilicon/gridot/compare/v0.1.0...HEAD
[0.1.0]: https://github.com/codersilicon/gridot/releases/tag/v0.1.0
