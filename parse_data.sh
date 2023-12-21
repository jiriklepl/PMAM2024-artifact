#!/bin/bash

echo "name,implementation,time"

find data -name "*.log" |
while read -r file; do
    (awk -vfile="$(basename "$file" | sed 's/\..*$//')" '
/Baseline:/ {
    if (Baseline++) data_baseline[Baseline] = $2
}

/Noarr:/ {
    if (Noarr++) data_noarr[Noarr] = $2
}

END {
    for (i = 2; i <= (Baseline > Noarr ? Baseline : Noarr); i++) {
        if (data_noarr[i] > 0)
            print(file ",noarr," data_noarr[i])

        if (data_baseline[i] > 0)
            print(file ",baseline," data_baseline[i])
    }
}' "$file" & wait )

done
