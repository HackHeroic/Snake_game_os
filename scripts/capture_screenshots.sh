#!/usr/bin/env bash
#
# capture_screenshots.sh
# ----------------------
# Drives the snake binary inside scripted tmux sessions and saves PNG
# screenshots of every key game state into report/figures/.
#
# Method: tmux gives the game a real PTY (so raw mode + ANSI work),
#         tmux send-keys feeds scripted input,
#         tmux capture-pane dumps the on-screen ANSI,
#         aha converts ANSI -> HTML,
#         headless Google Chrome renders HTML -> PNG.
#
# Requires: tmux, aha, Google Chrome.
#   brew install tmux aha
#   (Chrome usually already at /Applications/Google Chrome.app)

set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
BIN="${BIN:-$ROOT/snake}"
OUT="${OUT:-$ROOT/report/figures}"
SESSION="snake_capture"
COLS=80
ROWS=24
PREFIX="${PREFIX:-}"   # optional filename prefix, e.g. "phase2_"

mkdir -p "$OUT"

# --- preflight ----------------------------------------------------------------
for tool in tmux aha; do
    if ! command -v "$tool" >/dev/null 2>&1; then
        echo "ERROR: '$tool' not found. Install with: brew install $tool" >&2
        exit 1
    fi
done

CHROME=""
for candidate in \
    "/Applications/Google Chrome.app/Contents/MacOS/Google Chrome" \
    "/Applications/Chromium.app/Contents/MacOS/Chromium" \
    "/Applications/Brave Browser.app/Contents/MacOS/Brave Browser" \
    "/Applications/Microsoft Edge.app/Contents/MacOS/Microsoft Edge"; do
    if [[ -x "$candidate" ]]; then CHROME="$candidate"; break; fi
done
if [[ -z "$CHROME" ]]; then
    echo "ERROR: no Chrome / Chromium / Brave / Edge found." >&2
    exit 1
fi

if [[ ! -x "$BIN" ]]; then
    echo "Building snake at $BIN ..."
    (cd "$ROOT" && make >/dev/null)
fi
echo "Using browser: $CHROME"
echo "Using binary:  $BIN"

# --- helpers ------------------------------------------------------------------
end_session() {
    tmux kill-session -t "$SESSION" 2>/dev/null || true
}
# register cleanup BEFORE doing anything that might leave a session behind
trap end_session EXIT

end_session   # in case a previous run left a stale session

start_session() {
    tmux new-session -d -s "$SESSION" -x "$COLS" -y "$ROWS" \
         "cd '$ROOT' && '$BIN'"
    sleep 0.6
}

send() {
    tmux send-keys -t "$SESSION" "$1"
    sleep "${2:-0.15}"
}

snap() {
    local name="${PREFIX}$1"
    local ansi="$OUT/.tmp-$name.ansi"
    local html="$OUT/.tmp-$name.html"
    local png="$OUT/$name.png"

    tmux capture-pane -t "$SESSION" -e -p > "$ansi"
    aha --black --title "snake: $name" < "$ansi" > "$html"
    local css='<style>body{margin:0;padding:14px;background:#000;}pre{font-family:"Menlo","DejaVu Sans Mono","Consolas",monospace;font-size:14px;line-height:1.15;}</style>'
    /usr/bin/sed -i '' "s|</head>|${css}</head>|" "$html"

    "$CHROME" --headless=new --disable-gpu --no-sandbox --hide-scrollbars \
              --window-size=$((COLS * 9)),$((ROWS * 22)) \
              --screenshot="$png" "file://$html" 2>/dev/null
    rm -f "$ansi" "$html"
    if [[ -f "$png" ]]; then
        echo "  saved $png"
    else
        echo "  WARN: $png missing" >&2
    fi
}

# Drive a fresh game-launch through (mode, theme), play N ticks, then snap.
# Args: mode_key, theme_key, output_name, extra_keys (optional, played before snap)
capture_run() {
    local mode="$1" theme="$2" name="$3" extra="${4:-}"
    end_session
    start_session
    send "$mode" 0.6      # mode select on title screen
    send "$theme" 0.9     # theme select
    # let snake meander
    send "s" 0.4
    send "d" 0.4
    send "s" 0.4
    send "d" 1.4
    if [[ -n "$extra" ]]; then
        local i
        for ((i=0; i<${#extra}; i++)); do
            send "${extra:$i:1}" 0.18
        done
    fi
    snap "$name"
}

# --- 1. title screen ----------------------------------------------------------
echo "[01] title screen"
end_session
start_session
snap "title"

# --- 2. theme selection screen ------------------------------------------------
echo "[02] theme selection"
send "1" 0.5
snap "theme"

# --- 3a-3d. one gameplay shot per theme (Classic mode) ------------------------
echo "[03] gameplay -- Classic theme"
capture_run "1" "1" "gameplay_classic"
echo "[04] gameplay -- Ice theme"
capture_run "1" "2" "gameplay_ice"
echo "[05] gameplay -- Lava theme"
capture_run "1" "3" "gameplay_lava"
echo "[06] gameplay -- Rainbow theme"
capture_run "1" "4" "gameplay_rainbow"

# alias the Classic shot to the original 'gameplay' filename
cp "$OUT/${PREFIX}gameplay_classic.png" "$OUT/${PREFIX}gameplay.png"
echo "  aliased ${PREFIX}gameplay_classic.png -> ${PREFIX}gameplay.png"

# --- 4. wrap-around mode gameplay --------------------------------------------
echo "[07] gameplay -- Wrap-Around mode"
capture_run "2" "1" "gameplay_wrap"

# --- 5. pause overlay ---------------------------------------------------------
echo "[08] pause overlay"
end_session
start_session
send "1" 0.5     # classic mode
send "1" 0.7     # classic theme
send "s" 0.4
send "d" 0.6
send "p" 0.5     # pause
snap "pause"
send "p" 0.3     # unpause

# --- 6. game-over screen with stats ------------------------------------------
echo "[09] game over"
# we're still alive in classic/classic, drive into right wall
send "d" 0.2
for _ in 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15; do
    send "d" 0.18
done
sleep 1.2  # let death animation flash through
snap "gameover"

end_session

echo
echo "Done. Files in $OUT:"
ls -1 "$OUT"/*.png 2>/dev/null
