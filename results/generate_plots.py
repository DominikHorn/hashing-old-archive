import matplotlib as mpl
import matplotlib.pyplot as plt
import numpy as np
import pandas as pandas

csv = pandas.read_csv('throughput.csv')
colors = list(mpl.colors.CSS4_COLORS)

for fig, dataset_name in enumerate(sorted(set(csv['dataset']))):
    dataset = csv[csv['dataset'] == dataset_name]
    reducers = sorted(list(set(dataset['reducer'])))

    plt.figure(figsize=(30, 10))
    plt.subplots_adjust(hspace=0.5)

    for subplt, load_factor in enumerate(sorted(set(dataset['load_factor']))):
        load_data = dataset[dataset['load_factor'] == load_factor]

        # loosely taken from https://benalexkeen.com/bar-charts-in-matplotlib/
        plt.subplot(2, 2, subplt + 1)
        plt.title(f"{dataset_name} with load factor {load_factor}")
        plt.xticks(np.arange(len(reducers)) + 0.5, reducers)
        # plt.ylabel('ns per key')
        # plt.xlabel('hash reduction method')

        hashmethods = sorted(set(load_data['hash']))
        for j, hashname in enumerate(hashmethods):
            series = list(load_data[load_data['hash'] == hashname].sort_values('reducer')["nanoseconds_per_key"])
            width = 0.95 / len(hashmethods)

            for i, reducer in enumerate(reducers):
                if i == 0:
                    plt.bar(i + j * width, series, width, label=hashname, color=colors[j % len(colors)])
                else:
                    plt.bar(i + j * width, series, width, color=colors[j % len(colors)])

    plt.legend(loc="best", ncol=7)
    plt.savefig(f"throughput_{dataset_name}.pdf")

    exit()
