import numpy as np
import matplotlib as mpl
import matplotlib.pyplot as plt
import matplotlib.colors as mcolors
import pandas as pd

DATASET_KEY="dataset"
MACHINE_KEY="machine"
COMPILER_KEY="compiler"
REDUCER_KEY="reducer"
HASH_KEY="hash"
THROUGHPUT_KEY="throughput"

DO_NOTHING="do_nothing"
CLAMP="clamp"

colors = mcolors.CSS4_COLORS

csv = pd.read_csv(f"throughput.csv")
partial_data = csv[csv[DATASET_KEY].isnull()]
data = csv[csv[DATASET_KEY].notnull()]

datasets = sorted(set(data[DATASET_KEY]))
compilers = sorted(set(data[COMPILER_KEY]))

#
for compiler in compilers:
    d1 = data[data[COMPILER_KEY] == compiler]

    # Extract information
    machine = set(d1[MACHINE_KEY]).pop()
    processor = machine[machine.find("(")+1:machine.rfind(")")]

    # Use do_nothing entries to determine order
    do_nothing_data = data[(data[REDUCER_KEY] == DO_NOTHING) | (data[REDUCER_KEY] == CLAMP)].sort_values(THROUGHPUT_KEY)
    all_hashfns = list(dict.fromkeys(do_nothing_data[HASH_KEY])) # preserves order since python 3.7
    all_reducers = set(d1[d1[REDUCER_KEY] != DO_NOTHING][REDUCER_KEY])

    # Aggregate data over multiple datasets
    for i, reducer in enumerate(all_reducers):
        d = d1[d1[REDUCER_KEY] == reducer]

        # Generate plot
        fig, ax = plt.subplots()
        plt.suptitle(f"{compiler} @ {processor}")

        hashfns = [hfn for hfn in all_hashfns if hfn in set(d[HASH_KEY])]

        bars = {}
        for hashfn in hashfns:
            bars[hashfn] = list(d[d[HASH_KEY] == hashfn][THROUGHPUT_KEY])

        plt_labels = [s.strip() for s in hashfns]
        plt_values = [bars[hf] for hf in hashfns] 

        # Plt data
        ax.boxplot(plt_values, whis=(0,100))
        ax.title.set_text(f"Throughput {reducer}")
        ax.set_ylim([0, 300] if reducer == CLAMP else [0, 30])

        # Plot style/info
        plt.xticks(range(1,len(plt_labels)+1), plt_labels, rotation=45, ha="right", fontsize=8)
        plt.ylabel("ns per key")
        plt.tight_layout()

        plt.savefig(f"out/throughput_{reducer}_{compiler}.pdf")
