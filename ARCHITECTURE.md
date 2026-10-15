# Architecture

```
              ┌────────────┐
   argv ─────▶│   main.cpp │  option parsing, command dispatch
              └─────┬──────┘
                    │
        ┌───────────┴────────────┐
        ▼                        ▼
┌────────────────┐      ┌──────────────────┐
│  Backend       │      │ GroupStore       │  ~/.config/gridot/groups.tsv
│ (interface)    │      │ (groups.cpp)     │
└───────┬────────┘      └────────┬─────────┘
        │                       │ mark which installed packages are members
        │                       │
        │ AptBackend / RpmBackend
        │ (core/apt_backend.cpp, core/rpm_backend.cpp)
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
| `src/core/apt_backend.cpp` | apt backend: parses `dpkg-query` output into `Pkg` records |
| `src/core/rpm_backend.cpp` | dnf/yum backend: parses `rpm -qa` and dnf install reasons |
| `src/include/graph.h`, `src/graph.cpp` | protection rules, relation extraction, root reachability |
| `src/include/groups.h`, `src/groups.cpp` | tracked-group persistence and validation |
| `src/include/sections.h`, `src/sections.cpp` | orphan bucketing by dpkg section |
| `src/include/report.h`, `src/report.cpp` | text and JSON rendering |
| `src/include/util.h`, `src/util.cpp` | string, size and file helpers |

## Design notes

**One pass, no re-entry.** Each backend materialises the full package table once.
`graph.cpp` then works on an in-memory map, so repeated questions (orphan search, group
lookup, size filter) cost no subprocess calls.

**Reachability, not reverse-dependency counting.** A package is kept alive if any
installed package reaches it. Counting reverse edges would misclassify packages that are
only reachable through a chain that itself is orphaned.

**`Recommends` defaults on.** apt installs `Recommends` by default, and dnf installs
`Recommends` too, so a `Recommends` edge really does keep a package installed in
practice. Ignoring those edges loudly over-reports, which is why `--no-recommends` is
opt-in and documented as such.

**File-based requires.** RPM packages can require a file path (e.g. `/usr/bin/gpg2`)
rather than a package name. The rpm backend loads each package's file list so the graph
can resolve these to the owning package, matching how dnf resolves them.

**Read-only by construction.** `Backend` exposes no mutating operation, and `gridot` only
ever shells out to read commands (`dpkg-query`, `apt-mark`, `rpm`, `dnf`). There is no
code path that installs or removes anything, including the tracked-group store.

## Adding a backend

Implement `Backend` (see `src/include/backend.h`), then add a thin factory in
`src/core/`. The graph, groups, sections and report layers are backend-agnostic and need no
changes.