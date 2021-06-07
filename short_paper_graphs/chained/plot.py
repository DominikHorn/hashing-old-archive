import numpy as np
import matplotlib as mpl
import matplotlib.patches as mpatches
import matplotlib.pyplot as plt
import matplotlib.colors as mcolors
import pandas as pd
import math as math

# Style
hr_names = {"radix_spline": "RadixSpline",
        #"mult_prime64": "mult", "mult_add64": "mult_add", 
        "murmur_finalizer64": "Murmur"}
colors = {"RadixSpline": "tab:blue", "Murmur": "tab:orange"}
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
fig, axs = plt.subplots(2,2,figsize=(7.00697/2,2), sharex=True, sharey=True)

letter = [["A", "B"], ["C", "D"]]
for p, payload_size in enumerate(set(data[PAYLOAD_SIZE_KEY])):
    for s, slots_per_bucket in enumerate(set(data[SLOTS_PER_BUCKET_KEY])):
        ax = axs[p][s]
        ax.set_title(f"({letter[p][s]}) {payload_size}B payload {slots_per_bucket} slots per bucket", fontsize=6)

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
                   # (data[HASH_KEY] == "mult_prime64")
                   # | (data[HASH_KEY] == "mult_add64")
                    (data[HASH_KEY] == "murmur_finalizer64")
                   # | (data[HASH_KEY].str.contains("rmi"))
                    | (data[HASH_KEY].str.match("radix_spline"))
                   # | (data[HASH_KEY].str.match("pgm"))
                    )
                ]

        # dict preserves insertion order since python 3.7
        all_hashfns = list(set(d[d[REDUCER_KEY].str.match(CLAMP)][HASH_KEY])) + ["murmur_finalizer64"]

        datapoints = list(zip(d[DATASET_KEY], d[ADDITIONAL_BUCKETS_KEY], d[MEDIAN_PROBE_TIME_KEY], d[HASH_KEY]))
        for (dataset, additional_buckets, median_probe_time, hashfn) in datapoints:
            hash_name = name(hashfn)
            dataset_name = name_d(dataset)

            ax.scatter(additional_buckets, median_probe_time, c=colors.get(hash_name), marker='.', s=10)

            def x_adjust():
                return 0
            def y_adjust():
                return +3
            def ha():
                return 'center'
            def va():
                return 'baseline'


            #if hash_name != "Murmur":
            #    ax.annotate(
            #            f"{dataset_name}", 
            #            (additional_buckets + x_adjust(), median_probe_time + y_adjust()), 
            #            fontsize=5, 
            #            ha=ha(),
            #            va=va())

        # Plot style/info
        #ax.set_yticks([225, 250, 275])
        ax.set_xlim(-0.1,1.1)
        ax.set_xticks([0.0, 0.33, 0.66, 1.0])
        #ax.set_ylim(100,1500)
        ax.tick_params(axis='both', which='major', labelsize=8)
        #ax.margins(x=0.1)

        # Legend 
        if p == 0 and s == 1:
            ax.legend(handles=[mpatches.Patch(color=colors.get(name(h)), label=name(h)) for h,_ in hr_names.items()],
                loc="lower right",
                fontsize=5)


fig.text(0.5, 0.02, 'Additional buckets per key [percent]', ha='center', fontsize=8)
fig.text(0.01, 0.5, 'Probe time per key [ns]', va='center', rotation='vertical', fontsize=8)

plt.tight_layout()
plt.subplots_adjust(left=0.15, bottom=0.175, wspace=0.15, hspace=0.5)

plt.savefig(f"out/median_probe_time.pdf")
