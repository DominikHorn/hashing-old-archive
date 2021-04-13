from collections import OrderedDict

import math
import matplotlib.colors
import matplotlib.pyplot as plt
import numpy as np
import pandas as pandas

csv = pandas.read_csv('collision.csv')
dataset_names = sorted(set(csv['dataset']))

# ensure colors are at least consistent across plots
tableau_colors = list(matplotlib.colors.TABLEAU_COLORS)
hash_methods = list(
    OrderedDict.fromkeys(list(csv[csv['reducer'] != 'do_nothing'].sort_values('total_colliding_keys_percent')['hash'])))
colors = {method: tableau_colors[i % len(tableau_colors)] for i, method in enumerate(hash_methods)}

for fig, dataset_name in enumerate(dataset_names):
    dataset = csv[csv['dataset'] == dataset_name]

    plt.figure(figsize=(40, 12))
    plt.subplots_adjust(hspace=0.5)

    for k, load_factor in enumerate(reversed(sorted(set(dataset['load_factor'])))):
        ds = dataset[dataset['load_factor'] == load_factor]
        ds = ds[ds['reducer'] != 'do_nothing']
        reducers = sorted(list(set(ds['reducer'])))

        plt.subplot(2, 2, k + 1)

        # order preserving deduplication
        for j, hash_name in enumerate([m for m in hash_methods if m in set(ds['hash'])]):
            series = list(ds[ds['hash'] == hash_name].sort_values('reducer')['total_colliding_keys_percent'])
            width = 0.95 / len(hash_methods)

            for i, reducer in enumerate(reducers):
                # We only want to set label once as otherwise legend will contain duplicates
                plt.bar(i + j * width, series[i], width, label=hash_name if i == 0 else None,
                        color=colors.get(hash_name) or "white")

        plt.grid(linestyle='--', linewidth=0.5)
        plt.title(f"collisions on {dataset_name}, load_factor {load_factor}")
        plt.xticks(np.arange(len(reducers)) + 0.4, reducers)

        pred_collision_chance = 1 - pow(math.e, -load_factor)
        plt.yticks([x for x in [0.0, 0.2, 0.4, 0.6, 0.8, 1.0] if abs(x - pred_collision_chance) > 0.1] + [
            pred_collision_chance])
        plt.ylabel('colliding keys / total keys')
        plt.xlabel('hash reduction method')

        plt.legend(bbox_to_anchor=(0.5, -0.1), loc="upper center", ncol=7)

    plt.savefig(f"total_colliding_keys_percent_{dataset_name}.pdf", bbox_inches='tight', pad_inches=0.5)
