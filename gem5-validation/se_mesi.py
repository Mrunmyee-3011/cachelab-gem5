"""
GEM5 (v25.1) standard-library config: X86 multi-core, Ruby MESI_Two_Level, SE mode.

Run with a gem5 binary built with PROTOCOL=MESI_Two_Level (build/X86/gem5.opt, see README.md):

    build/X86/gem5.opt -d results/shared_4c  se_mesi.py --cores 4 --binary ./booking --iters 1000 --mode 0

Statistics are written to <outdir>/stats.txt.
"""
import argparse

from gem5.coherence_protocol import CoherenceProtocol
from gem5.components.boards.simple_board import SimpleBoard
from gem5.components.cachehierarchies.ruby.mesi_two_level_cache_hierarchy import (
    MESITwoLevelCacheHierarchy,
)
from gem5.components.memory import SingleChannelDDR3_1600
from gem5.components.processors.cpu_types import CPUTypes
from gem5.components.processors.simple_processor import SimpleProcessor
from gem5.isas import ISA
from gem5.resources.resource import BinaryResource
from gem5.simulate.simulator import Simulator
from gem5.utils.requires import requires

parser = argparse.ArgumentParser()
parser.add_argument("--cores", type=int, required=True, help="number of cores = number of threads")
parser.add_argument("--binary", required=True, help="path to the static booking binary")
parser.add_argument("--iters", type=int, default=1000)
parser.add_argument("--mode", type=int, choices=[0, 1], default=0, help="0=shared counter, 1=padded counters")
args = parser.parse_args()

requires(
    isa_required=ISA.X86,
    coherence_protocol_required=CoherenceProtocol.MESI_TWO_LEVEL,
)

cache_hierarchy = MESITwoLevelCacheHierarchy(
    l1d_size="32KiB", l1d_assoc=8,
    l1i_size="32KiB", l1i_assoc=8,
    l2_size="256KiB", l2_assoc=16,
    num_l2_banks=1,
)

board = SimpleBoard(
    clk_freq="3GHz",
    processor=SimpleProcessor(cpu_type=CPUTypes.TIMING, isa=ISA.X86, num_cores=args.cores),
    memory=SingleChannelDDR3_1600(size="512MiB"),
    cache_hierarchy=cache_hierarchy,
)

board.set_se_binary_workload(
    BinaryResource(local_path=args.binary),
    arguments=[str(args.cores), str(args.iters), str(args.mode)],
)

simulator = Simulator(board=board)
simulator.run()
print(f"Finished: cores={args.cores} iters={args.iters} mode={args.mode} tick={simulator.get_current_tick()}")
