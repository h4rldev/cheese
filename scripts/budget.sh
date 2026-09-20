#!/usr/bin/env bash
# Cheese memory budget check.
#
# Runs a release cheese demo (default: the butter Wayland gallery) and samples
# its /proc/<pid>/smaps_rollup once settled. Exits non-zero if Rss or Pss
# exceeds the budget. On-demand now, CI-ready later.
#
# Usage: scripts/budget.sh [binary] [seconds]
#   binary   release demo to run (default: butter-wayland-release)
#   seconds  settle time before sampling (default 5)
#
# Env: CHEESE_RSS_LIMIT_MIB, CHEESE_PSS_LIMIT_MIB (default 100 each)
set -euo pipefail

bin=${1:-bin/butter-wayland/butter-wayland-release/butter-wayland}
settle=${2:-5}
rss_limit=${CHEESE_RSS_LIMIT_MIB:-100}
pss_limit=${CHEESE_PSS_LIMIT_MIB:-100}

if [[ ! -x "$bin" ]]; then
  echo "budget: $bin not found; build with: conjure as butter-wayland-release test" >&2
  exit 1
fi

# On NixOS the Vulkan driver lives in the system profile, which the devshell
# does not export. Use the environment if set, else fall back to the Radeon ICD.
if [[ -z "${VK_ICD_FILENAMES:-}" && -f /run/opengl-driver/share/vulkan/icd.d/radeon_icd.x86_64.json ]]; then
  export VK_ICD_FILENAMES=/run/opengl-driver/share/vulkan/icd.d/radeon_icd.x86_64.json
  export LD_LIBRARY_PATH=/run/opengl-driver/lib${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}
fi

log=$(mktemp)
"$bin" >"$log" 2>&1 &
pid=$!
trap 'kill -9 "$pid" 2>/dev/null || true; rm -f "$log"' EXIT

sleep "$settle"
if ! kill -0 "$pid" 2>/dev/null; then
  echo "budget: demo exited before sampling:" >&2
  cat "$log" >&2
  exit 1
fi

read -r rss_kb pss_kb < <(awk '/^Rss:/{r=$2} /^Pss:/{p=$2} END{print r+0, p+0}' "/proc/$pid/smaps_rollup")
rss=$((rss_kb / 1024))
pss=$((pss_kb / 1024))
printf 'cheese budget: Rss=%d MiB Pss=%d MiB (limits %d / %d)\n' \
  "$rss" "$pss" "$rss_limit" "$pss_limit"

status=0
(( rss <= rss_limit )) || { echo "budget: Rss ${rss} MiB > ${rss_limit} MiB" >&2; status=1; }
(( pss <= pss_limit )) || { echo "budget: Pss ${pss} MiB > ${pss_limit} MiB" >&2; status=1; }
exit "$status"
