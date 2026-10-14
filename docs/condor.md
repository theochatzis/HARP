# Batch processing with HTCondor

See `condor/README.md` for the commands. Summary:

```bash
harp submit --cfg configs/hist_hadron_cfg.py --hist-only --sample pip_p0p2to200 pim_p0p2to200 \
            --output-dir /eos/user/$USER/HARP/hists_v1 --tag hists_v1 --files-per-job 10 [--opt linkScoreCut=0.8] [--dry-run]
harp check --tag hists_v1
harp submit --resubmit --tag hists_v1
harp merge --tag hists_v1          # -> /eos/user/$USER/HARP/hists_v1/merged/<sample>.root
harp plot configs/plot_hadron_cfg.py histDir=/eos/user/$USER/HARP/hists_v1/merged
```

* One job = one sample × `files-per-job` input files, run through
  `harp run <cfg> inputFiles=... outputFile=... histFile=...`.
* Work directory `~/condor_work/HARP/<tag>` (never on EOS); code shipped as
  `harp.tar.gz`; workers only need cvmfs (LCG view) and EOS.
* `harp check` opens every expected output (zombie / recovered / missing
  cutflow → bad) and writes `resubmit.txt`.
* `--dry-run` + `bash job.sh <jobid>` in a scratch directory reproduces a
  worker run interactively (see `condor/README.md`).

For skim productions (`skim_hadron_cfg.py`) drop `--hist-only`: each job
writes `<sample>_skim_hadron_<i>.root` and `<sample>_skim_hadron_hists_<i>.root`.
