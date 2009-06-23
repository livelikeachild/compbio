#!/usr/bin/env python

from rasmus import util, blast
import sys, time, os

options = [
 ["p:", "prog=", "prog", "<program>"],
 ["d:", "database=", "database", "<database fasta>"],
 ["i:", "query=", "query", "<query fasta>"],
 ["o:", "out=", "out", "<out file>"],
 ["s:", "split=", "split", "<# sequences>"],
 ["r", "resume", "resume", ""],
 ["l:", "log=", "log", "<log file>"],
]



param = util.parseOptions(sys.argv, options, quit=True, resthelp="options...")



# parse options
param.setdefault("out", ["-"])

if "split" in param:
    split = int(param["split"][-1])
else:
    split = 100

if "resume" in param and \
    param["out"][-1] != "-" and \
    os.path.isfile(param["out"][-1]) and \
    util.filesize(param["out"][-1]) != 0:
    
    infile = os.popen("tail -n1 %s" % param["out"][-1], "r")
    resume = infile.next().split()[0]
    out = util.open_stream(param["out"][-1], "a")
    infile.close()
else:
    resume = None
    out = util.open_stream(param["out"][-1], "w")


if "log" in param:
    util.globalTimer().addStream(util.open_stream(param["log"][-1], "a"))


# execute blast
util.tic("BLAST")
util.log("started", time.asctime())
util.log("executed with arguments:", " ".join(sys.argv[1:]))

reader = blast.blast(param["prog"][-1],
                     param["database"][-1], param["query"][-1], split=split,
                     resume=resume, options=" ".join(param["REST"]))

blast.filterBestHitPerTarget(reader, out)

util.log("finished ", time.asctime())
util.toc()
sys.stderr.flush()
sys.stdout.flush()

