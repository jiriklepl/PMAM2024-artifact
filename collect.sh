#!/usr/bin/bash

export BUILD_DIR=${BUILD_DIR:-build}
export DATASET_SIZE=${DATASET_SIZE:-EXTRALARGE}
export DATA_TYPE=${DATA_TYPE:-FLOAT}
export NOARR_STRUCTURES_BRANCH=${NOARR_STRUCTURES_BRANCH:-tuning}
export RUN_TBB=${RUN_TBB:-0}

if [ -z "$POLYBENCH_C_DIR" ]; then
	POLYBENCH_C_DIR="$BUILD_DIR/PolyBenchC-4.2.1"
	mkdir -p "$POLYBENCH_C_DIR" || exit 1
	if [ -d "$POLYBENCH_C_DIR/.git" ]; then
		( cd "$POLYBENCH_C_DIR" && git fetch && git checkout tbb && git pull )
	else
		git clone --branch=tbb "https://github.com/jiriklepl/PolyBenchC-4.2.1.git" "$POLYBENCH_C_DIR"
	fi
fi

( cd "$POLYBENCH_C_DIR" && srun -A kdss -p mpi-homo-short -ww201 ./build.sh ) || exit 1
( cd . && srun -A kdss -p mpi-homo-short -ww201 ./build.sh ) || exit 1

DATA_DIR="data"

mkdir -p "$DATA_DIR"

find "$BUILD_DIR" -maxdepth 1 -executable -type f |
while read -r file; do
    filename=$(basename "$file")

    case "$filename" in
        *_autotune)
            continue
            ;;
		*-tbb)
			if [ "$RUN_TBB" -eq 0 ]; then
				continue
			fi
			;;
		*)
			if [ "$RUN_TBB" -eq 1 ]; then
				continue
			fi
    esac

    echo "collecting $filename"
    ( srun -A kdss -p mpi-homo-short --exclusive -ww201 ./run_noarr_algorithm.sh "Noarr" "$BUILD_DIR/$filename" & wait ) > "$DATA_DIR/$filename.log"
    ( srun -A kdss -p mpi-homo-short --exclusive -ww201 ./run_c_algorithm.sh "Baseline" "$POLYBENCH_C_DIR/$BUILD_DIR/$filename" & wait ) >> "$DATA_DIR/$filename.log"
    echo "done"
done
