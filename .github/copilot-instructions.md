# s9s-tools - Severalnines Cluster Management CLI

s9s-tools is a C++ command-line interface for managing and monitoring Severalnines ClusterControl database clusters. The repository contains the s9s CLI tool and supporting libs9s library.

Always reference these instructions first and fallback to search or bash commands only when you encounter unexpected information that does not match the info here.

## Working Effectively

Bootstrap, build, and test the repository:
- Install dependencies: `sudo apt-get update && sudo apt-get install -y libssl-dev flex bison build-essential autotools-dev autoconf automake libtool clang-format`
- Setup build system: `./autogen.sh` -- takes ~7 seconds. Creates configure script and Makefiles.
- Build project: `make -j$(nproc)` -- takes ~75 seconds (1 min 15 seconds). NEVER CANCEL. Set timeout to 180+ seconds.
- Run unit tests: `./tests/runalltests.sh` -- takes <1 second. One test (ut_s9srpcclient) fails but this is an existing issue.
- Code formatting: `./scripts/apply-format --staged` to format staged changes, or `./scripts/apply-format --whole-file [files]` for complete files.

## Validation

After making changes, ALWAYS run these validation steps:
- Build validation: `make -j$(nproc)` -- NEVER CANCEL. Set timeout to 180+ seconds.
- Unit test validation: `./tests/runalltests.sh` -- expect 1 failing test (ut_s9srpcclient), this is normal.
- Format validation: `./scripts/apply-format --staged` to check code formatting.
- CLI smoke test: `./s9s/s9s --version && ./s9s/s9s --help` to verify CLI works.

## Understanding the Application

The s9s CLI tool is designed to connect to Severalnines ClusterControl controllers to manage database clusters (MySQL, PostgreSQL, MongoDB, etc.). Key characteristics:
- **Requires ClusterControl**: Most functionality requires connection to a ClusterControl controller (`--controller` option).
- **Configuration needed**: Expects user credentials and controller URL, typically in `~/.s9s/s9s.conf`.
- **Limited standalone mode**: Basic commands like `--version`, `--help` work without a controller.
- **Test environment limitations**: Full functionality testing requires actual database clusters and ClusterControl setup.

Basic CLI usage patterns:
```bash
# Basic information (works without controller)
./s9s/s9s --version
./s9s/s9s --help
./s9s/s9s cluster --help

# Typical usage (requires controller)
./s9s/s9s cluster --list --controller=https://controller:9501
./s9s/s9s node --stat --controller=https://controller:9501
```

## Build Process Details

The project uses autotools build system:
1. **autogen.sh**: Runs `autoreconf -vi` to generate configure script from configure.ac
2. **configure**: Generated script that checks dependencies and creates Makefiles
3. **make**: Builds libs9s library first, then s9s CLI, then test executables
4. **Flex/Bison**: Generates parsers for JSON and config files during build

CRITICAL timing expectations:
- **autogen.sh**: ~7 seconds
- **make build**: ~75 seconds (1 min 15 seconds) - NEVER CANCEL builds
- **unit tests**: <1 second
- **ALWAYS set timeouts of 180+ seconds for make commands**

## Common Tasks

### Repository Structure
```
s9s-tools/
├── libs9s/           # Core library (S9sString, S9sCluster, etc.)
├── s9s/              # CLI application main.cpp
├── tests/            # Unit tests (ut_*) and functional tests (ft_*.sh)
├── doc/              # Manual pages
├── scripts/          # Code formatting and utilities
├── autogen.sh        # Build system setup
├── configure.ac      # Autotools configuration
└── Makefile.am       # Automake configuration
```

### Key Files to Know
- `libs9s/`: Contains all core functionality classes
- `s9s/main.cpp`: CLI entry point and command routing
- `tests/ut_*/`: Unit tests for specific components
- `tests/ft_*.sh`: Functional tests requiring clusters
- `scripts/apply-format`: Code formatting using clang-format
- `.clang-format`: Code style configuration

### Development Workflow
1. Make changes to code
2. Run `./scripts/apply-format --whole-file [files]` for formatting
3. Build: `make -j$(nproc)` (timeout 180+ seconds)
4. Test: `./tests/runalltests.sh`
5. Verify CLI: `./s9s/s9s --version`

### Debugging and Testing
- Individual unit tests: `./tests/ut_s9sstring/ut_s9sstring`
- Library components are in `libs9s/` with corresponding tests in `tests/ut_*/`
- Functional tests in `tests/ft_*.sh` require actual database infrastructure
- Use `--print-json` flag with s9s commands for debugging communication

### Code Organization
- **libs9s/s9scluster.***: Cluster management functionality
- **libs9s/s9snode.***: Node/server management
- **libs9s/s9suser.***: User management
- **libs9s/s9sbackup.***: Backup operations
- **libs9s/s9srpclient.***: Communication with ClusterControl
- **libs9s/s9sstring.***: String utilities and operations

CRITICAL REMINDERS:
- **NEVER CANCEL** build commands - they may take 75+ seconds
- **ALWAYS** set build timeouts to 180+ seconds minimum
- **One unit test failure** (ut_s9srpcclient) is expected and normal
- **Controller connection required** for most CLI functionality testing
- **Format code** before committing using `./scripts/apply-format`