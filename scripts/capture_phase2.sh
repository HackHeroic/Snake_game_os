#!/usr/bin/env bash
#
# capture_phase2.sh
# -----------------
# Builds the ravi/phase2 branch in an isolated git worktree, drives the
# resulting binary, and saves PNG screenshots of phase-2 features (multi-food,
# ghost snake, power-ups, shrinking board, replay) to report/figures/.
#
# Leaves your main checkout untouched.

set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
OUT="$ROOT/report/figures"
WT="$ROOT/.phase2-worktree"
BRANCH="ravi/phase2"

cleanup_wt() {
    if [[ -d "$WT" ]]; then
        git -C "$ROOT" worktree remove --force "$WT" 2>/dev/null || true
        rm -rf "$WT"
    fi
    git -C "$ROOT" worktree prune 2>/dev/null || true
}
trap cleanup_wt EXIT

cleanup_wt   # in case a previous run failed

# verify branch exists locally; if not, hint at fetch
if ! git -C "$ROOT" rev-parse --verify "$BRANCH" >/dev/null 2>&1; then
    echo "ERROR: branch '$BRANCH' not found locally." >&2
    echo "       Run 'git fetch --all' (or 'git branch $BRANCH origin/$BRANCH') first." >&2
    exit 1
fi

echo "Creating worktree for $BRANCH at $WT ..."
git -C "$ROOT" worktree add "$WT" "$BRANCH"

echo "Building phase 2 binary ..."
(cd "$WT" && make >/dev/null 2>&1) || {
    echo "ERROR: phase 2 build failed. Run 'make -C $WT' manually to diagnose." >&2
    exit 1
}

if [[ ! -x "$WT/snake" ]]; then
    echo "ERROR: phase 2 binary not produced." >&2
    exit 1
fi

# Reuse the main capture script but point it at the phase-2 binary and prefix
# every screenshot with phase2_.
BIN="$WT/snake" PREFIX="phase2_" OUT="$OUT" "$ROOT/scripts/capture_screenshots.sh"

echo
echo "Phase 2 screenshots:"
ls -1 "$OUT"/phase2_*.png 2>/dev/null
