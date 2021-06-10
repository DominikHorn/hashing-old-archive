import numpy as np
import matplotlib as mpl
import matplotlib.patches as mpatches
from matplotlib.lines import Line2D
import matplotlib.pyplot as plt
import matplotlib.colors as mcolors
import pandas as pd
import math as math
from mpl_toolkits.axes_grid1 import make_axes_locatable

# ====================
#      Plot plan
# ====================
# primary key ratio (x axis) vs probe time (y axis)

# Latex figure export
mpl.use("pgf")
mpl.rcParams.update({
    "pgf.texsystem": "pdflatex",
    "font.family": "serif",
    "text.usetex": True,
    "pgf.rcfonts": False
})

# Style
hr_names = {"radix_spline": "RadixSpline",
        #"mult_prime64": "Mult", "mult_add64": "MultAdd", 
        "murmur_finalizer64": "Murmur",
        "aqua0_64": "AquaHash",
        "xxh3": "XXH3"}
colors = {"RadixSpline": "tab:blue", "Murmur": "tab:orange", "AquaHash": "tab:red", "XXH3": "tab:brown"}
markers = {"seq_200M_uint64": ".","gap_1%_200M_uint64": "s", "gap_10%_200M_uint64": "h", "wiki_200M_uint64": "D", "fb_200M_uint64": "X", "osm_200M_uint64": "^"}

def name_d(dataset):
    x = dataset
    return x.replace(r"_200M", "").replace("_uint64", "").replace("_", " ")

def name(hashfn):
    if hashfn.startswith("radix_spline"):
        hashfn = "radix_spline"
    return hr_names.get(hashfn) or hashfn

DATASET_KEY="dataset"
MACHINE_KEY="machine"
COMPILER_KEY="compiler"
REDUCER_KEY="reducer"
HASH_KEY="hash"
KICKING_STRAT_KEY="kicking_strategy"
PAYLOAD_SIZE_KEY="payload"
LOAD_FACTOR_KEY="load_factor"
SAMPLE_SIZE_KEY="sample_size"
PRIMARY_KEY_RATIO_KEY="primary_key_ratio"
MEDIAN_PROBE_TIME_KEY="median_probe_ns"

CLAMP="clamp"
FASTMOD="fast_modulo"

csv = pd.read_csv(f"cuckoo.csv")
partial_data = csv[csv[DATASET_KEY].isnull()]
data = csv[csv[DATASET_KEY].notnull()]

# Generate plot
letter = [["A", "B"], ["C", "D"]]
ylims = [[[(220,280), (410,700)], [(220,280), (410,700)]], [[(220,280), (350,830)], [(220,280), (350,850)]]]
fig, axs = plt.subplots(2,2,figsize=(7.00697/2,2.5),sharex=True,sharey=False)
for l, load_factor in enumerate([0.95, 0.98]):
    for s, kicking_strat in enumerate(["balanced_kicking", "biased_kicking_10"]):
        ax = axs[l][s]

        # break y-axis
        divider = make_axes_locatable(ax)
        ax2 = divider.new_vertical(size="100%", pad=0.1)
        fig.add_axes(ax2)

        # tune this for each plot
        ylim1 = ylims[l][s][0]
        ax.set_ylim(ylim1[0],ylim1[1])
        ax.set_xlim(0.35, 1.05)
        ax.set_xticks([0.4, 0.6, 0.8, 1.0])
        ax.tick_params(axis='both', which='major', labelsize=8)
        ax.spines['top'].set_visible(False)
        ylim2 = ylims[l][s][1]
        ax2.set_ylim(ylim2[0],ylim2[1])
        ax2.tick_params(bottom=False, labelbottom=False)
        ax2.set_xlim(-0.1,1.1)
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

        ax2.set_title(f"({letter[l][s]}) {load_factor}, {kicking_strat[0:kicking_strat.find('_')]} kicking", fontsize=6)

        # Filter data
        d = data[
                # Only use g++ results
                ((data[COMPILER_KEY].isnull()) | (data[COMPILER_KEY].str.match(r"g\+\+")))
                # Only use fast modulo and clamp results
                & ((data[REDUCER_KEY].str.match(FASTMOD)) | (data[REDUCER_KEY].str.match(CLAMP)))
                # Restrict load_fac & kicking strat
                & (data[LOAD_FACTOR_KEY] == load_factor)
                & (data[KICKING_STRAT_KEY] == kicking_strat)
                # Only use 16 byte payload numbers
                & (data[PAYLOAD_SIZE_KEY] == 16)
                # Only use these select datasets for now
                & (
                    (data[DATASET_KEY] == "seq_200M_uint64")
                    | (data[DATASET_KEY] == "gap_1%_200M_uint64")
                    | (data[DATASET_KEY] == "gap_10%_200M_uint64")
                    #| (data[DATASET_KEY] == "uniform_200M_uint64")
                    | (data[DATASET_KEY] == "wiki_200M_uint64")
                    | (data[DATASET_KEY] == "fb_200M_uint64")
                    | (data[DATASET_KEY] == "osm_200M_uint64")
                )
                # Only use certain hash functions
                & (
                    (data[HASH_KEY].str.match("murmur_finalizer64"))
                    | (data[HASH_KEY] == "aqua0_64")
                    | (data[HASH_KEY] == "xxh3")
                   # | (data[HASH_KEY].str.contains("rmi"))
                    | (data[HASH_KEY].str.match("radix_spline"))
                   # | (data[HASH_KEY].str.match("pgm"))
                   )
                ]

        all_hashfns = list(set(d[d[REDUCER_KEY].str.match(CLAMP)][HASH_KEY])) + ["mult_prime64", "mult_add64", "murmur_finalizer64"]

        # Only one datapoint for murmur
        murmur_datapoints = list()
        aqua_datapoints = list()
        xxh_datapoints = list()
        other_datapoints = list()
        for (d, p, m, h) in list(zip(d[DATASET_KEY], d[PRIMARY_KEY_RATIO_KEY], d[MEDIAN_PROBE_TIME_KEY], d[HASH_KEY])):
            if name(h) == "Murmur":
                murmur_datapoints.append((d, p, m, h))
            elif name(h) == "AquaHash":
                aqua_datapoints.append((d, p, m, h))
            elif name(h) == "XXH3":
                xxh_datapoints.append((d, p, m, h))
            else:
                other_datapoints.append((d, p, m, h))
        murmur_datapoints = sorted(murmur_datapoints, key=lambda t: t[2])
        murmur_datapoint = murmur_datapoints[int(len(murmur_datapoints)/2)]
        aqua_datapoints = sorted(aqua_datapoints, key=lambda t: t[2])
        aqua_datapoint = aqua_datapoints[int(len(murmur_datapoints)/2)]
        xxh_datapoints = sorted(xxh_datapoints, key=lambda t: t[2])
        xxh_datapoint = xxh_datapoints[int(len(murmur_datapoints)/2)]

        for (dataset, primary_key_ratio, median_probe_time, hashfn) in other_datapoints + [murmur_datapoint, aqua_datapoint, xxh_datapoint]:
            hash_name = name(hashfn)
            dataset_name = name_d(dataset)
            a = ax2 if dataset_name in {"fb", "osm"} else ax

            if hash_name == "Murmur":
                ax.scatter(primary_key_ratio, median_probe_time, marker='x', s=15, c=colors.get(hash_name), linewidth=0.8)
            elif hash_name == "AquaHash":
                ax.scatter(primary_key_ratio, median_probe_time, marker='1', s=15, c=colors.get(hash_name), linewidth=0.8)
            elif hash_name == "XXH3":
                ax.scatter(primary_key_ratio, median_probe_time, marker='2', s=15, c=colors.get(hash_name), linewidth=0.8)
            else:       
                a.scatter(primary_key_ratio, median_probe_time, marker=markers.get(dataset), s=11, facecolors='none', edgecolors=colors.get(hash_name), linewidths=0.5)

        # Legend 
        if l == 0 and s == 1:
            ax.legend(
                handles=[mpatches.Patch(color=colors.get(name(h)), label=name(h)) for h,_ in hr_names.items()],
                loc="lower left",
                fontsize=5,
                ncol=1,
                labelspacing=0.15,
                handlelength=1.0,
                columnspacing=0.1)
        if l == 1 and s == 1:
            ax2.legend(
                    handles=[Line2D([0], [0], marker=m, color='w', label=name_d(d), markerfacecolor='none', markeredgecolor="black", markeredgewidth=0.5,markersize=2) for d, m in markers.items()],
                    fontsize=5,
                    markerscale=2,
                    labelspacing=0.15,
                    columnspacing=0.1,
                    )

fig.text(0.5, 0.02, 'Primary key ratio [percent]', ha='center', fontsize=8)
fig.text(0.01, 0.5, 'Probe time per key [ns]', va='center', rotation='vertical', fontsize=8)

plt.tight_layout()
plt.subplots_adjust(left=0.15, bottom=0.15, wspace=0.3, hspace=0.3)
plt.savefig(f"out/cuckoo.pdf")
