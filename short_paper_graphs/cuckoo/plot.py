import numpy as np
import matplotlib as mpl
import matplotlib.patches as mpatches
import matplotlib.pyplot as plt
import matplotlib.colors as mcolors
import pandas as pd
import math as math

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
hr_names = {
        "radix_spline": "RadixSpline", 
       # "pgm_hash_eps128": "pgm_hash", 
       # "mult_prime64": "mult", 
       # "mult_add64": "mult_add", 
        "murmur_finalizer64": "Murmur"
        }
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

colors = {"RadixSpline": "tab:blue", "Murmur": "tab:orange"}
letter = [["A", "B"], ["C", "D"]]

# Generate plot
fig, axs = plt.subplots(2,2,figsize=(7.00697/2,2),sharex=True,sharey=True)
for l, load_factor in enumerate([0.95, 0.98]):
    for s, kicking_strat in enumerate(["balanced_kicking", "biased_kicking_10"]):
        ax = axs[l][s]
        ax.set_title(f"({letter[l][s]}) {load_factor}, {kicking_strat[0:kicking_strat.find('_')]} kicking", fontsize=6)

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
                   # (data[HASH_KEY].str.match("mult_prime64")) |
                   # (data[HASH_KEY].str.match("mult_add64")) |
                    (data[HASH_KEY].str.match("murmur_finalizer64")) |
                   # (data[HASH_KEY].str.contains("rmi")) |
                    (data[HASH_KEY].str.match("radix_spline")) #|
                   # (data[HASH_KEY].str.match("pgm"))
                   )
                ]

        all_hashfns = list(set(d[d[REDUCER_KEY].str.match(CLAMP)][HASH_KEY])) + ["murmur_finalizer64"]

        # Only one datapoint for murmur
        murmur_datapoints = list()
        other_datapoints = list()
        for (d, p, m, h) in list(zip(d[DATASET_KEY], d[PRIMARY_KEY_RATIO_KEY], d[MEDIAN_PROBE_TIME_KEY], d[HASH_KEY])):
            if name(h) == "Murmur":
                murmur_datapoints.append((d, p, m, h))
            else:
                other_datapoints.append((d, p, m, h))
        murmur_datapoints = sorted(murmur_datapoints, key=lambda t: t[2])
        murmur_datapoint = murmur_datapoints[int(len(murmur_datapoints)/2)]

        for (dataset, primary_key_ratio, median_probe_time, hashfn) in other_datapoints + [murmur_datapoint]:
            hash_name = name(hashfn)
            dataset_name = name_d(dataset)

            ax.scatter(primary_key_ratio, median_probe_time, c=colors.get(hash_name), marker='.', s=10)

            def x_adjust():
                return 0
            def y_adjust():
                if dataset_name == "osm":
                    return -75 if s == 1 and p == 0 else 40
                if dataset_name == "fb":
                    return -100
                return +20
            def ha():
                if dataset_name in {"seq", "gap 1%", "gap 10%", "wiki"}:
                    return 'right'
                return 'center'
            def va():
                return 'baseline'
            def rotation():
                if dataset_name in {"seq", "gap 1%", "gap 10%", "wiki"}:
                    return -45
                return 0


            if hash_name != "Murmur":
                ax.annotate(
                        f"{dataset_name}", 
                        (primary_key_ratio + x_adjust(), median_probe_time + y_adjust()), 
                        fontsize=5, 
                        ha=ha(),
                        va=va(),
                        rotation=rotation())

        # Plot style/info
        ax.set_xticks([0.4, 0.6, 0.8, 1.0])
        ax.tick_params(axis='both', which='major', labelsize=8)

        # Legend 
        if l == 1 and s == 1:
            ax.legend(handles=[mpatches.Patch(color=colors.get(name(h)), label=name(h)) for h,_ in hr_names.items()],
                loc="lower left",
                fontsize=5)

fig.text(0.5, 0.02, 'Primary key ratio [percent]', ha='center', fontsize=8)
fig.text(0.01, 0.5, 'Probe time per key [ns]', va='center', rotation='vertical', fontsize=8)

plt.tight_layout()
plt.subplots_adjust(left=0.125, bottom=0.175, wspace=0.15, hspace=0.5)
plt.savefig(f"out/cuckoo.pdf")
