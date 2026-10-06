# Changelog

All notable changes to this project are documented here. The format follows
[Keep a Changelog](https://keepachangelog.com/en/1.1.0/) and this project adheres to
[Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

### Added

- Nothing yet.

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

[Unreleased]: https://github.com/gridot/gridot/compare/v0.1.0...HEAD
[0.1.0]: https://github.com/gridot/gridot/releases/tag/v0.1.0