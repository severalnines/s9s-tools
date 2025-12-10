# Endpoint Correspondence Investigation - Quick Reference

## Overview

This directory contains the results of an investigation into whether s9s subcommands follow the principle of using one and only one corresponding v2 endpoint.

## Files

- **[endpoint-correspondence-investigation.md](endpoint-correspondence-investigation.md)** - Full detailed report with:
  - Methodology
  - Complete list of violations
  - Analysis and recommendations
  - Common violation patterns

- **[endpoint-correspondence-data.csv](endpoint-correspondence-data.csv)** - Machine-readable CSV format:
  - All subcommands and their methods
  - Endpoint mappings
  - Violation flags
  - Suitable for automated analysis

## Quick Summary

**Rule Being Tested:**
Each s9s subcommand should use only its corresponding v2 endpoint.
- `s9s cluster` → `/v2/clusters` only
- `s9s job` → `/v2/jobs` only  
- etc.

**Results:**
- ✅ **3 Compliant** (16%): job, alarm, report
- ❌ **16 Non-Compliant** (84%): All others

## Top Violators

| Subcommand | Non-Matching Endpoints | Major Issue |
|------------|----------------------|-------------|
| cluster | 3 | Uses /v2/jobs for 25+ operations |
| node | 3 | Uses /v2/jobs, /v2/host, /v2/config |
| server | 3 | Uses /v2/host, /v2/jobs, /v2/tree |
| controller | 3 | Uses /v2/config, /v2/host, /v2/jobs |
| backup | 1 | Uses /v2/jobs for 10+ operations |
| replication | 2 | Uses /v2/clusters, /v2/jobs |

## Common Pattern

**The /v2/jobs Pattern:**
Most violations occur because operations that create background jobs use the `/v2/jobs` endpoint regardless of which subcommand initiated them.

Examples:
- `s9s cluster --rolling-restart` → calls `/v2/jobs` endpoint
- `s9s backup --create` → calls `/v2/jobs` endpoint  
- `s9s node --restart` → calls `/v2/jobs` endpoint

This appears to be an architectural pattern where the backend is organized by **functional domain** (jobs, hosts, clusters) rather than by **CLI command structure**.

## Recommendations

1. **If API restructure is needed:** Create subcommand-specific endpoints that internally route to appropriate functional endpoints
2. **If current design is intentional:** Document the cross-endpoint patterns as part of the API design
3. **For new features:** Decide whether to continue functional organization or enforce CLI alignment

## Investigation Date

December 10, 2025

## Related Files

Source files analyzed:
- `libs9s/s9srpcclient.cpp` - RPC client with endpoint definitions
- `libs9s/s9sbusinesslogic.cpp` - Subcommand routing logic
