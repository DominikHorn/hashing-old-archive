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
        "aqua": "AquaHash",
        "xxh3": "XXH3",
        "radix_spline": "RadixSpline",
        #"mult_prime64": "mult", "mult_add64": "mult_add",
        "rmi": "RMI",
        #"pgm": "PGM"
        }
all_palette = list(mcolors.TABLEAU_COLORS.keys())
palette = all_palette[:-1]
colors = {
        "aqua": palette[3],
        "murmur_finalizer64": palette[1],
        "xxh3": palette[5],
        "radix_spline": palette[0],
        "rmi": "tab:cyan",
        #"pgm": palette[4]
        }

def name(hashfn):
    return hr_names.get(hashfn) or hashfn

def color(h):
    return colors.get(h) or all_palette[-1]

def plot_collision_statistic(stat_key, title, expected_fun, ymax=1):
    DATASET_KEY="dataset"
    MACHINE_KEY="machine"
    COMPILER_KEY="compiler"
    REDUCER_KEY="reducer"
    HASH_KEY="hash"
    LOAD_FACTOR_KEY="load_factor"
    SAMPLE_SIZE_KEY="sample_size"

    CLAMP="clamp"
    FASTMOD="fast_modulo"

    csv = pd.read_csv(f"collisions.csv")
    partial_data = csv[csv[DATASET_KEY].isnull()]
    data = csv[csv[DATASET_KEY].notnull()]

    # Filter data
    data = data[
            # Only use g++ results
            ((data[COMPILER_KEY].isnull()) | (data[COMPILER_KEY].str.match(r"g\+\+")))
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
                | (data[HASH_KEY].str.contains("rmi"))
                | (data[HASH_KEY].str.match("radix_spline"))
               # | (data[HASH_KEY].str.match("pgm"))
                )
            ]

    # dict preserves insertion order since python 3.7
    classical_hashfns = [
            #"mult_prime64", "mult_add64", 
            "murmur_finalizer64"] 
    learned_hashfns = list(dict.fromkeys(data[data[REDUCER_KEY] == CLAMP][HASH_KEY])) 
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
        d = data[data[DATASET_KEY] == dataset]
        hashfns = all_hashfns

        bars = {}
        for hashfn in hashfns:
            bars[hashfn] = list(d[d[HASH_KEY] == hashfn][stat_key])

        # Plt data
        plt_data = [(s.strip(), bars[s]) for s in hashfns]
        empty_space = 0.2
        bar_width = (1 - empty_space) / len(plt_data)
        gap_width = 0 #0.1 / len(plt_data)
        for j, (hash_name, value) in enumerate(plt_data):
            ax.bar(empty_space/2 + i + j * (bar_width+gap_width) +
                    (bar_width+gap_width)/2, value, bar_width, color=colors.get(hash_name) or "purple")

    # Expected value (load factor 1)
    y = expected_fun(1.0)
    ax.plot([0, len(datasets)], [y,y], color="black",
            linestyle="dashed", linewidth=1.0)

    # Plot style/info
    yticks = np.linspace(0, ymax, 4)
    plt.ylim(0,ymax)
    plt.yticks(yticks, [f"{int(yt*100)}%" for yt in yticks], fontsize=15)
    plt.ylabel(f"{stat_key.replace('_', ' ').capitalize()}", fontsize=15)

    plt.xticks([i+0.5 for i in range(0, len(datasets))], [d.replace(r"_200M",
        "").replace("_uint64", "").replace("_", " ") for d in datasets],
        va="center_baseline",position=(0.5,-0.05), fontsize=15, rotation=35, ha="right")
    plt.xlabel("Dataset", fontsize=15)

    plt.margins(x=0.01,y=0.2)
    plt.tight_layout()
    plt.subplots_adjust(bottom=0.4, top=0.95)

    # Legend
    ax.legend(
        handles=[mpatches.Patch(color=colors.get(h), label=name(h)) for h in all_hashfns],
        loc="upper left",
        fontsize=11,
        borderpad=0.2,
        labelspacing=0.1,
        ncol=1)

    plt.savefig(f"out/{stat_key}.pdf")
    plt.savefig(f"out/{stat_key}.pgf")

#plot_expected_colliding_keys()
#plot_collision_statistic("colliding_keys", "Colliding keys", lambda load_fac : 1
#        - np.exp(-load_fac))
#plot_collision_statistic("colliding_slots", "Colliding slots", lambda load_fac :
#        1 - (((1 + 1/load_fac) * np.exp(-load_fac)) / (1 / load_fac)), ymax=0.5)
plot_collision_statistic("empty_slots", "Empty slots", lambda load_fac:
        np.exp(-load_fac), ymax=1.0)
#plot_collision_statistic("exclusive_slots", "Exclusive slots (exactly one key)", lambda load_fac :
#        1 - (1 - (((1 + 1/load_fac) * np.exp(-load_fac)) / (1 / load_fac)) +
#            np.exp(-load_fac)))

