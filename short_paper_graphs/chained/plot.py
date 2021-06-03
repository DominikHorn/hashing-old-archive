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
    "text.usetex": False,
    "pgf.rcfonts": False
})

# Style
hr_names = {"radix_spline": "RadixSpline",
        #"mult_prime64": "mult", "mult_add64": "mult_add", 
        "murmur_finalizer64": "Murmur"}
def name(hashfn):
    if hashfn.startswith("radix_spline"):
        hashfn = "radix_spline"
    return hr_names.get(hashfn) or hashfn
def ds_name(d):
    return d

DATASET_KEY="dataset"
MACHINE_KEY="machine"
COMPILER_KEY="compiler"
REDUCER_KEY="reducer"
HASH_KEY="hash"
LOAD_FACTOR_KEY="load_factor"
PAYLOAD_SIZE_KEY="payload_size"
SLOTS_PER_BUCKET_KEY="slots_per_bucket"
SAMPLE_SIZE_KEY="sample_size"
MEDIAN_PROBE_TIME_KEY="median_probe_time"

CLAMP="clamp"
FASTMOD="fast_modulo"

csv = pd.read_csv(f"chained.csv")
partial_data = csv[csv[DATASET_KEY].isnull()]
data = csv[csv[DATASET_KEY].notnull()]

# Generate plot
fig, axs = plt.subplots(2,2,figsize=(7.00697,4))

letter = [["A", "B"], ["C", "D"]]
for p, payload_size in enumerate(set(data[PAYLOAD_SIZE_KEY])):
    for s, slots_per_bucket in enumerate(set(data[SLOTS_PER_BUCKET_KEY])):
        ax = axs[p][s]
        ax.set_title(f"({letter[p][s]}) {payload_size} bytes payload, {slots_per_bucket} slots per bucket", fontsize=8)

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
                # Don't use normal dataset results for now (dataset broken)
                & ((data[DATASET_KEY] != "normal_200M_uint64"))
                # Only use certain hash functions
                & (
                   # (data[HASH_KEY] == "mult_prime64")
                   # | (data[HASH_KEY] == "mult_add64")
                    (data[HASH_KEY] == "murmur_finalizer64")
                   # | (data[HASH_KEY].str.contains("rmi"))
                    | (data[HASH_KEY].str.match("radix_spline"))
                   # | (data[HASH_KEY].str.match("pgm"))
                    )
                ]

        # Use do_nothing entries to determine order
        tmp_d = d[((d[SAMPLE_SIZE_KEY] == 0.01) | (d[SAMPLE_SIZE_KEY].isnull()))
                & (d[DATASET_KEY] ==
                    "wiki_200M_uint64")].sort_values(MEDIAN_PROBE_TIME_KEY)
        # dict preserves insertion order since python 3.7
        classical_hashfns = [
               # "mult_prime64", "mult_add64", 
                "murmur_finalizer64"] 
        tmp_d = d[((d[SAMPLE_SIZE_KEY] == 0.01) | (d[SAMPLE_SIZE_KEY].isnull()))]
        learned_hashfns = list(dict.fromkeys(tmp_d[tmp_d[REDUCER_KEY].str.match(CLAMP)][HASH_KEY]))
        all_hashfns = learned_hashfns + classical_hashfns

        colors = {"RadixSpline": "tab:blue", "Murmur": "tab:orange"}
        datasets = sorted(set(d[DATASET_KEY]))

        # Aggregate data over multiple datasets
        datasets =  [
                # synthetic
                'seq_200M_uint64',
                'gap_1%_200M_uint64', 
                'gap_10%_200M_uint64', 
                #'uniform_dense_200M_uint64',

                # real
                'wiki_200M_uint64',
                #'fb_200M_uint64', 
                #'osm_cellids_200M_uint64'
                ]

        for i, dataset in enumerate(datasets):
            d2 = d[d[DATASET_KEY] == dataset]
            seen_reps = dict()

            bars = {}
            for hashfn in all_hashfns:
                res_l = list(d2[d2[HASH_KEY] == hashfn][MEDIAN_PROBE_TIME_KEY])
                if len(res_l) != 1:
                    continue
                res = res_l[0]

                if name(hashfn) in seen_reps:
                    old_hashfn = seen_reps[name(hashfn)]
                    old_res = bars[name(old_hashfn)]
                    if old_res > res:
                        seen_reps[name(hashfn)] = hashfn
                        bars[name(hashfn)] = res
                else:
                    seen_reps[name(hashfn)] = hashfn
                    bars[name(hashfn)] = res

            # Plt data
            plt_data = [(s.strip(), bars[s]) for s in bars.keys()]
            if len(plt_data) <= 0:
                continue

            empty_space = 0.2
            bar_width = (1 - empty_space) / len(plt_data)
            gap_width = 0 #0.1 / len(plt_data)
            for j, (hash_name, value) in enumerate(plt_data):
                ax.bar(empty_space/2 + i + j * (bar_width+gap_width) +
                        (bar_width+gap_width)/2, value, bar_width,
                        color=colors.get(name(hash_name)) or "purple")

        # Plot style/info
        # if payload_size == 16:
        #     ax.set_ylim(175,250)
        #     ax.set_yticks([175,200,225,250])
        # else:
        #     ax.set_ylim(200,350)
        #     ax.set_yticks([200,250,300,350])
        ax.set_ylim(175,300)
        ax.set_yticks([175,200,225,250,275, 300])

        ax.set_xticks([i+0.5 for i in range(0, len(datasets))])
        ax.set_xticklabels([ds_name(d.replace(r"_200M", "").replace("_uint64",
            "").replace("_", " ")) for d in datasets],
            va="center_baseline",position=(0.5,-0.05), fontsize=8)
        
        # Legend
        ax.legend(
            handles=[mpatches.Patch(color=colors.get(name(h)), label=name(h)) for h,_ in
                hr_names.items()],
            #bbox_to_anchor=(1, 0.98),
            loc="upper right",
            fontsize=6)


fig.text(0.5, 0.02, 'Dataset', ha='center', fontsize=8)
fig.text(0.01, 0.5, 'Nanoseconds per key', va='center', rotation='vertical', fontsize=8)

plt.tight_layout()
plt.subplots_adjust(left=0.085, bottom=0.12)

plt.savefig(f"out/median_probe_time.pdf")
