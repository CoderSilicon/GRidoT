# Security

## Supported versions

| Version | Supported |
| --- | --- |
| 0.1.x | yes |

## Reporting a vulnerability

You are allowed to report privately to the maintainer via GitHub's
[private vulnerability reporting](https://github.com/codersilicon/gridot/security/advisories/new).
but Please open a public issue for a security problem. then only you might be able to submit an PR.

Include: affected version, how to reproduce, and the impact you observed.

## App model and it's threat

`gridot` is a **read-only** reporting tool. That is the *main security property* it
maintains.

- It shells out to `dpkg-query` and `apt-mark`, and reads
  `/var/lib/dpkg/status`, `/var/lib/apt/extended_states` and
  `/var/log/apt/history.log`. It contains no code path that *installs, removes,
  upgrades or configures* packages.
- The only file it writes is the tracked-group store at
  `$XDG_CONFIG_HOME/gridot/groups.tsv` (default `~/.config/gridot/groups.tsv`) which doesn't affect system in *any possible way*.
- It does *not open* a network connection or request any data outside the localhost and it's pkg manager.
- Package names and relationship fields are used as map keys and output strings,
  never as shell input: every command it runs is a *fixed literal*.

If you find a path where `gridot` mutates system state, that is a security bug — please
report it.
