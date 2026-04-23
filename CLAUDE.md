# s9s-tools

C++11 CLI (`s9s`) that talks to the ClusterControl controller (`cmon`) over JSON-RPC.

## Architecture

- `libs9s/` — all logic: business logic, RPC client, models, CLI parsing, terminal UI. Built as `libs9s.a` / `libs9s.so`.
- `s9s/` — thin CLI entry (`main.cpp`), wires options to business logic.
- `tests/` — unit tests (`ut_*`) and functional tests (`ft_*.sh`).
- Key classes (all `S9s`-prefixed): `S9sOptions` (singleton, holds every CLI option), `S9sBusinessLogic`, `S9sRpcClient`, domain models (`S9sNode`, `S9sCluster`, `S9sBackup`, ...), terminal UI (`S9sMonitor`, `S9sTopUi`).
- Flex (`*.l`) + Bison (`*.y`) generate the JSON and config parsers; generated `.cpp` is checked in.
- Parallel class hierarchy with `clustercontrol-enterprise` (`S9sNode` vs `CmonHost`) — RPC contract changes usually require matching changes there.

## Tests

```bash
tests/runalltests.sh                        # all unit tests
tests/ut_s9sstring/ut_s9sstring             # single unit test (each is a standalone binary)
tests/ft_galera.sh                          # functional test (requires a running controller)
```

**Any behaviour change must be validated against the repo's unit tests (and functional tests when relevant) before it's considered done.**

## Everything else

Build instructions, install, conventions, related repos — see `README.md`.

## Claude Code notes

- `libs9s/s9soptions.cpp` is the largest file in the repo (~564KB). Prefer
  `Grep` over full `Read`; read only the ranges you need.
- Flex/Bison generated `.cpp` files in `libs9s/` are checked in — regenerate
  only when editing the `.l` / `.y` sources.
- Functional tests (`tests/ft_*.sh`) require a running controller. Do not run
  them unless the user explicitly asks.
