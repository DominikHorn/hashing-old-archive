import numpy as np
import matplotlib as mpl
import matplotlib.patches as mpatches
import matplotlib.pyplot as plt
import matplotlib.colors as mcolors
import pandas as pd
import math as math
from mpl_toolkits.axes_grid1 import make_axes_locatable

# Latex figure export
mpl.use("pgf")
mpl.rcParams.update({
    "pgf.texsystem": "pdflatex",
    "font.family": "serif",
    "text.usetex": True,
    "pgf.rcfonts": False
})

# Style
hr_names = {
        "murmur_finalizer64": "Murmur",
        "radix_spline": "RadixSpline",
        }
all_palette = list(mcolors.TABLEAU_COLORS.keys())
palette = all_palette[:-1]
colors = {
        "murmur_finalizer64": palette[1],
        "radix_spline": palette[0],
        }

def name(hashfn):
    return hr_names.get(hashfn) or hashfn

def color(h):
    return colors.get(h) or all_palette[-1]

DATASET_KEY="dataset"
MACHINE_KEY="machine"
COMPILER_KEY="compiler"
REDUCER_KEY="reducer"
HASH_KEY="hash"
LOAD_FACTOR_KEY="load_factor"
SAMPLE_SIZE_KEY="sample_size"
MEDIAN_PROBE_KEY="median_probe_ns"
PAYLOAD_SIZE="payload_size"

CLAMP="clamp"
FASTMOD="fast_modulo"

def plot(payload_size):
    cuckoo_csv = pd.read_csv(f"cuckoo.csv")
    chained_csv = pd.read_csv(f"chained.csv")
    cuckoo_data = cuckoo_csv[cuckoo_csv[DATASET_KEY].notnull()]
    chained_data = chained_csv[chained_csv[DATASET_KEY].notnull()]

    # Filter data
    cuckoo_data = cuckoo_data[
            ((cuckoo_data[HASH_KEY] == "murmur_finalizer64") | (cuckoo_data[HASH_KEY].str.match("radix_spline")))
            & (cuckoo_data[PAYLOAD_SIZE] == payload_size)]
    chained_data = chained_data[
            ((chained_data[HASH_KEY] == "murmur_finalizer64") | (chained_data[HASH_KEY].str.match("radix_spline")))
            & (chained_data[PAYLOAD_SIZE] == payload_size)]
     
    # dict preserves insertion order since python 3.7
    classical_hashfns = [
            "murmur_finalizer64"] 
    learned_hashfns = list(dict.fromkeys(cuckoo_data[cuckoo_data[REDUCER_KEY].str.match(CLAMP)][HASH_KEY])) 
    all_hashfns = learned_hashfns + classical_hashfns

    # Aggregate data over multiple datasets
    datasets =  [
            # synthetic
            'seq_200M_uint64',
            'gap_1%_200M_uint64', 
            'gap_10%_200M_uint64', 
            #'uniform_dense_200M_uint64',

            # real
            'wiki_200M_uint64',
            'fb_200M_uint64', 
            'osm_200M_uint64']

    # Generate plot
    fig, ax = plt.subplots(figsize=(7.00697/2,2.3))

    # break y-axis
    divider = make_axes_locatable(ax)
    ax2 = divider.new_vertical(size="100%", pad=0.1)
    fig.add_axes(ax2)

    # tune this for each plot
    xticks=[i+0.5 for i in range(0, len(datasets))]
    ylim1 = [0,250] if payload_size == 16 else [0,350]
    ax.set_ylim(ylim1[0],ylim1[1])
    ax.tick_params(axis='both', which='major', labelsize=8)
    ax.set_xticks(xticks)
    ax.set_xticklabels([d.replace(r"_200M",
        "").replace("_uint64", "").replace("_", " ") for d in datasets],
        va="center_baseline",position=(0.5,-0.05), fontsize=11, rotation=35, ha="right")
    ax.spines['top'].set_visible(False)
    ylim2 = [400,800] if payload_size == 16 else [550,1000]
    ax2.set_ylim(ylim2[0],ylim2[1])
    ax2.tick_params(bottom=False, labelbottom=False)
    ax2.tick_params(axis='both', which='major', labelsize=8)
    ax2.spines['bottom'].set_visible(False)

    # From https://matplotlib.org/examples/pylab_examples/broken_axis.html
    d = .015  # how big to make the diagonal lines in axes coordinates
    # arguments to pass to plot, just so we don't keep repeating them
    kwargs = dict(transform=ax2.transAxes, color='k', clip_on=False)
    ax2.plot((-d, +d), (-d, +d), **kwargs)        # top-left diagonal
    ax2.plot((1 - d, 1 + d), (-d, +d), **kwargs)  # top-right diagonal

    kwargs.update(transform=ax.transAxes)  # switch to the bottom axes
    ax.plot((-d, +d), (1 - d, 1 + d), **kwargs)  # bottom-left diagonal
    ax.plot((1 - d, 1 + d), (1 - d, 1 + d), **kwargs)  # bottom-right diagonal

    for i, dataset in enumerate(datasets):
        cuckoo = cuckoo_data[cuckoo_data[DATASET_KEY] == dataset]
        chained = chained_data[chained_data[DATASET_KEY] == dataset]
        hashfns = all_hashfns

        cuckoo_bars = {}
        chained_bars = {}
        for hashfn in hashfns:
            cuckoo_bars[hashfn] = list(cuckoo[cuckoo[HASH_KEY] == hashfn][MEDIAN_PROBE_KEY])
            chained_bars[hashfn] = list(chained[chained[HASH_KEY] == hashfn][MEDIAN_PROBE_KEY])

        empty_space = 0.2
        bar_width = (0.8 - empty_space) / (len(cuckoo_bars.items()) + len(chained_bars.items()))
        gap_width = 0.05 #0.1 / len(plt_data)
        for j, (hname, probe_time) in enumerate(cuckoo_bars.items()):
            if len(probe_time) == 0:
                print("Missing datapoint:", hname, probe_time, "cuckoo", dataset)
                continue
            if len(probe_time) > 1:
                print("ERR, TO MANY DATAPOINTS FOR SINGLE BAR:", hname, probe_time, "cuckoo", dataset)
                exit()
            for a in {ax, ax2}:
                a.bar(i + j * (bar_width+gap_width), probe_time, bar_width, color=colors.get(hname) or "purple", edgecolor=colors.get(hname) or "purple")

        for k, (hname, probe_time) in enumerate(chained_bars.items()):
            if len(probe_time) == 0:
                print("Missing datapoint:", hname, probe_time, "chained", dataset)
                continue
            if len(probe_time) > 1:
                print("ERR, TO MANY DATAPOINTS FOR SINGLE BAR:", hname, probe_time, "chained", dataset)
                exit()
            j = k + len(cuckoo_bars.items())
            for a in {ax, ax2}:
                a.bar(i + j * (bar_width+gap_width), probe_time, bar_width, edgecolor=colors.get(hname) or "purple", fill=False)


    plt.tight_layout()
    plt.subplots_adjust(left=0.2)

    ax2.legend(
        handles=[mpatches.Patch(color=colors.get(h), label=name(h)) for h in all_hashfns] +
            [mpatches.Rectangle((0,0), 10, 5, color='black', label="Cuckoo"),
               mpatches.Rectangle((0,0), 10, 5, color='black', label="Chained", fill=False) ],
        loc="upper left",
        fontsize=6,
        labelspacing=0.1,
        ncol=2)


    fig.text(0.5, 0.02, 'Dataset', ha='center', fontsize=11)
    fig.text(0.01, 0.5, 'Probe time per key [ns]', va='center', rotation='vertical', fontsize=11)

    plt.savefig(f"out/probe_times_overall_{payload_size}.pdf")

plot(16)
plot(64)

