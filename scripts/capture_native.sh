#!/usr/bin/env bash
#
# capture_native.sh
# -----------------
# Drives the snake binary in a real Terminal.app window and uses macOS
# screencapture to save PNGs of the live window.
#
# Pros: true high-fidelity screenshots (real font, real colors, real cursor).
# Cons: needs Screen Recording permission for the parent terminal,
#       and the Terminal window must be visible (not minimised).
#
# Requirements:
#   - macOS (uses osascript + screencapture)
#   - Screen Recording permission for whatever launches this script
#     (Terminal / iTerm / Claude Code).  Grant in:
#       System Settings -> Privacy & Security -> Screen Recording.
#
# If permission is missing, the captured PNGs will be black; in that case
# fall back to scripts/capture_screenshots.sh (headless).

set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
BIN="$ROOT/snake"
OUT="$ROOT/report/figures"
mkdir -p "$OUT"

if [[ ! -x "$BIN" ]]; then
    echo "Building snake first..."
    (cd "$ROOT" && make >/dev/null)
fi

# --- launch a fresh Terminal window running the game --------------------------
# We use a small wrapper so AppleScript can target this exact window.
WRAPPER="$ROOT/.snake_capture_wrapper.sh"
cat > "$WRAPPER" <<EOF
#!/usr/bin/env bash
cd "$ROOT"
exec ./snake
EOF
chmod +x "$WRAPPER"

echo "Opening a Terminal.app window with the game..."
WINDOW_ID=$(osascript <<APPLESCRIPT
tell application "Terminal"
    activate
    set newTab to do script "exec '$WRAPPER'"
    set newWindow to window 1
    set bounds of newWindow to {100, 100, 900, 600}
    delay 0.4
    return id of newWindow
end tell
APPLESCRIPT
)

echo "Window id: $WINDOW_ID"
sleep 0.8   # let the title screen draw

# --- helpers ------------------------------------------------------------------
send_keys() {
    # send_keys "<keystrokes>" [delay-after-seconds]
    local keys="$1"
    local delay="${2:-0.2}"
    osascript <<APPLESCRIPT >/dev/null
tell application "System Events"
    tell process "Terminal"
        set frontmost to true
        keystroke "$keys"
    end tell
end tell
APPLESCRIPT
    sleep "$delay"
}

snap() {
    local name="$1"
    local png="$OUT/$name.png"
    # -l <windowID>  capture this exact window
    # -x             no shutter sound
    # -o             no window shadow
    screencapture -l "$WINDOW_ID" -x -o "$png"
    echo "  saved $png"
}

cleanup() {
    # send Q a few times to exit the game cleanly, then close window
    osascript <<'APPLESCRIPT' >/dev/null 2>&1 || true
tell application "System Events"
    tell process "Terminal"
        set frontmost to true
        keystroke "q"
        delay 0.2
        keystroke "q"
    end tell
end tell
delay 0.4
tell application "Terminal"
    try
        close window 1 saving no
    end try
end tell
APPLESCRIPT
    rm -f "$WRAPPER"
}
trap cleanup EXIT

# --- 1. title screen ----------------------------------------------------------
echo "[1/5] title screen"
snap "title"

# --- 2. theme selection -------------------------------------------------------
# pressing "1" on the title picks Classic mode and advances to theme select
echo "[2/5] theme selection"
send_keys "1" 0.6
snap "theme"

# --- 3. gameplay --------------------------------------------------------------
echo "[3/5] gameplay"
send_keys "1" 0.8     # pick theme 1 (Classic) -> game starts
send_keys "s" 0.5
send_keys "d" 0.5
send_keys "s" 0.5
send_keys "d" 1.5     # let several ticks elapse
snap "gameplay"

# --- 4. pause overlay ---------------------------------------------------------
echo "[4/5] pause overlay"
send_keys "p" 0.5
snap "pause"
send_keys "p" 0.3     # unpause

# --- 5. game over (drive into right wall) -------------------------------------
echo "[5/5] game over"
send_keys "d" 0.2
for _ in 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15; do
    send_keys "d" 0.18
done
sleep 1.0
snap "gameover"

echo
echo "Done. Files in $OUT:"
ls -1 "$OUT"/*.png 2>/dev/null
