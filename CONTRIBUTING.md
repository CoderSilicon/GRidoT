# Contributing

Thanks for looking at `gridot`.

## Getting set up

```sh
sudo apt install build-essential cmake ninja-build clang-format clang-tidy
git clone https://github.com/gridot/gridot
cd gridot
make debug
```

## Ground rules

- **Read-only.** `gridot` must never install or remove packages. If a change would make it
  mutate system state, it does not belong here.
- **No new runtime dependencies** unless there is a strong reason; the tool is meant to
  work on a machine that has nothing but `dpkg` and a C++20 compiler.
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

Short imperative subject, ≤ 72 characters, no trailing period:

```
Add --limit cap to the orphan detail list
```

## Reporting a bug

Include:

- `gridot --version`-ish output: distro (`cat /etc/os-release`) and `dpkg-query --version`
- the exact command you ran
- what you expected and what happened
- `gridot scan --json` output if it is safe to share (it contains package names only)

## Reporting a false positive

A false positive is a bug. Include the output of:

```sh
gridot scan --json | jq '.orphans[] | select(.name == "<package>")'
dpkg-query -W -f='${Package} ${Priority} ${Auto-Installed}\n' <package>
```

That usually tells us whether the package is being kept alive by a `Recommends` edge or by a
tracked group that has gone stale.