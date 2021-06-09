import numpy as np
import matplotlib as mpl
import matplotlib.markers as mmark
import matplotlib.patches as mpatches
from matplotlib.lines import Line2D
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
hr_names = {"radix_spline": "RadixSpline",
        "mult_prime64": "Mult", "mult_add64": "MultAdd", 
        "murmur_finalizer64": "Murmur"}
colors = {"RadixSpline": "tab:blue", "Murmur": "tab:orange", "Mult": "black", "MultAdd": "gray"}
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
LOAD_FACTOR_KEY="load_factor"
PAYLOAD_SIZE_KEY="payload_size"
SLOTS_PER_BUCKET_KEY="slots_per_bucket"
SAMPLE_SIZE_KEY="sample_size"
ADDITIONAL_BUCKETS_KEY="additional_buckets_per_key"
MEDIAN_PROBE_TIME_KEY="median_probe_time"

CLAMP="clamp"
FASTMOD="fast_modulo"

csv = pd.read_csv(f"chained.csv")
partial_data = csv[csv[DATASET_KEY].isnull()]
data = csv[csv[DATASET_KEY].notnull()]

# Generate plot
letter = [["A", "B"], ["C", "D"]]
fig, axs = plt.subplots(2,2,figsize=(7.00697/2,2), sharex=True, sharey=True)
for p, payload_size in enumerate(set(data[PAYLOAD_SIZE_KEY])):
    for s, slots_per_bucket in enumerate(set(data[SLOTS_PER_BUCKET_KEY])):
        ax = axs[p][s]
        ax.set_title(f"({letter[p][s]}) {payload_size}B payload {slots_per_bucket} slots", fontsize=6)

        # Filter data
        d = data[
                # Match slots_per_bucket & payload_size
                (data[SLOTS_PER_BUCKET_KEY] == slots_per_bucket)
                & (data[PAYLOAD_SIZE_KEY] == payload_size)
                # Only use g++ results
                & ((data[COMPILER_KEY].isnull()) | (data[COMPILER_KEY].str.match(r"g\+\+")))
                # Only use load factor 1 results
                & (data[LOAD_FACTOR_KEY] == 1.0)
                # Only use fast modulo results
                & ((data[REDUCER_KEY] == FASTMOD) | (data[REDUCER_KEY] == CLAMP))
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
                   #(data[HASH_KEY] == "mult_prime64")
                   #| (data[HASH_KEY] == "mult_add64")
                    (data[HASH_KEY] == "murmur_finalizer64")
                   # | (data[HASH_KEY].str.contains("rmi"))
                    | (data[HASH_KEY].str.match("radix_spline"))
                   # | (data[HASH_KEY].str.match("pgm"))
                    )
                ]

        # dict preserves insertion order since python 3.7
        all_hashfns = list(set(d[d[REDUCER_KEY].str.match(CLAMP)][HASH_KEY])) + ["mult_prime64", "mult_add64", "murmur_finalizer64"]

        # Only one datapoint for murmur
        murmur_datapoints = list()
        other_datapoints = list()
        for (d, a, m, h) in list(zip(d[DATASET_KEY], d[ADDITIONAL_BUCKETS_KEY], d[MEDIAN_PROBE_TIME_KEY], d[HASH_KEY])):
            if name(h) == "Murmur":
                murmur_datapoints.append((d, a, m, h))
            else:
                other_datapoints.append((d, a, m, h))
        murmur_datapoints = sorted(murmur_datapoints, key=lambda t: t[2])
        murmur_datapoint = murmur_datapoints[int(len(murmur_datapoints)/2)]

        for (dataset, additional_buckets, median_probe_time, hashfn) in other_datapoints + [murmur_datapoint]:
            hash_name = name(hashfn)
            dataset_name = name_d(dataset)

            if hash_name == "Murmur":
                ax.scatter(additional_buckets, median_probe_time, marker='+', s=15, c=colors.get(hash_name), linewidth=0.8)
            else:       
                ax.scatter(additional_buckets, median_probe_time, marker=markers.get(dataset), s=11, facecolors='none', edgecolors=colors.get(hash_name), linewidths=0.5)

        # Plot style/info
        ax.set_yscale('log')
        ax.set_xlim(-0.1,1.1)
        ax.set_xticks([0.0, 0.33, 0.66, 1.0])
        ax.tick_params(axis='both', which='major', labelsize=8)
        ax.tick_params(axis='both', which='minor', labelsize=6)

        if p == 0 and s == 1:
            ax.legend(
                handles=[mpatches.Patch(color=colors.get(name(h)), label=name(h)) for h,_ in hr_names.items()],
                loc="upper right",
                fontsize=5,
                ncol=1,
                labelspacing=0.15,
                handlelength=1.0,
                columnspacing=0.1)
        if p == 1 and s == 1:
            ax.legend(
                    handles=[Line2D([0], [0], marker=m, color='w', label=name_d(d), markerfacecolor='none', markeredgecolor="black", markeredgewidth=0.5,markersize=2) for d, m in markers.items()],
                    fontsize=5,
                    markerscale=2,
                    labelspacing=0.15,
                    columnspacing=0.1,
                    )


fig.text(0.5, 0.02, 'Additional buckets per key [percent]', ha='center', fontsize=8)
fig.text(0.01, 0.5, 'Probe time per key [ns]', va='center', rotation='vertical', fontsize=8)

plt.tight_layout()
plt.subplots_adjust(left=0.20, bottom=0.175, wspace=0.1, hspace=0.45)

plt.savefig(f"out/chained.pdf")
