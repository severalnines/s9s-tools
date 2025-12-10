# s9s Subcommand to v2 Endpoint Correspondence Investigation

## Overview

This document reports the findings of an investigation into whether s9s subcommands follow the rule of using one and only one corresponding v2 endpoint for their operations.

**Investigation Date:** 2025-12-10

## Expected Rule

Each s9s subcommand should use only its corresponding v2 endpoint:
- `s9s cluster` should use only `/v2/cluster` or `/v2/clusters`
- `s9s job` should use only `/v2/job` or `/v2/jobs`
- `s9s node` should use only `/v2/node` or `/v2/nodes`
- etc.

## Methodology

The investigation was conducted by:
1. Parsing `libs9s/s9srpcclient.cpp` to map RPC client methods to their v2 endpoints
2. Parsing `libs9s/s9sbusinesslogic.cpp` to map s9s subcommands to RPC client methods
3. Analyzing which endpoints each subcommand ultimately uses
4. Identifying violations where subcommands use non-matching endpoints

## Summary of Findings

**Total Subcommands Analyzed:** 19

**Compliant Subcommands (3):**
- ✅ `s9s job` - Uses only `/v2/jobs`
- ✅ `s9s alarm` - Uses only `/v2/alarm`
- ✅ `s9s report` - Uses only `/v2/reports`

**Non-Compliant Subcommands (16):**
- ❌ `s9s account` - Uses 1 non-matching endpoint
- ❌ `s9s backup` - Uses 1 non-matching endpoint
- ❌ `s9s cluster` - Uses 3 non-matching endpoints
- ❌ `s9s container` - Uses 2 non-matching endpoints
- ❌ `s9s controller` - Uses 3 non-matching endpoints
- ❌ `s9s group` - Uses 1 non-matching endpoint
- ❌ `s9s maintenance` - Uses 1 non-matching endpoint
- ❌ `s9s metatype` - Uses 1 non-matching endpoint
- ❌ `s9s node` - Uses 3 non-matching endpoints
- ❌ `s9s process` - Uses 1 non-matching endpoint
- ❌ `s9s replication` - Uses 2 non-matching endpoints
- ❌ `s9s script` - Uses 1 non-matching endpoint
- ❌ `s9s server` - Uses 3 non-matching endpoints
- ❌ `s9s sheet` - Uses 1 non-matching endpoint
- ❌ `s9s tree` - Uses 1 non-matching endpoint
- ❌ `s9s user` - Uses 1 non-matching endpoint

## Detailed Violations

### s9s account

**Expected:** `/v2/account` or `/v2/accounts`

**Violations:**
- Uses `/v2/clusters` for:
  - `createAccount()`
  - `deleteAccount()`
  - `grantPrivileges()`
  - `revokePrivileges()`

### s9s backup

**Expected:** `/v2/backup` or `/v2/backups`

**Correctly uses:** `/v2/backup`

**Violations:**
- Uses `/v2/jobs` for:
  - `createBackup()`
  - `deleteAllBackups()`
  - `deleteBackupSchedule()`
  - `deleteOldBackups()`
  - `restoreBackup()`
  - `restoreCluster()`
  - `restoreController()`
  - `saveCluster()`
  - `saveController()`
  - `verifyBackup()`

### s9s cluster

**Expected:** `/v2/cluster` or `/v2/clusters`

**Correctly uses:** `/v2/clusters`

**Violations:**
- Uses `/v2/discovery` for:
  - `checkHosts()`

- Uses `/v2/jobs` for:
  - `checkPkgUpgrades()`
  - `collectLogs()`
  - `createDeleteDatabaseJob()`
  - `createReport()`
  - `demoteNode()`
  - `deployAgents()`
  - `deployCmonAgents()`
  - `disableRecoveryWithJob()`
  - `disableSsl()`
  - `dropCluster()`
  - `enableRecoveryWithJob()`
  - `enableSsl()`
  - `importConfig()`
  - `promoteSlave()`
  - `removeNode()`
  - `renewCert()`
  - `rollingRestart()`
  - `setClusterReadOnly()`
  - `setupAuditLogging()`
  - `setupLogRotate()`
  - `startCluster()`
  - `stopCluster()`
  - `syncClusters()`
  - `uninstallCmonAgents()`
  - `upgradeCluster()`

- Uses `/v2/logical_replication` for:
  - `addPublication()`
  - `addSubscription()`
  - `dropPublication()`
  - `dropSubscription()`
  - `listPublications()`
  - `listSubscriptions()`
  - `modifyPublication()`
  - `modifySubscription()`

### s9s container

**Expected:** `/v2/container` or `/v2/containers`

**Violations:**
- Uses `/v2/host` for:
  - `getContainers()`

- Uses `/v2/jobs` for:
  - `createContainerWithJob()`
  - `deleteContainerWithJob()`
  - `startContainerWithJob()`
  - `stopContainerWithJob()`

### s9s controller

**Expected:** `/v2/controller` or `/v2/controllers`

**Violations:**
- Uses `/v2/config` for:
  - `getLdapConfig()`
  - `setLdapConfig()`

- Uses `/v2/host` for:
  - `getControllers()`

- Uses `/v2/jobs` for:
  - `createSnapshotJob()`

### s9s group

**Expected:** `/v2/group` or `/v2/groups`

**Violations:**
- Uses `/v2/users` for:
  - `createGroup()`
  - `deleteGroup()`

### s9s maintenance

**Expected:** `/v2/maintenance` or `/v2/maintenances`

**Correctly uses:** `/v2/maintenance`

**Violations:**
- Uses `/v2/jobs` for:
  - `createMaintenanceWithJob()`

### s9s metatype

**Expected:** `/v2/metatype` or `/v2/metatypes`

**Violations:**
- Uses `/v2/discovery` for:
  - `getSupportedClusterTypes()`

### s9s node

**Expected:** `/v2/node` or `/v2/nodes`

**Violations:**
- Uses `/v2/config` for:
  - `setConfig()`
  - `unsetConfig()`

- Uses `/v2/host` for:
  - `unregisterHost()`

- Uses `/v2/jobs` for:
  - `inspectHost()`
  - `rebootHost()`
  - `registerHost()`
  - `restartNode()`
  - `startNode()`

### s9s process

**Expected:** `/v2/process` or `/v2/processes`

**Violations:**
- Uses `/v2/clusters` for:
  - `getSqlProcesses()`
  - `getTopQueries()`

### s9s replication

**Expected:** `/v2/replication` or `/v2/replications`

**Violations:**
- Uses `/v2/clusters` for:
  - `getClusters()`

- Uses `/v2/jobs` for:
  - `failoverMaster()`
  - `promoteReplicationSlave()`
  - `resetSlave()`
  - `stageSlave()`
  - `startSlave()`
  - `stopSlave()`
  - `toggleSync()`

### s9s script

**Expected:** `/v2/script` or `/v2/scripts`

**Violations:**
- Uses `/v2/jobs` for:
  - `executeCdtEntry()`

### s9s server

**Expected:** `/v2/server` or `/v2/servers`

**Violations:**
- Uses `/v2/host` for:
  - `getAcl()`
  - `getContainers()`
  - `getServers()`
  - `registerServers()`
  - `startServers()`
  - `stopServers()`
  - `unregisterServers()`

- Uses `/v2/jobs` for:
  - `createServer()`

- Uses `/v2/tree` for:
  - `addAcl()`
  - `deleteFromTree()`

### s9s sheet

**Expected:** `/v2/sheet` or `/v2/sheets`

**Violations:**
- Uses `/v2/spreadsheets` for:
  - `createSpreadsheet()`
  - `getSpreadsheet()`
  - `getSpreadsheets()`

**Note:** This is a naming inconsistency - the backend uses "spreadsheets" while the CLI uses "sheet".

### s9s tree

**Expected:** `/v2/tree` or `/v2/trees`

**Correctly uses:** `/v2/tree`

**Violations:**
- Uses `/v2/host` for:
  - `getAcl()`

### s9s user

**Expected:** `/v2/user` or `/v2/users`

**Correctly uses:** `/v2/users`

**Violations:**
- Uses `/v2/auth` for:
  - `resetPassword()`

## Common Patterns of Violations

### 1. Job Creation Pattern
Many subcommands use `/v2/jobs` for operations that create background jobs, even though the operation is initiated from a different subcommand context. Examples:
- `s9s cluster` operations that start jobs (e.g., `rollingRestart()`, `startCluster()`)
- `s9s backup` operations that create backup jobs (e.g., `createBackup()`, `restoreBackup()`)
- `s9s node` operations that start jobs (e.g., `restartNode()`, `registerHost()`)

### 2. Shared Resource Pattern
Some operations access shared resources through different endpoints:
- Multiple subcommands use `/v2/host` for host-related operations
- Multiple subcommands use `/v2/config` for configuration management

### 3. Logical Grouping vs. Technical Endpoint
Some operations are logically grouped under a subcommand but technically belong to a different endpoint:
- `s9s account` operations use `/v2/clusters` (accounts are cluster-specific)
- `s9s group` operations use `/v2/users` (groups are user management features)
- `s9s process` operations use `/v2/clusters` (processes are cluster-specific)

## Analysis

### Why Violations Occur

1. **Architectural Design**: The backend API is organized by technical function (jobs, hosts, clusters) rather than by CLI command structure.

2. **Job Creation Abstraction**: Many operations that initiate long-running tasks use the `/v2/jobs` endpoint regardless of which subcommand triggers them.

3. **Resource Hierarchy**: Some resources are nested or related (e.g., accounts belong to clusters, groups are part of user management).

4. **Historical Evolution**: The CLI and backend API may have evolved independently, leading to mismatches.

### Impact

The violations indicate that:
- The rule "one subcommand = one endpoint" is not consistently followed in the current implementation
- The backend API is organized more by functional domain than by CLI command structure
- Many operations span multiple endpoints to complete their tasks

### Recommendations

If strict adherence to the "one subcommand = one endpoint" rule is desired, consider:

1. **Backend API Restructuring**: Create dedicated endpoints for each subcommand that aggregate the necessary operations
2. **Documentation**: If the current structure is intentional, document the cross-endpoint usage patterns
3. **Frontend Wrapper Layer**: Add a layer in the CLI that maps subcommands to their appropriate endpoints, consolidating the logic
4. **Accept Current Design**: Recognize that the current design reflects functional organization and may be more maintainable than strict 1:1 mapping

## Files Analyzed

- `libs9s/s9srpcclient.cpp` - RPC client implementation (3130 lines)
- `libs9s/s9sbusinesslogic.cpp` - Business logic and subcommand routing
- `libs9s/s9sbusinesslogic.h` - Business logic interface

## Conclusion

The investigation reveals that **only 3 out of 19 subcommands** (16%) strictly follow the rule of using one and only one corresponding v2 endpoint. The majority of violations stem from operations that create jobs (using `/v2/jobs`) and operations that work with shared resources like hosts and configuration.

This suggests that the current API design prioritizes functional organization over CLI command structure alignment.
