# Contributing
Thanks for looking at `GRidoT`. If you want to lend a hand in this simple but useful project, Here are few following guidelines and protocols which might strictly be followed:

## Getting set up

```sh
sudo apt install build-essential cmake ninja-build clang-format clang-tidy
git clone https://github.com/codersilicon/gridot
cd gridot
make debug
```

## Ground rules

- **Read-only.** `gridot` must never install or remove packages. If a change would make it
  mutate system state, it violates the protocol and does not belong here.
- **No new runtime dependencies** unless there is a strong reason; the tool is meant to
  work on any Debian/Ubuntu machine that has `dpkg`, `apt` and a C++20 compiler.
- **Follow the existing style.** `make format` applies `clang-format` (`.clang-format`,
  Allman braces, 2-space indent) and `cmake-format`. A CI job checks that the tree is clean.
- **Warnings are errors in CI** (`-DGRIDOT_ENABLE_WERROR=ON`).

## Before opening a pull request

```sh
make format
make lint
make debug
make asan        # AddressSanitizer + UndefinedBehaviorSanitizer
```

## Commit messages

Type of Commit with Short imperative subject, ≤ 72 characters, no trailing period:

```
Add --limit cap to the orphan detail list
```

## Reporting a bug

Include:

- `gridot --version` output, your distro (`cat /etc/os-release`) and `dpkg-query --version`
- the exact command you ran
- what you expected and what happened
- `gridot scan --json` output if it is safe to share (it contains package names only)

## Reporting a false positive

A false positive is a bug. Include the output of:

```sh
gridot scan --json | jq '.orphans[] | select(.name == "<package>")'
dpkg-query -W -f='${Package} ${Priority} ${Essential}\n' <package>
grep -A1 '^Package: <package>$' /var/lib/apt/extended_states
```

The first command shows whether the package is kept alive by a `Recommends` edge or by a
tracked group that has gone stale; the last one shows apt's auto-install flag, which is
where `gridot` reads it from.
