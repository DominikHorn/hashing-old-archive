import numpy as np
import matplotlib as mpl
import matplotlib.patches as mpatches
import matplotlib.pyplot as plt
import matplotlib.colors as mcolors
import pandas as pd
import math as math

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

    # Generate plot
    fig, ax = plt.subplots(figsize=(7.00697/2,2.3))

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
            ax.bar(i + j * (bar_width+gap_width), probe_time, bar_width, color=colors.get(hname) or "purple", edgecolor=colors.get(hname) or "purple")

        for k, (hname, probe_time) in enumerate(chained_bars.items()):
            j = k + len(cuckoo_bars.items())
            ax.bar(i + j * (bar_width+gap_width), probe_time, bar_width, edgecolor=colors.get(hname) or "purple", fill=False)


    ## Plot style/info
    ymax=800 if payload_size == 16 else 1000
    yticks = np.linspace(0, ymax, 5)
    plt.ylim(0,ymax)
    plt.ylabel(f"Probe time [ns]", fontsize=15)

    plt.xticks([i+0.5 for i in range(0, len(datasets))], [d.replace(r"_200M",
        "").replace("_uint64", "").replace("_", " ") for d in datasets],
        va="center_baseline",position=(0.5,-0.05), fontsize=15, rotation=35, ha="right")
    plt.xlabel("Dataset", fontsize=15)

    #plt.margins(x=0.01,y=0.2)
    plt.tight_layout()
    plt.subplots_adjust(bottom=0.4, top=0.95)

    ax.legend(
        handles=[mpatches.Patch(color=colors.get(h), label=name(h)) for h in all_hashfns] +
            [mpatches.Rectangle((0,0), 10, 5, color='black', label="Cuckoo"),
               mpatches.Rectangle((0,0), 10, 5, color='black', label="Chained", fill=False) ],
        loc="upper left",
        fontsize=6,
        labelspacing=0.1,
        ncol=2)

    plt.savefig(f"out/probe_times_overall_{payload_size}.pdf")

plot(16)
plot(64)

