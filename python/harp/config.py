"""Process / Module / VarParsing: python description of a HARP job.

A configuration file is an ordinary python script that defines a variable
``process`` (see configs/*.py). ``harp run`` executes it, calls
``process.to_json()`` and hands the result to ``harpRun``.
"""
import json
import os
import sys
from collections import OrderedDict


class Module:
    """A module instance: C++ type plus its parameters (any JSON-serialisable values)."""

    def __init__(self, type_, **params):
        self.type = type_
        self.params = OrderedDict(params)

    def __getattr__(self, name):
        params = self.__dict__.get("params")
        if params is not None and name in params:
            return params[name]
        raise AttributeError(name)

    def __setattr__(self, name, value):
        if name in ("type", "params"):
            object.__setattr__(self, name, value)
        else:
            self.params[name] = value

    def clone(self, **overrides):
        m = Module(self.type, **self.params)
        for k, v in overrides.items():
            m.params[k] = v
        return m

    def to_dict(self):
        d = OrderedDict()
        d["@type"] = self.type
        d.update(self.params)
        return d

    def __repr__(self):
        return "Module(%s, %s)" % (self.type, ", ".join("%s=%r" % kv for kv in self.params.items()))


class Process:
    """Container of modules + path + input/output settings. Attributes that are
    Modules become entries of the "modules" block; ``process.path`` is the
    ordered list of module labels to run (filters stop the path)."""

    def __init__(self, name, files=None, treeName="Events", maxEvents=-1, skipEvents=0,
                 histFile="hists.root", logLevel="INFO", reportEvery=1000):
        object.__setattr__(self, "_modules", OrderedDict())
        self.name = name
        self.input = OrderedDict(files=list(files or []), treeName=treeName, maxEvents=int(maxEvents), skipEvents=int(skipEvents))
        self.output = OrderedDict(histFile=histFile)
        self.options = OrderedDict(logLevel=logLevel, reportEvery=int(reportEvery), pluginDirs=[])
        self.path = []

    # --- module registration through attribute access -----------------------
    def __setattr__(self, name, value):
        if isinstance(value, Module):
            self._modules[name] = value
        else:
            object.__setattr__(self, name, value)

    def __getattr__(self, name):
        mods = self.__dict__.get("_modules", {})
        if name in mods:
            return mods[name]
        raise AttributeError("process has no attribute or module '%s'" % name)

    def __delattr__(self, name):
        if name in self._modules:
            del self._modules[name]
            if name in self.path:
                self.path.remove(name)
        else:
            object.__delattr__(self, name)

    def modules(self):
        return self._modules

    def add_to_path(self, *labels):
        """Append labels (or Module objects already attached) to the path."""
        for l in labels:
            if isinstance(l, Module):
                l = self._label_of(l)
            if l not in self._modules:
                raise KeyError("module '%s' is not attached to the process" % l)
            self.path.append(l)

    def _label_of(self, module):
        for k, v in self._modules.items():
            if v is module:
                return k
        raise KeyError("module object not attached to the process")

    # --- serialisation -------------------------------------------------------
    def to_dict(self):
        for l in self.path:
            if l not in self._modules:
                raise KeyError("path refers to unknown module '%s'" % l)
        return OrderedDict([
            ("process", self.name),
            ("input", self.input),
            ("output", self.output),
            ("options", self.options),
            ("modules", OrderedDict((k, v.to_dict()) for k, v in self._modules.items())),
            ("path", list(self.path)),
        ])

    def to_json(self, indent=2):
        return json.dumps(self.to_dict(), indent=indent)

    def dump(self, path):
        with open(path, "w") as f:
            f.write(self.to_json())
        return path

    def summary(self):
        lines = ["Process %s" % self.name, "  input: %d file(s), maxEvents=%s" % (len(self.input["files"]), self.input["maxEvents"]),
                 "  histFile: %s" % self.output["histFile"], "  path:"]
        for l in self.path:
            lines.append("    %-24s %s" % (l, self._modules[l].type))
        return "\n".join(lines)


class VarParsing:
    """Command line key=value options for configuration files (cmsRun style).

        options = VarParsing()
        options.register("inputFiles", [], list, "input files")
        options.register("maxEvents", -1, int, "events to process")
        options.parse()           # reads sys.argv[1:] entries of the form key=value
        options.inputFiles        # -> list

    List options accept comma separated values; a value starting with '@' is a
    text file with one entry per line; ``sample=NAME`` can be resolved through
    harp.samples in the configuration itself.
    """

    def __init__(self):
        self._opts = OrderedDict()
        self._values = {}

    def register(self, name, default, type_=None, help_=""):
        if type_ is None:
            type_ = type(default) if default is not None else str
        self._opts[name] = (default, type_, help_)
        self._values[name] = default

    def parse(self, argv=None):
        argv = sys.argv[1:] if argv is None else argv
        for a in argv:
            if a in ("-h", "--help", "help"):
                print(self.help())
                sys.exit(0)
            if "=" not in a:
                raise SystemExit("VarParsing: expected key=value, got '%s'\n%s" % (a, self.help()))
            key, value = a.split("=", 1)
            if key not in self._opts:
                raise SystemExit("VarParsing: unknown option '%s'\n%s" % (key, self.help()))
            self._values[key] = self._convert(key, value)
        return self

    def _convert(self, key, value):
        default, type_, _ = self._opts[key]
        if type_ is list:
            if value.startswith("@"):
                with open(value[1:]) as f:
                    return [l.strip() for l in f if l.strip() and not l.startswith("#")]
            return [v for v in value.split(",") if v]
        if type_ is bool:
            return value.lower() in ("1", "true", "yes", "on")
        return type_(value)

    def help(self):
        lines = ["Options (key=value):"]
        for k, (d, t, h) in self._opts.items():
            lines.append("  %-16s %-6s default=%-24r %s" % (k, t.__name__, d, h))
        return "\n".join(lines)

    def __getattr__(self, name):
        values = self.__dict__.get("_values", {})
        if name in values:
            return values[name]
        raise AttributeError(name)

    def __getitem__(self, name):
        return self._values[name]


def load_config(path, argv=()):
    """Execute a python configuration file and return its ``process`` object."""
    path = os.path.abspath(path)
    g = {"__file__": path, "__name__": "__harp_config__"}
    old_argv = sys.argv
    sys.argv = [path] + list(argv)
    sys.path.insert(0, os.path.dirname(path))
    try:
        with open(path) as f:
            code = compile(f.read(), path, "exec")
        exec(code, g)
    finally:
        sys.argv = old_argv
        sys.path.pop(0)
    obj = g.get("process") or g.get("job")
    if obj is None:
        raise RuntimeError("%s must define a 'process' (harp.config.Process) or 'job' (harp.plotcfg.PlotJob)" % path)
    return obj
