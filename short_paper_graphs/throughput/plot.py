import numpy as np
import matplotlib as mpl
import matplotlib.pyplot as plt
import matplotlib.colors as mcolors
import pandas as pd

DATASET_KEY="dataset"
MACHINE_KEY="machine"
REDUCER_KEY="reducer"
HASH_KEY="hash"
THROUGHPUT_KEY="throughput"

DO_NOTHING="do_nothing"
CLAMP="clamp"

colors = mcolors.CSS4_COLORS

def plot_classical_throughput():
    csv = pd.read_csv(f"throughput.csv")
    partial_data = csv[csv[DATASET_KEY].isnull()]
    data = csv[csv[DATASET_KEY].notnull()]

    datasets = sorted(set(data[DATASET_KEY]))

    # Extract information
    machine = set(data[MACHINE_KEY]).pop()
    processor = machine[machine.find("(")+1:machine.rfind(")")]

    # Use do_nothing entries to determine order
    do_nothing_data = data[data[REDUCER_KEY] == DO_NOTHING].sort_values(THROUGHPUT_KEY)
    all_hashfns = list(dict.fromkeys(do_nothing_data[HASH_KEY])) # preserves order since python 3.7
    all_reducers = set(data[(data[REDUCER_KEY] != DO_NOTHING) &
        (data[REDUCER_KEY] != CLAMP)][REDUCER_KEY])

    fig, axs = plt.subplots(nrows=len(all_reducers), ncols=1, sharex=True,
            sharey=False, figsize=(15,20))
    plt.suptitle(f"Throughput (various datasets and compilers)\n{processor}\n")

    # Aggregate data over multiple datasets
    for i, reducer in enumerate(all_reducers):
        ax = axs[i]
        d = data[data[REDUCER_KEY] == reducer]

        hashfns = [hfn for hfn in all_hashfns if hfn in set(d[HASH_KEY])]

        bars = {}
        for hashfn in hashfns:
            bars[hashfn] = list(d[d[HASH_KEY] == hashfn][THROUGHPUT_KEY])

        plt_labels = [s.strip() for s in hashfns]
        plt_values = [bars[hf] for hf in hashfns] 

        # Plt data
        ax.boxplot(plt_values, whis=(0,100))
        ax.title.set_text(f"Throughput {reducer}")
        ax.set_ylim([0, 30])

        # Plot style/info
        plt.xticks(range(1,len(plt_labels)+1), plt_labels, rotation=45, ha="right", fontsize=8)
        plt.ylabel("ns per key")
        plt.tight_layout()

    plt.savefig(f"out/throughput_classical.pdf")

def plot_learned_throughput():
    csv = pd.read_csv(f"throughput.csv")
    partial_data = csv[csv[DATASET_KEY].isnull()]
    data = csv[csv[DATASET_KEY].notnull()]

    datasets = sorted(set(data[DATASET_KEY]))

    # Extract information
    machine = set(data[MACHINE_KEY]).pop()
    processor = machine[machine.find("(")+1:machine.rfind(")")]

    # Use do_nothing entries to determine order
    hashfns = list(dict.fromkeys(data[data[REDUCER_KEY] == CLAMP][HASH_KEY])) # preserves order since python 3.7

    fig, ax = plt.subplots()
    plt.suptitle(f"Throughput (various datasets and compilers)\n{processor}\n")

    # Aggregate data over multiple datasets
    d = data[data[REDUCER_KEY] == CLAMP]

    bars = {}
    for hashfn in hashfns:
        bars[hashfn] = list(d[d[HASH_KEY] == hashfn][THROUGHPUT_KEY])

    hashfns_ordered = sorted(hashfns, key=lambda h: max(bars[h]))
    plt_labels = [h.strip() for h in hashfns_ordered]
    plt_values = [bars[h] for h in hashfns_ordered] 

    # Plt data
    ax.boxplot(plt_values, whis=(0,100))
    ax.title.set_text(f"Throughput learned hash functions")
    ax.set_ylim([0, 400])

    # Plot style/info
    plt.xticks(range(1,len(plt_labels)+1), plt_labels, rotation=45, ha="right", fontsize=8)
    plt.ylabel("ns per key")
    plt.tight_layout()

    plt.savefig(f"out/throughput_learned.pdf")

plot_classical_throughput()
plot_learned_throughput()
