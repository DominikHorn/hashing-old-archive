from collections import OrderedDict

import matplotlib.colors
import matplotlib.pyplot as plt
import numpy as np
import pandas as pandas

csv = pandas.read_csv('throughput.csv')
dataset_names = sorted(set(csv['dataset']))

# ensure colors are at least consistent across plots
tableau_colors = list(matplotlib.colors.TABLEAU_COLORS)
hash_methods = list(
    OrderedDict.fromkeys(list(
        csv[csv['load_factor'] == 1.0][csv['reducer'] == 'do_nothing'].sort_values('nanoseconds_per_key')['hash'])))
colors = {method: tableau_colors[i % len(tableau_colors)] for i, method in enumerate(hash_methods)}

for fig, dataset_name in enumerate(dataset_names):
    dataset = csv[csv['dataset'] == dataset_name]
    dataset = dataset[dataset['load_factor'] == 1.0]

    reducers = sorted(list(set(dataset['reducer'])))

    plt.figure(figsize=(30, 10))

    # order preserving deduplication
    for j, hash_name in enumerate([m for m in hash_methods if m in set(dataset['hash'])]):
        series = list(dataset[dataset['hash'] == hash_name].sort_values('reducer')['nanoseconds_per_key'])
        width = 0.95 / len(hash_methods)

        for i, reducer in enumerate(reducers):
            # We only want to set label once as otherwise legend will contain duplicates
            plt.bar(i + j * width, series[i], width, label=hash_name if i == 0 else None,
                    color=colors.get(hash_name) or "white")

    plt.grid(linestyle='--', linewidth=0.5)
    plt.title(f"throughput on {dataset_name}")
    plt.xticks(np.arange(len(reducers)) + 0.4, reducers)
    plt.yticks(list(plt.yticks()[0]) + [1, 2, 3, 4])
    plt.ylabel('ns per key')
    plt.xlabel('hash reduction method')
    plt.legend(bbox_to_anchor=(0.5, -0.1), ncol=7)

    plt.savefig(f"throughput_{dataset_name}.pdf", bbox_inches='tight', pad_inches=0.5)
    plt.savefig(f"throughput_{dataset_name}.png", bbox_inches='tight', pad_inches=0.5)
