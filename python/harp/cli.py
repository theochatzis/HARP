"""The `harp` command line front-end (see scripts/harp)."""
import argparse
import os
import subprocess
import sys

from harp import samples
from harp.config import load_config

HARP_BASE = samples.HARP_BASE


def _work_json(cfg, workdir, suffix="json"):
    os.makedirs(workdir, exist_ok=True)
    stem = os.path.splitext(os.path.basename(cfg))[0]
    return os.path.join(workdir, stem + "." + suffix)


def cmd_run(args):
    """harp run cfg.py key=value ... [--dry-run] [--workdir DIR] [-- harpRun options]"""
    process = load_config(args.config, args.options)
    if not hasattr(process, "path"):
        raise SystemExit("%s does not define a harp.config.Process" % args.config)
    path = process.dump(_work_json(args.config, args.workdir))
    print(process.summary())
    print("[harp run] configuration written to %s" % path)
    if args.dry_run:
        return 0
    cmd = ["harpRun", path] + args.harp_args
    print("[harp run] " + " ".join(cmd))
    return subprocess.call(cmd)


def cmd_dump(args):
    obj = load_config(args.config, args.options)
    print(obj.to_json())
    return 0


def cmd_plot(args):
    job = load_config(args.config, args.options)
    if not hasattr(job, "plots"):
        raise SystemExit("%s does not define a harp.plotcfg.PlotJob" % args.config)
    path = job.dump(_work_json(args.config, args.workdir))
    print(job.summary())
    print("[harp plot] plot job written to %s" % path)
    if args.dry_run:
        return 0
    cmd = ["harpPlot", path] + args.harp_args
    print("[harp plot] " + " ".join(cmd))
    return subprocess.call(cmd)


def cmd_samples(args):
    if args.action == "list":
        for name, info in samples.registry().items():
            cached = os.path.exists(samples.list_path(name))
            print("%-18s pdgId=%-6d %-10s %s%s" % (name, info["pdgId"], info.get("type", ""), info["path"], "  [cached list]" if cached else ""))
    elif args.action == "scan":
        for name in (args.names or samples.names()):
            files = samples.scan(name)
            print("%-18s %d files -> %s" % (name, len(files), samples.list_path(name)))
    elif args.action == "files":
        for name in args.names:
            for f in samples.files(name, max_files=args.max):
                print(f)
    return 0


def cmd_submit(args):
    from harp import condor
    if args.resubmit:
        work = args.workdir or condor.default_work_dir(args.tag)
        return condor.submit(work, joblist="resubmit.txt", dry_run=args.dry_run)
    if not args.cfg or not args.sample or not args.output_dir:
        raise SystemExit("harp submit needs --cfg, --sample and --output-dir (or --resubmit --tag TAG)")
    work, jobs = condor.prepare(args.cfg, args.sample, args.output_dir, args.tag, files_per_job=args.files_per_job,
                                max_files=args.max_files, options=args.opt, work_dir=args.workdir, flavour=args.flavour,
                                memory=args.memory, skim=not args.hist_only)
    print("[harp submit] %d job(s) prepared in %s" % (len(jobs), work))
    return condor.submit(work, dry_run=args.dry_run)


def cmd_check(args):
    from harp import condor
    work = args.workdir or condor.default_work_dir(args.tag)
    good, bad = condor.check(work)
    return 0 if not bad else 1


def cmd_merge(args):
    from harp import condor
    work = args.workdir or condor.default_work_dir(args.tag)
    return condor.merge(work, args.dest)


def cmd_inspect(args):
    return subprocess.call(["harpInspect"] + args.harp_args)


def main(argv=None):
    p = argparse.ArgumentParser(prog="harp", description="HARP: hadron analysis and response performance framework",
                                epilog="Run 'harp <command> -h' for the options of a command. Sources env/setup.sh first.")
    sub = p.add_subparsers(dest="command", metavar="command")

    r = sub.add_parser("run", help="run a process configuration (python -> JSON -> harpRun)",
                       description="Execute a configuration file and run harpRun on it. Everything after '--' goes to harpRun (e.g. -- -n 100 --dump).")
    r.add_argument("config", help="python configuration defining 'process'")
    r.add_argument("options", nargs="*", help="key=value options of the configuration (see 'harp run cfg.py help')")
    r.add_argument("--workdir", default="work", help="where the JSON is written (default: work)")
    r.add_argument("--dry-run", action="store_true", help="only write the JSON")
    r.set_defaults(func=cmd_run)

    d = sub.add_parser("dump", help="print the JSON of a configuration")
    d.add_argument("config")
    d.add_argument("options", nargs="*")
    d.set_defaults(func=cmd_dump)

    pl = sub.add_parser("plot", help="run a plot configuration (python -> JSON -> harpPlot)")
    pl.add_argument("config", help="python configuration defining 'job'")
    pl.add_argument("options", nargs="*")
    pl.add_argument("--workdir", default="work")
    pl.add_argument("--dry-run", action="store_true")
    pl.set_defaults(func=cmd_plot)

    s = sub.add_parser("samples", help="list / scan samples, print file lists")
    s.add_argument("action", choices=["list", "scan", "files"])
    s.add_argument("names", nargs="*", help="sample names")
    s.add_argument("--max", type=int, default=-1, help="max files to print")
    s.set_defaults(func=cmd_samples)

    sb = sub.add_parser("submit", help="submit condor jobs", description="Example: harp submit --cfg configs/hist_hadron_cfg.py --sample pip_p1to200 Kp_p1to200 --output-dir /eos/user/u/user/HARP/v1 --tag v1 --files-per-job 10 --hist-only")
    sb.add_argument("--cfg", help="configuration under configs/")
    sb.add_argument("--sample", nargs="+", help="sample name(s)")
    sb.add_argument("--output-dir", help="where outputs are copied (EOS path ok)")
    sb.add_argument("--tag", required=True, help="name of the submission (work dir ~/condor_work/HARP/<tag>)")
    sb.add_argument("--files-per-job", type=int, default=10)
    sb.add_argument("--max-files", type=int, default=-1, help="limit files per sample (tests)")
    sb.add_argument("--opt", action="append", default=[], help="key=value option passed to the configuration (repeatable)")
    sb.add_argument("--hist-only", action="store_true", help="the configuration has no outputFile option (hist job)")
    sb.add_argument("--flavour", default="workday", help="condor JobFlavour")
    sb.add_argument("--memory", default="2500MB")
    sb.add_argument("--workdir", help="override the work directory (must not be on EOS)")
    sb.add_argument("--resubmit", action="store_true", help="resubmit the jobs listed in resubmit.txt (after harp check)")
    sb.add_argument("--dry-run", action="store_true", help="prepare everything but do not call condor_submit")
    sb.set_defaults(func=cmd_submit)

    c = sub.add_parser("check", help="verify the outputs of a submission, write resubmit.txt")
    c.add_argument("--tag", required=True)
    c.add_argument("--workdir")
    c.set_defaults(func=cmd_check)

    m = sub.add_parser("merge", help="hadd the outputs of a submission per sample")
    m.add_argument("--tag", required=True)
    m.add_argument("--workdir")
    m.add_argument("--dest", help="destination directory (default: <output-dir>/merged)")
    m.set_defaults(func=cmd_merge)

    i = sub.add_parser("inspect", help="inspect a nano-like file (harpInspect)")
    i.set_defaults(func=cmd_inspect)

    argv = sys.argv[1:] if argv is None else list(argv)
    # split "-- harpRun options"
    harp_args = []
    if "--" in argv:
        idx = argv.index("--")
        harp_args = argv[idx + 1:]
        argv = argv[:idx]
    if argv and argv[0] == "inspect":
        harp_args = argv[1:]
        argv = argv[:1]
    args = p.parse_args(argv)
    args.harp_args = harp_args
    if not args.command:
        p.print_help()
        return 1
    return args.func(args)
