#!/usr/bin/env bash
# build.sh - compile the HARP illustration slides (illustrations/harp_illustrations.tex).
#
# Usage:
#   bash illustrations/build.sh            # -> illustrations/harp_illustrations.pdf
#   bash illustrations/build.sh --png      # also render one PNG per slide into illustrations/png/
#   bash illustrations/build.sh --clean    # remove the LaTeX auxiliary files
#   bash illustrations/build.sh -h
# Needs pdflatex with beamer and TikZ (TeX Live on lxplus is enough); --png needs pdftoppm.
set -euo pipefail
cd "$(dirname "${BASH_SOURCE[0]}")"
case "${1:-}" in
  -h|--help) sed -n '2,/^set -euo/p' "$0" | grep '^#' | sed 's/^# \{0,1\}//'; exit 0 ;;
  --clean) rm -f harp_illustrations.{aux,log,nav,out,snm,toc,vrb}; echo "cleaned"; exit 0 ;;
esac
pdflatex -interaction=nonstopmode -halt-on-error harp_illustrations.tex > build.log 2>&1 || { grep -A3 "^!" build.log | head -30; exit 1; }
pdflatex -interaction=nonstopmode -halt-on-error harp_illustrations.tex >> build.log 2>&1
rm -f harp_illustrations.{aux,nav,out,snm,toc,vrb}
echo "built harp_illustrations.pdf ($(grep -c "^" build.log) log lines, see build.log for warnings)"
if [ "${1:-}" = "--png" ]; then
  mkdir -p png
  pdftoppm -png -r 110 harp_illustrations.pdf png/slide
  echo "rendered $(ls png/*.png | wc -l) PNG slides in png/"
fi
