#!/usr/bin/env bash
# run_quick_test.sh - end-to-end smoke test of HARP on a handful of files:
# skim -> hist (on nano and on the skim) -> plots, for all gun samples.
#
# Usage:
#   source env/setup.sh
#   bash test/run_quick_test.sh [nFiles]     # nFiles per sample for the hist step (default 2)
#   bash test/run_quick_test.sh -h
# Output goes to test/out/ (histograms, skim, plots); takes a few minutes.
set -euo pipefail
case "${1:-}" in -h|--help) sed -n '2,/^set -euo/p' "$0" | grep '^#' | sed 's/^# \{0,1\}//'; exit 0;; esac
N=${1:-2}
cd "$HARP_BASE"
OUT=test/out
mkdir -p $OUT/hists
echo "== skim"
harp run configs/skim_hadron_cfg.py sample=pip_p1to200 maxFiles=1 maxEvents=300 outputFile=$OUT/skim_pip.root histFile=$OUT/skim_pip_hists.root --workdir work/test
harpInspect $OUT/skim_pip.root -c -e 1
echo "== hist on skim"
harp run configs/hist_hadron_cfg.py inputFiles=$OUT/skim_pip.root histFile=$OUT/hists_from_skim.root --workdir work/test
echo "== hist on nanos ($N files per sample)"
for s in $(harp samples list | awk '{print $1}'); do
  harp run configs/hist_hadron_cfg.py sample=$s maxFiles=$N histFile=$OUT/hists/$s.root --workdir work/test/$s > $OUT/hists/$s.log 2>&1 && echo "  $s ok" || { echo "  $s FAILED (see $OUT/hists/$s.log)"; exit 1; }
done
echo "== plots"
harp plot configs/plot_hadron_cfg.py histDir=$OUT/hists outputDir=$OUT/plots formats=png --workdir work/test
echo "== done: $(ls $OUT/plots/*.png | wc -l) figures in $OUT/plots"
