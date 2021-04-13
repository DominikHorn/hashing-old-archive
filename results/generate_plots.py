from collections import OrderedDict

import matplotlib as mpl
import matplotlib.pyplot as plt
import numpy as np
import pandas as pandas

colors = list(mpl.colors.TABLEAU_COLORS)

csv = pandas.read_csv('throughput.csv')
dataset_names = sorted(set(csv['dataset']))

for fig, dataset_name in enumerate(dataset_names):
    dataset = csv[csv['dataset'] == dataset_name]
    dataset = dataset[dataset['load_factor'] == 1.0]

    reducers = sorted(list(set(dataset['reducer'])))

    plt.figure(figsize=(30, 10))

    # loosely taken from https://benalexkeen.com/bar-charts-in-matplotlib/
    plt.title(f"throughput on {dataset_name}")
    plt.xticks(np.arange(len(reducers)) + 0.5, reducers)
    plt.ylabel('ns per key')
    plt.xlabel('hash reduction method')

    # order preserving deduplication
    hash_methods = list(OrderedDict.fromkeys(list(dataset.sort_values('nanoseconds_per_key')['hash'])))

    for j, hash_name in enumerate(hash_methods):
        series = list(dataset[dataset['hash'] == hash_name].sort_values('reducer')['nanoseconds_per_key'])
        width = 0.95 / len(hash_methods)

        for i, reducer in enumerate(reducers):
            if i == 0:
                plt.bar(i + j * width, series[i], width, label=hash_name, color=colors[j % len(colors)])
            else:
                plt.bar(i + j * width, series[i], width, color=colors[j % len(colors)])

    plt.legend(bbox_to_anchor=(0.5, -0.1), ncol=7)
    plt.savefig(f"throughput_{dataset_name}.pdf", bbox_inches='tight', pad_inches=0.5)
