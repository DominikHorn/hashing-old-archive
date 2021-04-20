import colorsys
from collections import OrderedDict

import math
import matplotlib.pyplot as plt
import numpy as np
import pandas as pandas

def gen_colors(max=40.0, s_sawtooth_min=0.7, s_sawtooth_max=0.9, s_sawtooth_step=0.1, v_sawtooth_min=0.5,
               v_sawtooth_max=1.0, v_sawtooth_step=0.15):
    s_next = s_sawtooth_min
    v_next = v_sawtooth_min
    for i in range(max):
        h = (2 * i / max) if 2 * i <= max else (2 * i - max) / max
        s = s_next
        v = v_next
        yield colorsys.hsv_to_rgb(h, s, v)

        s_next += s_sawtooth_step
        if s_next > s_sawtooth_max:
            s_next = s_sawtooth_min

        v_next += v_sawtooth_step
        if v_next > v_sawtooth_max:
            v_next = v_sawtooth_min


compilers = ["clang++", "g++-10"]
_csv = pandas.read_csv(f"collisions_hash-{compilers[0]}.csv")
dataset_names = sorted(set(_csv['dataset']))
hash_methods = list(OrderedDict.fromkeys(list(_csv[_csv['reducer'] != 'do_nothing'].sort_values('nanoseconds_per_key')['hash'])))
_colors = list(gen_colors(len(hash_methods)))
colors = {method: _colors[i] for i, method in enumerate(hash_methods)}

for compiler in compilers:
    csv = pandas.read_csv(f'collisions_hash-{compiler}.csv')

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
                    plt.bar(i + j * width + j * 0.005, series[i], width, label=hash_name if i == 0 else None,
                            color=colors.get(hash_name) or "white")

            plt.grid(linestyle='--', linewidth=0.5)
            plt.title(f"collisions on {dataset_name}, load_factor {load_factor}, compiler {compiler}")
            plt.xticks(np.arange(len(reducers)) + (1.0 / len(reducers)), reducers)
            pred_collision_chance = 1 - pow(math.e, -load_factor)
            plt.yticks([x for x in [0.0, 0.2, 0.4, 0.6, 0.8, 1.0] if abs(x - pred_collision_chance) > 0.1] + [
                pred_collision_chance])
            plt.ylabel('colliding keys / total keys')
            plt.xlabel('hash reduction method')

            plt.legend(bbox_to_anchor=(0.5, -0.1), loc="upper center", ncol=7)

        plt.savefig(f"graphs/colliding_keys_percent_{dataset_name}_{compiler}.png", bbox_inches='tight', pad_inches=0.5)
        plt.savefig(f"graphs/colliding_keys_percent_{dataset_name}_{compiler}.pdf", bbox_inches='tight', pad_inches=0.5)
