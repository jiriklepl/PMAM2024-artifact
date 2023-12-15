#!/usr/bin/env bash

# This script compares the output of the C and C++/Noarr implementations of the Polybench benchmarks
# It assumes that the C++/Noarr implementations are built in the build directory and that the C implementations are built in the $POLYBENCH_C_DIR/build directory

BUILD_DIR=${BUILD_DIR:-build}
IGNORE_AUTOTUNED=${IGNORE_AUTOTUNED:-0}
SKIP_DIFF=${SKIP_DIFF:-0}

if [ -z "$POLYBENCH_C_DIR" ]; then
	POLYBENCH_C_DIR="$BUILD_DIR/PolyBenchC-4.2.1"
	mkdir -p "$POLYBENCH_C_DIR" || exit 1
	if [ -d "$POLYBENCH_C_DIR/.git" ]; then
		( cd "$POLYBENCH_C_DIR" && git pull )
	else
		git clone "https://github.com/jiriklepl/PolyBenchC-4.2.1.git" "$POLYBENCH_C_DIR"
	fi
fi

dirname=$(mktemp -d)

trap "echo deleting $dirname; rm -rf $dirname" EXIT

( cd "$POLYBENCH_C_DIR" && ./build.sh ) || exit 1
( cd . && ./build.sh ) || exit 1

find "$BUILD_DIR" -maxdepth 1 -executable -type f |
while read -r file; do
	filename=$(basename "$file")

	echo "Comparing $filename"

	printf "\tNoarr:             "
	"$BUILD_DIR/$filename" 2>&1 1> "$dirname/cpp"

	printf "\tC:                 "
	"$POLYBENCH_C_DIR/$BUILD_DIR/$filename" 2> "$dirname/c"

	paste <(grep -oE '[0-9]+\.[0-9]+' "$dirname/c") <(grep -oE '[0-9]+(\.[0-9]+)?' "$dirname/cpp") |
	awk "BEGIN {
		different = 0
		n = 0
		changes = 0
	}

	{
		n++
		if (\$1 != \$2 && changes < 10) {
			print \"baseline\", n, \$1
			print \"   noarr\", n, \$2
			changes++
			different = 1
		}

		if (changes >= 10)
			nextfile

		next
	}

	{ different = 1; nextfile }

	END {
		if (different) {
			printf \"Different output on %s \n\", \"$filename\"
			exit 1
		}
	}" 1>&2
done
