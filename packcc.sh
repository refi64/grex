#!/bin/sh

packcc="$1"
source="$2"
out_c="$3"
out_h="$4"

"$packcc" -o $(echo "$out_c" | sed 's/\.c$//') "$source"

# Fix the header include path packcc generates.
sed -i 's,"src/,",' "$out_c"
