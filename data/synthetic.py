import math
import os
import random
import sys
import time


# -------------------------------------
#   Synthetic dataset generation code
# -------------------------------------

def update_progress(name, message="", progress=0.0, width=40):
    bars = min(width, math.floor(progress * width))
    empty = width - bars
    prompt = f"{name} [{'=' * bars}{' ' * empty}]"
    if message != "":
        prompt += f" - {message}"

    sys.stdout.write(prompt)
    sys.stdout.flush()
    sys.stdout.write('\b' * len(prompt))


def write_dataset(name, numbers, amount, bytes_per_number=8):
    """
    Writes a list of numbers in our simple binary dataset format

    :param name: name of the dataset, will be used to derive filename as <name>_<bits_per_number>
    :param numbers: generator for the dataset
    :param amount: how many numbers to write from generator
    :param bytes_per_number: how many bytes per number we should use for encoding. Defaults to 8
    :return:
    """
    # write payload to file
    path = os.path.realpath(os.path.dirname(__file__))
    filename = f"{name}_uint{bytes_per_number * 8}"
    buffering = 2 ** 10

    with open(f"{path}/{filename}", 'wb', buffering=buffering) as file:
        update_progress(name=filename, progress=0.0)

        # header contains the amount of elements in the list (8 bytes in little endian)
        file.write(amount.to_bytes(8, byteorder='little'))

        # used to calculate eta
        start = time.time()

        # properly encoded bytestream from input numbers
        def byte_stream():
            n = 0
            for e in numbers:
                yield e.to_bytes(bytes_per_number, byteorder='little')

                # ensure we never generate more than the requested amount of numbers
                n += 1
                if n >= amount:
                    break

                # update progress indicator every 100000 elements
                if n % 100000 == 0:
                    passed = time.time() - start
                    total_est = (passed / n) * amount
                    update_progress(filename, message=f"{passed:.2f}s / {total_est:.2f}s", progress=n / amount)

        # payload contains all numbers consecutively stored in little endian,
        # each taking up exactly bytes_per_number bytes. Write out in 256 MB chunks
        file.writelines(byte_stream())

    update_progress(filename, progress=1.0)
    print()


# dense keys, e.g., autogenerated, consecutive primary keys without deletion
def dense(start=1):
    """
    Dense numbers, i.e., consecutive natural numbers. This distribution
    simulates primary keys without deletions.

    :param start: first number to occur in the dataset. Defaults to 1.
    :return:
    """
    num = start
    while True:
        yield num
        num += 1


# monotone keys, i.e., autogenerated primary keys with random missing elements/gaps, i.e., to simulate deletes
def gapped(start=1, delete_probability=0.05):
    for i in dense(start=start):
        if random.random() < 1 - delete_probability:
            yield i


# monotone keys with duplicates
def duplicated(start=1, duplicate_chance=0.4):
    num = start
    while True:
        yield num
        if random.random() < 1 - duplicate_chance:
            num += 1

def uniform(start=0, end=pow(2, 64)):
    while True:
        yield int(random.uniform(start, end))

# write datasets
write_dataset(name="debug", numbers=duplicated(start=1), amount=1000000, bytes_per_number=8)
write_dataset(name="uniform_200M", numbers=uniform(), amount=2 * (10**8), bytes_per_number=8)
write_dataset(name="consecutive_200M", numbers=dense(start=1000), amount=2 * (10**8), bytes_per_number=8)
write_dataset(name="consecutive_100M", numbers=dense(start=1000), amount=1 * (10 ** 8), bytes_per_number=8)
write_dataset(name="gapped_1permill_200M", numbers=gapped(start=10000,delete_probability=0.001), amount=2 * (10**8), bytes_per_number=8)
write_dataset(name="gapped_1percent_200M", numbers=gapped(start=10000,delete_probability=0.01), amount=2 * (10**8), bytes_per_number=8)
write_dataset(name="gapped_10percent_200M", numbers=gapped(start=10000,delete_probability=0.1), amount=2 * (10**8), bytes_per_number=8)
