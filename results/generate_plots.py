import matplotlib as mpl
import matplotlib.pyplot as plt
import numpy as np
import pandas as pandas

csv = pandas.read_csv('throughput.csv')
colors = list(mpl.colors.CSS4_COLORS)

for dataset_name in sorted(set(csv['dataset'])):
    dataset = csv[csv['dataset'] == dataset_name]
    for load_factor in sorted(set(dataset['load_factor'])):
        dataset = dataset[dataset['load_factor'] == load_factor]
        reducers = sorted(list(set(dataset['reducer'])))

        # loosely taken from https://benalexkeen.com/bar-charts-in-matplotlib/
        # plt.clf()
        hashmethods = sorted(set(dataset['hash']))
        for j, hashname in enumerate(hashmethods):
            series = list(dataset[dataset['hash'] == hashname].sort_values('reducer')["nanoseconds_per_key"])
            width = 0.95 / len(hashmethods)

            for i, reducer in enumerate(reducers):
                if i == 0:
                    plt.bar(i + j * width, series, width, label=hashname, color=colors[j % len(colors)])
                else:
                    plt.bar(i + j * width, series, width, color=colors[j % len(colors)])

        plt.ylabel('ns per key')
        plt.xlabel('hash reduction method')
        plt.title(f"{dataset_name} with load factor {load_factor}")

        plt.xticks(np.arange(len(reducers)) + 0.5, reducers)
        plt.legend(loc="best", ncol=7)
        plt.gcf().set_size_inches(20, 5)
        plt.savefig(f"throughput_{dataset_name}_{load_factor}.pdf")

        exit()
