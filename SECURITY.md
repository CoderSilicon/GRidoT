# Security Policy

## Supported versions

| Version | Supported |
| --- | --- |
| 0.1.x | yes |

## Reporting a vulnerability

Report privately to the maintainers via GitHub's
[private vulnerability reporting](https://github.com/gridot/gridot/security/advisories/new).
Please do not open a public issue for a security problem.

Include: affected version, how to reproduce, and the impact you observed.

## Threat model

`gridot` is a **read-only** reporting tool. That is the main security property it
maintains.

- It shells out to `dpkg-query` and reads the dpkg status database. It contains no code
  path that installs, removes, upgrades or configures packages.
- The only file it writes is the tracked-group store at
  `$XDG_DATA_HOME/gridot/groups` (default `~/.local/share/gridot/groups`).
- It does not open a network connection.
- Package names and relationship fields come from dpkg and are used as map keys and
  output strings, never as shell input.

If you find a path where `gridot` mutates system state, that is a security bug — please
report it.