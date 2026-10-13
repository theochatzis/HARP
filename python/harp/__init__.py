"""HARP python layer: configuration management for the C++ framework.

The C++ executables (harpRun, harpPlot) consume JSON. These modules build that
JSON from python configuration files, in the spirit of CMSSW's cms.Process:

    from harp.config import Process, Module, VarParsing
    options = VarParsing(); options.register("inputFiles", [...]); options.parse()
    process = Process("HADRONHIST", files=options.inputFiles, histFile=options.histFile)
    process.hgcalReader = Module("HGCalNanoReader", genParticles="HGCalGenPart", ...)
    process.path = ["hgcalReader", ...]
"""
__version__ = "0.1.0"
