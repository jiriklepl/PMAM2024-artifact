# Scripts

This directory contains scripts that are used to run the experiment, validate the implementations, generate the plots presented in the paper and produce further data from the collected measurements.

## Shell scripts

- [run-measurements-CPU.sh](run-measurements-CPU.sh): Script for running the measurements on CPU.

  This script runs the measurements of Polybench/C and the tuned/TBB-parallelized versions in 10 repetitions with a warm-up run. The measured wall-clock times are stored in the `medium-data`, `large-data`, and `extralarge-data` directories in the respective benchmark directories (always in the ones ending with `-Noarr`). The measured wall-clock times are stored in `<algorithm>.log` files in the following format:

  ```log
  <implementation>: <wall-clock time>
  ```

  where `<implementation>` is either `Noarr` or `Baseline` and `<wall-clock time>` is the measured wall-clock time in seconds with 6 decimal places. The first line of each implementation is the warm-up run and it should be filtered out before analysis.

- [run-measurements-GPU.sh](run-measurements-GPU.sh): Script for running the measurements on GPU.

  This script runs the measurements of Polybench/GPU in 10 repetitions with a warm-up run. The measured wall-clock times are stored in the `data` directory in the `PolyBenchGPU-Noarr` directory. The log files are stored in the same format as in [run-measurements-CPU.sh](run-measurements-CPU.sh).

- [validate-CPU.sh](validate-CPU.sh): Script for validating the implementations of the algorithms for CPU.

  This script runs the implementations of the Polybench/C and the tuned/TBB-parallelized versions and compares their respective outputs with their Noarr counterparts. It outputs whether it found any mismatches in the outputs.

- [validate-GPU.sh](validate-GPU.sh): Script for validating the implementation of the algorithms for GPU.

  This script runs the implementations of the Polybench/GPU and compares their respective outputs with their Noarr counterparts. It outputs whether it found any mismatches in the outputs.

- [parse_data.sh](parse_data.sh): Script for parsing the measured wall-clock times.

  This script parses the measured wall-clock times from the given `data` directory (containing the log files in the format described above), filters out the warm-up runs, and outputs the results in the following comma-separated format (CSV) to the standard output:

  ```csv
  <name>,<implementation>,<wall-clock time>
  ```

  where `<name>` is the name of the algorithm, `<implementation>` is either `noarr` or `baseline`, and `<wall-clock time>` is the measured wall-clock time in seconds with 6 decimal places.

- [generate_plots.sh](generate_plots.sh): Script for generating the plots from the measured wall-clock times using `parse_data.sh` and R scripts in the root directory.

  This script generates the plots from the measured wall-clock times that are presented in the paper.

- [code_compare.sh](code_compare.sh): Script for comparing the code of the original Polybench/C benchmark and the Noarr implementation.

  This script compares the code of the original Polybench/C benchmark and the Noarr implementation and outputs the differences in the following comma-separated format (CSV) into the file `statistics.csv` in the `PolybenchC-Noarr` directory:

  ```csv
  <implementation>,<algorithm>,<lines>,<characters>,<tokens>,<gzip-size>
  ```
  
  where `<implementation>` is either `noarr` or `baseline`, `<algorithm>` is the name of the algorithm, `<lines>` is the number of lines enclosed within SCOP regions after stripping any comments and applying clang-format, `<characters>` is then the number of characters, `<tokens>` is the number of single C/C++ tokens, and `<gzip-size>` is the preprocessed SCOP region compressed using gzip.

  On the standard output, it outputs the summarized statistics (as shown in `code_overall.log` in the `PolybenchC-Noarr` directory), comparing the number of lines, characters, and tokens of the original Polybench/C benchmark and the Noarr implementation after the same preprocessing. It also outputs the total size of the preprocessed SCOP regions compressed using gzip as single files and as a tar archive.

  it also outputs files `noarr.cpp` and `c.cpp` that contain the concatenated SCOP regions of the respective implementations for inspection.

- [compare_transformations.sh](compare_transformations.sh): Script for comparing the code changes required to perform the tuning transformations implemented in [PolybenchC-tuned](PolybenchC-tuned) on algorithms in [PolybenchC-pretune](PolybenchC-pretune) that are adjusted for the transformations. The changes are compared against Noarr proto-structures in [PolybenchC-Noarr-tuned](PolybenchC-Noarr-tuned) that perform the same transformations on the Noarr abstraction of the algorithms.

  On the standard output, it outputs the column-wise diff for the Polybench/C baseline and the list of proto-structures for the Noarr implementation for each algorithm preceded by the name of the algorithm. Each list is followed by the total number of changes or proto-structures.

- [more_visualizations.sh](more_visualizations.sh): Script for generating additional visualizations of the measured wall-clock times and the code comparison.

  This script generates additional visualizations of the measured wall-clock times and the code comparison that are referenced in the paper.

- [run-examples.sh](run-examples.sh): Script for running the examples of Noarr code presented in the paper.

  This script runs the examples of Noarr code presented in the paper on freshly generated input data and performs a simple validation of the outputs. The purpose of this script is to demonstrate that the presented Noarr code is functional and can be used in practice. The script requires Python 3.6 or newer on top of the requirements for the experiments.

## R scripts

- [plot-polybench.R](plot-polybench.R): Script for generating the plots for the Polybench/C benchmarks.

  This script generates the plots for the Polybench/C benchmarks that are presented in the paper.

- [plot-tbb.R](plot-tbb.R): Script for generating the plots for the TBB-parallelized benchmarks.

  This script generates the plots for the TBB-parallelized benchmarks that are presented in the paper. It is handled separately from [plot-polybench.R](plot-polybench.R) because the TBB-parallelized benchmarks are plotted via a boxplot to better visualize the variance of the measured wall-clock times.

- [plot-others.R](plot-others.R): Script for generating the plots for the other benchmarks (polybench/GPU and polybench/C with tuning).

  This script generates the plots for the other benchmarks (polybench/GPU and polybench/C with tuning) that are presented in the paper. These are presented in the same scale as the TBB-parallelized benchmarks.

- [plot-statistics.R](plot-statistics.R): Script for generating the plots for the code comparison.
