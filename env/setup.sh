#!/usr/bin/env bash
# HARP environment. Source this once per shell:
#   source env/setup.sh            # LCG view from cvmfs (default, works on lxplus + condor)
#   HARP_ENV=native source env/setup.sh   # use the system ROOT/g++/cmake instead
#
# Everything HARP needs is derived from HARP_BASE (the repository root).

_harp_this="${BASH_SOURCE[0]:-$0}"
export HARP_BASE="$(cd "$(dirname "$_harp_this")/.." && pwd)"
export HARP_ENV="${HARP_ENV:-lcg}"
export HARP_LCG_VIEW="${HARP_LCG_VIEW:-/cvmfs/sft.cern.ch/lcg/views/LCG_107a/x86_64-el9-gcc13-opt}"

if [ "$HARP_ENV" = "lcg" ]; then
  if [ -f "$HARP_LCG_VIEW/setup.sh" ]; then
    # shellcheck disable=SC1090
    source "$HARP_LCG_VIEW/setup.sh"
  else
    echo "[HARP] LCG view not found at $HARP_LCG_VIEW, falling back to native environment" >&2
    export HARP_ENV=native
  fi
fi

export HARP_BUILD="${HARP_BUILD:-$HARP_BASE/build}"
export HARP_PLUGIN_PATH="$HARP_BUILD/lib/plugins"
export PATH="$HARP_BUILD/bin:$HARP_BASE/scripts:$PATH"
export LD_LIBRARY_PATH="$HARP_BUILD/lib:${LD_LIBRARY_PATH:-}"
export PYTHONPATH="$HARP_BASE/python:${PYTHONPATH:-}"
export PYTHONDONTWRITEBYTECODE=1

echo "[HARP] HARP_BASE=$HARP_BASE  env=$HARP_ENV  ROOT=$(root-config --version 2>/dev/null || echo 'not found')"
