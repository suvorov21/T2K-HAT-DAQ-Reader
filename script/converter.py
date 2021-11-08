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
    parser.add_argument("-i", metavar="input_dir", type=str,
        help='Input directory',
        required=True)

    parser.add_argument("-o", metavar="output_dir", type=str,
        help='output directory',
        required=True)

    parser.add_argument("-e", metavar="executable", type=str,
        help='Converter executable',
        required=True)

    args = parser.parse_args()

    for root, dirt, find_file in chain.from_iterable([os.walk(args.input_dir)]):
        for file in find_file:
            if ".aqs" == file[-4:]:
                print(f"Working on file {os.path.join(root, file)}")
                subprocess.run([args.executable, "-i", os.path.join(root, file), "-o", args.output_dir], check=False)


if __name__ == "__main__":
    main()