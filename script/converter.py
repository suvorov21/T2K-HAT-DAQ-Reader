#!/usr/bin/env python3

import argparse
import subprocess
import shutil
import os
import random
from sys import exit
from itertools import chain

def main():
    parser = argparse.ArgumentParser(description='Event convertion AQS --> ROOT')
    parser.add_argument("-i", metavar="i", type=str,
        help='Input directory',
        required=True)

    parser.add_argument("-o", metavar="o", type=str,
        help='output directory',
        required=True)

    parser.add_argument("-e", metavar="e", type=str,
        help='Converter executable',
        required=True)

    args = parser.parse_args()

    for root, dirt, find_file in chain.from_iterable([os.walk(args.i)]):
        for file in find_file:
            if ".aqs" in file:
                print(f"Working on file {os.path.join(root, file)}")
                subprocess.run([args.e, "-i", os.path.join(root, file), "-o", args.o], check=False)


if __name__ == "__main__":
    main()