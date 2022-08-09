#!/usr/bin/env python3

import argparse
import csv
import itertools
import math
import matplotlib
import os
import subprocess
import sys
import pandas

class WorkingDirectory:
    "Scoped context for changing working directory"

    def __init__(self, working_dir):
        self.original_dir = os.getcwd()
        self.working_dir = working_dir

    def __enter__(self):
        sys.stderr.write("Entering directory `%s'\n" % self.working_dir)
        os.chdir(self.working_dir)
        return self

    def __exit__(self, exc_type, value, traceback):
        sys.stderr.write("Leaving directory `%s'\n" % self.working_dir)
        os.chdir(self.original_dir)


def get_dashes():
    "Generator for plot line dash patterns"
    dash = 2.0
    space = dot = 0.75

    yield []  # Solid
    yield [dash, space]  # Dashed
    yield [dot, space]  # Dotted

    # Dash-dots, with increasing number of dots for each line
    for i in itertools.count(2):
        yield [dash, space] + [dot, space] * (i - 1)


def plot(in_filename, out_filename, x_label, y_label, y_max=None):
    "Plot a TSV file as SVG"

    matplotlib.use("agg")
    import matplotlib.pyplot as plt

    plt.rcParams.update({"font.size": 7})

    fig_height = 1.8
    dashes = get_dashes()
    markers = itertools.cycle(["o", "s", "v", "D", "*", "p", "P", "h", "X"])

    df = pandas.read_csv(in_filename, sep="\t")
    errors = [[df['Mean'] - df['Min'], df['Max'] - df['Mean']]]

    ax = df.plot(x="Plugin", y="Mean", kind="bar", yerr=errors, figsize=(16, 8), fontsize=8, width=0.8)
    # FIXME: p

    sample_rate = 48000
    buffer_duration = df['Block'][0] / sample_rate
    if any(df['Max'] >= buffer_duration):
        ax.axhline(y=buffer_duration, dashes=[2.0, 2.0], color='darkred')

    # reader = csv.reader(in_file, delimiter="\t")
    # header = next(reader)

    # dataframe.head()
    # cols = [x for x in zip(*list(reader))]
    # print(header)
    # print(cols)

    # plt.clf()
    # fig = plt.figure(figsize=(fig_height * math.sqrt(2), fig_height))
    # ax = fig.add_subplot(111)

    # ax.set_xlabel(x_label)
    # ax.set_ylabel(y_label)

    # if y_max is not None:
    #     ax.set_ylim([0.0, y_max])

    # ax.grid(linewidth=0.25, linestyle=":", color="0", dashes=[0.2, 1.6])
    # ax.ticklabel_format(style="sci", scilimits=(4, 0), useMathText=True)
    # ax.tick_params(axis="both", width=0.75)

    # x = list(map(float, cols[0]))
    # for i, y in enumerate(cols[1::]):
    #     ax.plot(
    #         x,
    #         list(map(float, y)),
    #         label=header[i + 1],
    #         marker=next(markers),
    #         dashes=next(dashes),
    #         markersize=3.0,
    #         linewidth=1.0,
    #     )

    plt.legend(labelspacing=0.25)
    plt.savefig(out_filename, bbox_inches="tight", pad_inches=0.125)
    plt.close()
    sys.stderr.write("Wrote {}\n".format(out_filename))


def plot_results(tsv_filename, output_filename):
    "Plot all benchmark results"

    plot(tsv_filename, output_filename, "X", "Y")


if __name__ == "__main__":
    ap = argparse.ArgumentParser(
        usage="%(prog)s [OPTION]... TSV_FILE SVG_FILE",
        description="Plot the results of an lv2bench run\n",
        formatter_class=argparse.RawDescriptionHelpFormatter,
    )

    ap.add_argument("tsv_file", help="path to TSV file written by lv2bench")
    ap.add_argument("svg_file", help="path for output SVG file")

    args = ap.parse_args(sys.argv[1:])
    plot_results(args.tsv_file, args.svg_file)
