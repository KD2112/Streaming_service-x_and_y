#!/usr/bin/env bash
# Poll public IPv4 and report when the provider IP changes.
set -euo pipefail

INTERVAL_SEC="${INTERVAL_SEC:-60}"
STATE_FILE="${STATE_FILE:-$HOME/.cache/watch-public-ip.last}"
LOG_FILE="${LOG_FILE:-$HOME/.cache/watch-public-ip.log}"

mkdir -p "$(dirname "$STATE_FILE")" "$(dirname "$LOG_FILE")"

fetch_ip() {
  local ip=""
  local url
  for url in \
    "https://api.ipify.org" \
    "https://icanhazip.com" \
    "https://ifconfig.me/ip"
  do
    ip="$(curl -4 -fsS --max-time 8 "$url" 2>/dev/null | tr -d '[:space:]' || true)"
    if [[ "$ip" =~ ^[0-9]+\.[0-9]+\.[0-9]+\.[0-9]+$ ]]; then
      printf '%s\n' "$ip"
      return 0
    fi
  done
  return 1
}

prev=""
if [[ -f "$STATE_FILE" ]]; then
  prev="$(tr -d '[:space:]' <"$STATE_FILE" || true)"
fi

echo "Watching public IP every ${INTERVAL_SEC}s (Ctrl+C to stop)"
echo "State: $STATE_FILE"
echo "Log:   $LOG_FILE"
echo

while true; do
  ts="$(date -Iseconds)"
  if ! ip="$(fetch_ip)"; then
    echo "[$ts] ERROR: could not fetch public IP"
    sleep "$INTERVAL_SEC"
    continue
  fi

  if [[ -z "$prev" ]]; then
    echo "[$ts] current IP: $ip"
    printf '%s\n' "$ip" >"$STATE_FILE"
    printf '%s START %s\n' "$ts" "$ip" >>"$LOG_FILE"
    prev="$ip"
  elif [[ "$ip" != "$prev" ]]; then
    echo "[$ts] CHANGED: $prev -> $ip"
    printf '%s CHANGE %s -> %s\n' "$ts" "$prev" "$ip" >>"$LOG_FILE"
    printf '%s\n' "$ip" >"$STATE_FILE"
    prev="$ip"
  else
    echo "[$ts] unchanged: $ip"
  fi

  sleep "$INTERVAL_SEC"
done
