---
name: PR Auto-Review
description: >
  Automatically approve any open, non-draft PR carrying the 'auto-review' label, UNLESS the
  risk assessment comes back HIGH or CRITICAL — those are never auto-approved, only flagged for
  human review. If 'service-account-teamcity' isn't already a requested reviewer, requests it
  first, then evaluates the diff for risk. Posts a summary to #-dev-cc-general on Slack.

# Event-driven, not scheduled — reacts the moment the label lands. `names: [auto-review]`
# scopes the trigger at the GitHub Actions level (no wasted runs for unrelated labels).
# `skip-if-no-match` is a cheap pre-agent gate: a repo-scoped search run BEFORE the engine
# starts, skipping the whole run (no engine cost) unless a PR currently has the label.
on:
  pull_request:
    types: [labeled]
    names: [auto-review]
  skip-if-no-match: "is:pr is:open label:auto-review"
  workflow_dispatch:
    inputs:
      pr_number:
        description: "PR number to process directly (manual test run)"
        required: true
      dry_run:
        description: "Print intended actions without approving or posting to Slack"
        type: boolean
        required: false
        default: false

# This file is deployed as-is (via `gh aw deploy`) into each target repo's own
# .github/workflows/ — GitHub Actions triggers only fire on events in the repo a
# workflow lives in, so "any repo" coverage means a copy in every repo, not one
# central workflow watching everything.
#
# Deploy scope (per 2026-07-24 direction): develop/main/master AND every release branch,
# on every CC repo that has a maintainer roster in `config/repo-maintainers.json` — i.e.
# clustercontrol-enterprise, s9s-tools, s9s-cmon-test, frontend-hub, cmon-proxy, cmon_infra,
# clustercontrol-k8s. (Narrower than the original 10-repo list — cloudlink-go, cmon-events,
# cmon-ssh, cc-telemetry are dropped since they have no maintainer for the PR-comment
# reference below.) Full multi-branch rollout is still pending; currently only deployed to
# clustercontrol-enterprise (develop, 2.4.0_release) and 2.5.0_release (needs this update
# re-applied — see MAINTAINER_GITHUB_LOGIN below, which is the ONE value that must differ
# per repo; keep everything else in sync across deployments.

# Bare-metal self-hosted runner
runs-on: baremetal

# Single-PR event, not a multi-repo sweep — short ceiling.
timeout-minutes: 15

# Read-only at the job level — approval/comment/reviewer-add go through safe-outputs below,
# which run in their own scoped job so the main agent job never holds write creds.
# copilot-requests: write lets the Copilot engine authenticate via the GitHub Actions
# token instead of a personal COPILOT_GITHUB_TOKEN secret — requires centralized
# Copilot billing enabled on the severalnines org (confirmed enabled 2026-07-23). If
# a target repo's org context ever lacks that, drop this permission and set
# COPILOT_GITHUB_TOKEN instead (see https://github.github.com/gh-aw/reference/billing/).
permissions:
  contents: read
  pull-requests: read
  issues: read
  copilot-requests: write

# Model choice is constrained by the *installed Copilot CLI binary*, which validates the
# requested model against its own list BEFORE any request reaches gh-aw's api-proxy. So gh-aw
# model aliases ("agent", "sonnet", "large") and its model_fallback chain do NOT help here --
# the CLI rejects the alias outright. Observed on CLI v1.0.65:
#   model 'agent' -> "Error: model 'agent' is retired or unsupported. Did you mean 'gpt-4'?"
#   model 'gpt-5' -> same class of rejection
# Hence a concrete model ID, not an alias.
#
# Upgrading gh-aw to get a newer CLI (v0.83.4 pins Copilot CLI 1.0.75) was tried and reverted:
# the newer compiler emits gh-aw-firewall image tag 0.27.42, the baremetal runners only have
# 0.27.11 cached, and the agent step runs `docker compose up --pull never`, so it dies with
# "Command failed with exit code 1: docker compose up ... squid-proxy". Bumping gh-aw therefore
# requires provisioning the new firewall images on the runners first. Staying on v0.81.6.
#
# gpt-4 (the previous pin) is accepted by the CLI but too weak to drive this workflow: observed
# runs used ~160 output tokens in 7-12s, made ZERO GitHub tool calls, and still reported
# success -- s9s-cmon-test#1121 narrated "review request issued" and "auto-approved" while
# agent_output.json was `{"items":[]}`, and s9s-cmon-test#1117 claimed the PR was a draft or
# unlabelled when it was neither. Both runs went green, so the failure is SILENT.
#
# Model choice is deliberately NOT a Claude model: most of the team already reviews with
# Claude, so an independent model surfaces issues a same-family reviewer would tend to agree
# with.
#
# The severalnines Copilot Business tier exposes exactly these chat models (read from the
# api-proxy at runtime, artifact sandbox/firewall/logs/api-proxy-logs/models.json):
#   gpt-5.3-codex, gpt-5-mini, gpt-4.1, gpt-4o (+dated), gpt-4, gpt-3.5-turbo,
#   claude-opus-4.6, claude-opus-4.5, claude-sonnet-4.5, claude-haiku-4.5,
#   gemini-2.5-pro, gemini-3-flash-preview
# There is NO gpt-5.5 / gpt-5.4 / plain gpt-5 on this tier -- gpt-5.5 passed CLI validation but
# the API returned "400 The requested model is not supported" (unavailable for this
# subscription tier). Check that list before changing this field.
#
# gpt-5.3-codex is the strongest GPT-family model on the tier. claude-sonnet-4.5 is verified
# working (clustercontrol-enterprise#3341 approved with 2 real tool calls) and is the fallback
# if a GPT model regresses.
#
# The silent-success mode is inherent to agentic tool-calling and is only made less likely by a
# stronger model, not eliminated. Treat a green run with an empty safe_outputs artifact as a
# FAILED run and re-run it; there is still no automatic detection or retry.
engine:
  id: copilot
  model: gpt-5.3-codex

# Let the agent reach GitHub and Slack
network:
  allowed:
    - "github.com"
    - "api.github.com"
    - "hooks.slack.com"

tools:
  # Built-in GitHub MCP toolset for reads (current PR's labels/reviewers/diff). No
  # custom github-token here — this workflow only ever touches the repo it lives in
  # (each deployed copy is single-repo), so the default GITHUB_TOKEN's read scope is
  # sufficient. AUTO_REVIEW_BOT_TOKEN is reserved for the write side (safe-outputs
  # below) where actions must be attributed to service-account-teamcity specifically,
  # not for reads.
  github:
    read-only: true
    # Reacts only to PRs already gated behind the `auto-review` label, applied by an
    # internal maintainer — not driven by untrusted fork events, so no minimum
    # provenance/integrity gate.
    min-integrity: none
    toolsets: [default]
  bash:
    - "jq"
    - "date"

# Writes are funneled through safe-outputs so strict mode can validate the blast
# radius at compile time instead of trusting free-form bash. `github-token` here
# must be a credential for `service-account-teamcity` specifically (not the default
# GITHUB_TOKEN, which would post as `github-actions[bot]`) — the whole point is that
# the reviewer-add, approval, and comment are all attributed to that account.
safe-outputs:
  github-token: ${{ secrets.AUTO_REVIEW_BOT_TOKEN }}
  # allowed-reviewers locks this down at compile time: even if the agent tried, it could
  # never request review from anyone other than the bot itself.
  add-reviewer:
    max: 1
    allowed-reviewers: ["service-account-teamcity"]
  submit-pull-request-review:
    max: 1
    allowed-events: [APPROVE]
  add-comment:
    max: 1

env:
  DRY_RUN: ${{ github.event.inputs.dry_run || 'false' }}
  PR_NUMBER: ${{ github.event.pull_request.number || github.event.inputs.pr_number }}
  # The ONE value that must be customized per deployed repo copy (see deploy-scope note
  # above) — always alvaro-vinuela on clustercontrol-enterprise and s9s-cmon-test per
  # explicit instruction; on other repos, the first entry in that repo's
  # config/repo-maintainers.json roster (deterministic default — override here if a
  # different maintainer should be referenced for a given repo).
  MAINTAINER_GITHUB_LOGIN: csjpeter-s9s
---

# PR Auto-Review — Automated Approval with Risk Assessment

You are the ClusterControl Engineering Agent, triggered by a single pull request event in this
repository. Your job: verify PR `PR_NUMBER` currently carries the `auto-review` label and isn't a
draft, then assess risk and approve it **only if the risk is LOW or MEDIUM** — requesting yourself
as reviewer first if you aren't already one. HIGH and CRITICAL risk PRs are never auto-approved;
they get a comment flagging the maintainer for manual review and approval instead.

> Team roster, repo list, and the `#-dev-cc-general` Slack channel (`C02HQ48KVC7`) live in
> `CLAUDE.md`. This workflow builds on those conventions.

> **Reviewer identity note:** the write-side credential (`AUTO_REVIEW_BOT_TOKEN`) belongs to
> **`service-account-teamcity`** — an existing shared CI/automation GitHub account (confirmed
> reusable by Alex Yu in Slack, `#-po-and-crafters`, 2026-06-26), not a new account. Being a
> requested reviewer is **not** a precondition for approval anymore — the `auto-review` label
> alone is the opt-in signal. If the bot isn't already a requested reviewer when this runs, it
> requests itself first (Step 2), so no separate human action or assignment automation is needed
> beyond applying the label.

> **Why re-check instead of trusting the trigger payload:** `skip-if-no-match` already confirmed
> *some* PR in this repo has the label before this job started, but that search isn't necessarily
> the exact PR that fired the triggering event (e.g. the label could have been removed again
> between the event firing and this job running). Always re-verify PR `PR_NUMBER`'s *current*
> state directly rather than assuming the trigger payload or the pre-check still holds.

## Runtime inputs

- `PR_NUMBER` (env) — the pull request to process. From `github.event.pull_request.number` on a
  real trigger, or `github.event.inputs.pr_number` on a manual `workflow_dispatch` test run.
- `DRY_RUN` (env) — when `"true"`, print intended actions only; do **not** request review, approve,
  comment, or post to Slack.
- `MAINTAINER_GITHUB_LOGIN` (env) — the GitHub login referenced in the human-review-recommended PR
  comment (Step 5). Fixed per repo, not rotated — see the frontmatter note above.

## Steps

> **Tool access note:** reads go through the built-in `github` MCP toolset configured in
> frontmatter — not the `gh` CLI. Confirm exact tool names for this gh-aw version with
> `gh aw mcp list-tools pr-auto-review --server github` before relying on a specific one, since
> toolset contents can shift between gh-aw releases.

> **Formatting note — read before Steps 4-6:** every `body` text you submit to a safe-output tool
> (`submit_pull_request_review`, `add_comment`) must contain **real line breaks**, not the literal
> two-character sequence `\n`. This model has intermittently emitted literal `\n` in these bodies
> (e.g. clustercontrol-enterprise#3386, #3409), producing an unreadable single-line comment/review.
> Before calling any of these tools, re-read the `body` value you're about to send and confirm it
> renders as multiple paragraphs, not one line containing backslash-n characters — if you drafted
> it as a JSON-escaped string in your own reasoning, decode it to actual newlines first.

### 1. Re-verify the PR still qualifies

Fetch PR `PR_NUMBER` in this repository — current `isDraft`, `labels`, and `reviewRequests`.

Proceed only if **both** hold:

- `isDraft` is `false` (draft PRs are never processed, even if manually labelled), **and**
- `labels[].name` includes `auto-review` (case-insensitive).

If either fails, stop here — print a one-line no-op summary (see Step 7) and do **not** call any
safe-output. This is the expected outcome for most `labeled` events on this repo (most PRs aren't
going through auto-review at all); it is not an error.

### 2. Ensure you're a requested reviewer

Check `reviewRequests[].login` from Step 1. If it already includes `service-account-teamcity`,
skip this step. Otherwise — unless `DRY_RUN` is `"true"` — call the `add_reviewer` safe-output
tool:

- `pull_request_number`: `PR_NUMBER`
- `reviewers`: `["service-account-teamcity"]`

`allowed-reviewers` on the `add-reviewer` safe-output restricts this to that one account — the
tool cannot be used to request review from anyone else, by design.

### 3. Risk assessment

Fetch the full diff for PR `PR_NUMBER` via the GitHub toolset's diff tool.

Acting as a **senior software developer**, evaluate the diff for:

1. **Security** — credential handling, injection, auth bypass, data exposure
2. **Data integrity** — migrations, schema changes, data-loss potential
3. **Breaking changes** — API contract changes, backward incompatibility
4. **Complexity** — large refactors, algorithmic changes, concurrency
5. **Test coverage** — are the changes adequately tested?
6. **Configuration** — environment, infra, deployment config

Produce this JSON (no markdown fences) for your own bookkeeping — it feeds the review body and the
Slack summary, it is not itself a tool call:

```json
{
  "risk_level": "LOW | MEDIUM | HIGH | CRITICAL",
  "needs_human_review": true,
  "summary": "<1-2 sentence summary of what the PR does>",
  "risks": ["<risk 1>", "<risk 2>"],
  "recommendation": "<brief recommendation>"
}
```

Rules for `needs_human_review`:

- LOW risk, small scope (< 100 lines), well-tested → `false`
- MEDIUM risk but straightforward (docs, config, deps) → `false`
- MEDIUM risk with logic changes → `true`
- HIGH or CRITICAL risk → always `true`
- Any security-sensitive change → always `true`
- Database migrations or schema changes → always `true`
- Diff larger than 10,000 lines → note "diff too large for full analysis" and default to `true`

Separately, compute `auto_approve = risk_level is LOW or MEDIUM` — **HIGH and CRITICAL never
auto-approve**, regardless of `needs_human_review` (which is already always `true` for both, but
`auto_approve` is the gate the next step actually checks — keep the two concepts distinct).

### 4. Approve the PR — only when risk is LOW or MEDIUM

If `auto_approve` is `false` (risk is HIGH or CRITICAL), **skip this step entirely** — do not call
`submit_pull_request_review` at all. Go straight to Step 5; the PR is left with no review from
this workflow, and a human must approve it manually.

Otherwise, if `DRY_RUN` is `"true"`, print the intended approval instead and skip the tool call.
Otherwise call the `submit_pull_request_review` safe-output tool:

- `pull_request_number`: `PR_NUMBER`
- `event`: `APPROVE`
- `body`:

```
✅ Auto-approved by Engineering Agent

**Risk Assessment:** <risk_level>
**Needs Human Review:** <Yes/No>

<summary>

<risks as a bullet list, if any>

<recommendation>

---
_Automated review — if flagged for human review, please have a team member verify before merge._
```

Never merge — approval only. `allowed-events: [APPROVE]` on the `submit-pull-request-review`
safe-output means this is the only review event this workflow is even permitted to submit — a
compile-time guardrail against ever emitting `REQUEST_CHANGES` unattended.

### 5. Flag the maintainer on the PR

When `needs_human_review` is `true`, call the `add_comment` safe-output tool with a follow-up
comment that @-mentions `MAINTAINER_GITHUB_LOGIN`. The wording depends on whether Step 4 approved:

- If `auto_approve` was `false` (HIGH/CRITICAL — not approved):

```
🚫 Not auto-approved — risk assessed as <risk_level>. This PR needs a human review AND approval
before merge, this workflow has not submitted any review. cc @<MAINTAINER_GITHUB_LOGIN>

<recommendation>
```

- If `auto_approve` was `true` but `needs_human_review` is still `true` (MEDIUM with logic
  changes) — unchanged from before:

```
⚠️ Human review recommended before merge. cc @<MAINTAINER_GITHUB_LOGIN>

<recommendation>
```

- `pull_request_number`: `PR_NUMBER`

The maintainer reference is a fixed value for this repo (`MAINTAINER_GITHUB_LOGIN`), not resolved
dynamically from CODEOWNERS or rotated — see the env var note above. Skip this step under
`DRY_RUN`.

### 6. Post the Slack summary

Build the summary message (Slack `mrkdwn` — single `*asterisks*` for bold, `<url|text>` links):

```
🤖 *PR Auto-Review — <repo>#<PR_NUMBER>*
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

<https://github.com/<owner>/<repo>/pull/<PR_NUMBER>|<repo>#<PR_NUMBER>> — _<title>_
Author: <author> | Risk: <emoji> <risk_level>
<if !auto_approve>🚫 *Not auto-approved — human approval required:* <recommendation></if>
<if auto_approve && needs_human_review>⚠️ *Approved, human review recommended:* <recommendation></if>
<if auto_approve && !needs_human_review>✅ No additional review needed</if>
```

Risk emojis: LOW → 🟢, MEDIUM → 🟡, HIGH → 🟠, CRITICAL → 🔴.

If `DRY_RUN` is `"true"`, print the message to the job log instead of posting.

> **Open design item — do not wire this up as raw `curl` + env secret.** `gh-aw` strict mode
> flags any secret placed where the agent process/container can read it (that's the whole
> "secrets in env leaked to agent container" error this file kept hitting during earlier
> compilation attempts). A Slack webhook URL in a bash-tool env var has exactly the same problem
> `GH_TOKEN` did. Two compliant options, to decide before enabling this workflow:
> 1. **Emit the Slack text as the agent's structured output** (e.g. via a small custom
>    safe-output, or written to the job summary) and have a plain, non-agent GitHub Actions step
>    — added after the agent job, outside the sandboxed container — do the actual `curl` POST
>    with `SLACK_WEBHOOK_URL` bound only in *that* step's env.
> 2. **Use the Slack MCP server** already referenced in this repo's `CLAUDE.md`
>    (`mcp.slack.com/mcp`, `slack_send_message`) instead of a raw webhook — but confirm its auth
>    flow is non-interactive first; the Slack MCP integration used elsewhere in this repo may rely
>    on a per-user OAuth handshake that doesn't work from an unattended CI job.

### 7. Job summary

Print to the job log — either:

```
🤖 PR Auto-Review — <repo>#<PR_NUMBER>   [DRY-RUN | APPROVED | NOT APPROVED - HIGH RISK]
Risk: <risk_level> | Human review recommended: <Yes/No>
```

or, if Step 1's conditions weren't met:

```
🤖 PR Auto-Review — <repo>#<PR_NUMBER> — no-op
Missing: <draft / auto-review label — whichever wasn't satisfied>
```

## Guardrails

- **Never merge** — this workflow only approves. Merging stays a human decision.
- **HIGH and CRITICAL risk PRs are never auto-approved** (`auto_approve = false`, Step 3/4) — this
  workflow submits no review at all for them, only a PR comment and Slack summary flagging that a
  human must review AND approve before merge. This intentionally does NOT unblock CI/branch
  protection gates that require an approval — that's the point for risky changes.
- A PR flagged `needs_human_review: true` but with `auto_approve: true` (MEDIUM risk with logic
  changes) is still approved to unblock CI gates, with the PR comment and Slack summary calling
  for human verification before merge — this case is unchanged from before.
- The `auto-review` label is the **sole** opt-in signal (Step 1) — being a requested reviewer is no
  longer required; the workflow self-assigns as reviewer (Step 2) if needed before approving.
- Never print secrets to the log — none should reach the agent process at all; GitHub access goes
  through the `tools.github` toolset proxy, not a raw token in `env`.
- `safe-outputs` caps this run to at most 1 reviewer-add, 1 review, and 1 comment (this workflow
  processes exactly one PR per trigger). Reviews may only ever be `APPROVE` — `REQUEST_CHANGES`/
  `COMMENT` are not in `allowed-events`, so this workflow cannot block a PR even if the agent
  tried to. Reviewer-add is locked to `service-account-teamcity` only via `allowed-reviewers`.
- Deployed as a near-identical copy across all target repos (`gh aw deploy`) — the ONE field that
  should differ per repo is `env.MAINTAINER_GITHUB_LOGIN`; keep everything else in sync across
  deployments, don't hand-edit a single repo's copy independently otherwise.
- Tool-calling isn't 100% reliable even on a working model/config — a run can occasionally narrate
  "approved" without actually calling the safe-output tool, leaving the PR unapproved with no error
  surfaced. No retry/detection for this failure mode is built in yet; worth monitoring.
