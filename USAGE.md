# Usage

## Scanning

```sh
gridot                       # same as `gridot scan`
gridot scan
gridot scan -m 50            # only orphans >= 50 MiB
gridot scan -n 20            # cap the detail list at 20 rows
gridot scan -r ffmpeg        # treat a manually installed package as a root
gridot scan --json           # machine-readable
gridot scan --no-groups      # skip the tracked-groups section
```

### What counts as an orphan

A package is reported when **all** of the following hold:

1. It is installed.
2. It is not protected: on apt that means not `Essential: yes` and not
   `Priority: required`/`important`; on dnf/yum that means not `dnf`, not a
   kernel/installonly package, and not listed in `protected.d`.
3. It was auto-installed (`Auto-Installed: 1` on apt, install reason
   `Dependency`/`WeakDependency` on dnf).
4. No installed package pulls it in. On apt that means `Pre-Depends`, `Depends`
   or (unless `--no-recommends`) `Recommends`. On dnf/yum that means `Requires`,
   `Recommends` (unless `--no-recommends`) and file-based requires such as
   `/usr/bin/gpg2`.
5. It is not a member of a tracked group.

Because auto-installed packages that nothing needs are exactly the residue of
removed dependencies, condition 3 keeps your deliberate installs out of the
report.

## Tracked groups

A tracked group is a named set of packages that together serve one use case.

```sh
gridot track audio pulseaudio pipewire wireplumber
gridot track -g audio libspa-0.2-0
gridot groups
gridot untrack audio
```

Group state lives in `$XDG_CONFIG_HOME/gridot/groups.tsv` (default
`~/.config/gridot/groups.tsv`).

Each group reports one of:

| Status | Meaning |
| --- | --- |
| `OK` | every member is present and accounted for |
| `AT RISK` | the group is intact but one or more members are only present because something else needs them — removing them could break the use case |
| `BROKEN` | at least one member is missing from apt |

`AT RISK` is the interesting one: it is a warning that a future removal will silently
break the group.

## Exit codes

| Code | Meaning |
| --- | --- |
| `0` | success |
| `1` | runtime error |
| `2` | invalid usage |

```sh
gridot scan --json | jq '.orphans | length'
```