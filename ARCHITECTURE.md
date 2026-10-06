# Architecture

```
              ┌────────────┐
   argv ─────▶│   main.cpp │  option parsing, command dispatch
              └─────┬──────┘
                    │
        ┌───────────┴────────────┐
        ▼                        ▼
┌────────────────┐      ┌──────────────────┐
│  Backend       │      │ GroupStore       │  ~/.local/share/gridot/groups
│ (interface)    │      │ (groups.cpp)     │
└───────┬────────┘      └────────┬─────────┘
        │                       │
        │ AptBackend            │ mark which installed packages are members
        │ (core/apt_backend.cpp)│
        ▼                       │
┌────────────────┐               │
│  Graph         │◀──────────────┘
│  (graph.cpp)   │  reachability from roots; orphan + size ranking
└───────┬────────┘
        │
        ├──▶ sections.cpp   group orphans by dpkg section
        ▼
   report.cpp     human table  /  --json
```

## Modules

| File | Responsibility |
| --- | --- |
| `src/main.cpp` | CLI parsing (`Options`), usage text, command dispatch, exit codes |
| `src/include/backend.h` | `Pkg` record and the `Backend` interface (source of package state) |
| `src/core/backend.cpp` | shared backend helpers and field normalisation |
| `src/core/apt_backend.cpp` | real backend: parses `dpkg-query` output into `Pkg` records |
| `src/include/graph.h`, `src/graph.cpp` | protection rules, relation extraction, root reachability |
| `src/include/groups.h`, `src/groups.cpp` | tracked-group persistence and validation |
| `src/include/sections.h`, `src/sections.cpp` | orphan bucketing by dpkg section |
| `src/include/report.h`, `src/report.cpp` | text and JSON rendering |
| `src/include/util.h`, `src/util.cpp` | string, size and file helpers |

## Design notes

**One pass, no dpkg re-entry.** `AptBackend` materialises the full package table once.
`graph.cpp` then works on an in-memory map, so repeated questions (orphan search, group
lookup, size filter) cost no subprocess calls.

**Reachability, not reverse-dependency counting.** A package is kept alive if any
installed package reaches it. Counting reverse edges would misclassify packages that are
only reachable through a chain that itself is orphaned.

**`Recommends` defaults on.** `APT::Install-Recommends` is `true` on Debian and Ubuntu, so
a `Recommends` edge really does keep a package installed in practice. Ignoring those edges
loudly over-reports, which is why `--no-recommends` is opt-in and documented as such.

**Read-only by construction.** `Backend` exposes no mutating operation, and `gridot` only
ever shells out to `dpkg-query`. There is no code path that installs or removes anything,
including the tracked-group store.

## Adding a backend

Implement `Backend` (see `src/include/backend.h`), then add a thin factory in
`src/core/`. The graph, groups, sections and report layers are backend-agnostic and need no
changes.