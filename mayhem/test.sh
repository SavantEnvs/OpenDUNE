#!/usr/bin/env bash
#
# mayhem/test.sh — OpenDUNE functional oracle (upstream ships no test suite).
# Runs mayhem/out/opendune_selftest (built by build.sh with normal flags), which loads hand-built
# PAK/SHP/FNT/string/ini files through the engine's loaders and asserts the decoded values, and
# runs upstream's extractpak tool on a known PAK, diffing the extracted files.
set -uo pipefail
[ -n "${SOURCE_DATE_EPOCH:-}" ] || unset SOURCE_DATE_EPOCH
: "${SRC:=/mayhem}"
cd "$SRC"

emit_ctrf() {
  local tool="$1" passed="$2" failed="$3" skipped="${4:-0}" pending="${5:-0}" other="${6:-0}"
  local tests=$(( passed + failed + skipped + pending + other ))
  cat > "${CTRF_REPORT:-$SRC/ctrf-report.json}" <<JSON
{
  "results": {
    "tool": { "name": "$tool" },
    "summary": {
      "tests": $tests,
      "passed": $passed,
      "failed": $failed,
      "pending": $pending,
      "skipped": $skipped,
      "other": $other
    }
  }
}
JSON
  printf 'CTRF {"results":{"tool":{"name":"%s"},"summary":{"tests":%d,"passed":%d,"failed":%d,"pending":%d,"skipped":%d,"other":%d}}}\n' \
    "$tool" "$tests" "$passed" "$failed" "$pending" "$skipped" "$other"
  [ "$failed" -eq 0 ]
}

ST="$SRC/mayhem/out/opendune_selftest"
XP="$SRC/mayhem/out/extractpak_plain"
for b in "$ST" "$XP"; do
  [ -x "$b" ] || { echo "missing $b — run mayhem/build.sh" >&2; emit_ctrf opendune-selftest 0 1; exit 1; }
done

EXPECTED=20
work=$(mktemp -d "${TMPDIR:-/tmp}/opendune-test.XXXXXX")
trap 'rm -rf "$work"' EXIT

out=$(TMPDIR="$work" "$ST" "$work/test.pak" 2>/dev/null)
echo "$out"
passed=$(grep -c '^PASS ' <<<"$out" || true)
failed=$(grep -c '^FAIL ' <<<"$out" || true)
# a selftest that stops early (crash, no-op) is missing checks: count them as failures
missing=$(( EXPECTED - passed - failed ))
[ "$missing" -gt 0 ] && failed=$(( failed + missing ))

# extractpak: extract the selftest's PAK and compare against the known entries
mkdir -p "$work/x"
if [ -f "$work/test.pak" ] && "$XP" --outdir "$work/x" "$work/test.pak" >/dev/null 2>&1 \
   && [ "$(cat "$work/x/test/A.TXT" 2>/dev/null)" = "hello" ] && [ "$(cat "$work/x/test/B.BIN" 2>/dev/null)" = "xyz1234" ]; then
  echo "PASS extractpak"; passed=$(( passed + 1 ))
else
  echo "FAIL extractpak"; failed=$(( failed + 1 ))
fi

emit_ctrf opendune-selftest "$passed" "$failed"
