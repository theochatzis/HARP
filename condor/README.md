# Condor submission

`harp submit` wraps the whole procedure (see `python/harp/condor.py`):

```bash
source env/setup.sh
harp samples scan                       # cache the file lists of samples/*.json once

# histogram job on all pion samples, 10 nano files per job
harp submit --cfg configs/hist_hadron_cfg.py --hist-only \
            --sample pip_p0p2to200 pim_p0p2to200 pi0_p0p2to200 \
            --output-dir /eos/user/$USER/HARP/hists_v1 --tag hists_v1 --files-per-job 10

# skim job (writes a skim + a control hist file per job)
harp submit --cfg configs/skim_hadron_cfg.py --sample pip_p0p2to200 \
            --output-dir /eos/user/$USER/HARP/skim_v1 --tag skim_v1 --files-per-job 5 --opt minEnergy=2

harp check --tag hists_v1               # validates every output, writes resubmit.txt
harp submit --resubmit --tag hists_v1   # resubmits the failed ones
harp merge --tag hists_v1               # hadd per sample -> <output-dir>/merged/<sample>.root
```

What happens:

1. A work directory `~/condor_work/HARP/<tag>/` is created **outside EOS**
   (condor schedds refuse submit files on EOS). It holds `job.sub`, `job.sh`,
   `jobs.json` (job id -> files / expected outputs), `logs/`.
2. The compiled binaries, plugin libraries, `python/`, `configs/`, `samples/`
   and `env/` are packed in `harp.tar.gz` and transferred with each job, so the
   worker only needs cvmfs (LCG view) and EOS access.
3. The worker unpacks, sources `env/setup.sh`, runs
   `harp run configs/<cfg> inputFiles=... outputFile=... histFile=...` and
   copies the outputs to `--output-dir` with `xrdcp` (EOS) or `cp`.

`--dry-run` prepares the work directory without submitting; you can then
test a job locally:

```bash
cd ~/condor_work/HARP/<tag> && mkdir -p test && cd test && cp ../harp.tar.gz ../jobs.json . && bash ../job.sh pip_p0p2to200_hist_hadron_000
```

The templates in `condor/templates/` are the same texts that `condor.py`
generates, kept here for reference and manual use.
