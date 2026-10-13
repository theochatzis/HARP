"""Sample registry: samples/*.json describe where the nanos live and what they
contain (pdgId, energy range, legend label, colour). File lists are cached in
samples/files/<name>.txt by ``harp samples scan``.

    from harp import samples
    samples.files("pip_p1to200", max_files=5)
    samples.info("pip_p1to200")["pdgId"]   # 211
"""
import glob
import json
import os

HARP_BASE = os.environ.get("HARP_BASE", os.path.abspath(os.path.join(os.path.dirname(__file__), "..", "..")))
SAMPLES_DIR = os.path.join(HARP_BASE, "samples")
FILES_DIR = os.path.join(SAMPLES_DIR, "files")


def registry():
    """Merge all samples/*.json into one dict name -> info."""
    out = {}
    for path in sorted(glob.glob(os.path.join(SAMPLES_DIR, "*.json"))):
        with open(path) as f:
            data = json.load(f)
        group = os.path.splitext(os.path.basename(path))[0]
        for name, info in data.items():
            info = dict(info)
            info.setdefault("group", group)
            info["name"] = name
            out[name] = info
    return out


def info(name):
    reg = registry()
    if name not in reg:
        raise KeyError("unknown sample '%s'. Known: %s" % (name, ", ".join(sorted(reg))))
    return reg[name]


def names(group=None):
    return [n for n, i in registry().items() if group is None or i.get("group") == group]


def scan(name, write=True):
    """List the ROOT files of a sample from its 'path' (+ 'pattern') and cache them."""
    i = info(name)
    files = sorted(glob.glob(os.path.join(i["path"], i.get("pattern", "*.root"))))
    if not files:
        raise RuntimeError("no files found for sample %s in %s" % (name, i["path"]))
    if write:
        os.makedirs(FILES_DIR, exist_ok=True)
        with open(list_path(name), "w") as f:
            f.write("\n".join(files) + "\n")
    return files


def list_path(name):
    return os.path.join(FILES_DIR, name + ".txt")


def files(name, max_files=-1, first=0):
    """Files of a sample: from the cached list if present, else scanned now."""
    lp = list_path(name)
    if os.path.exists(lp):
        with open(lp) as f:
            fl = [l.strip() for l in f if l.strip() and not l.startswith("#")]
    else:
        fl = scan(name, write=False)
    fl = fl[first:]
    if max_files >= 0:
        fl = fl[:max_files]
    return fl


def resolve_inputs(input_files, sample, max_files=-1, first=0):
    """Helper for configuration files: explicit files win, else the sample."""
    if input_files:
        return list(input_files)
    if sample:
        return files(sample, max_files=max_files, first=first)
    return []
