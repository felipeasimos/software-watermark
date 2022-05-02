#!/bin/bash

OPT=2
WATERMARK=119
DIR=""
OUTPUT_FILE=`mktemp`

function rm_temp_output_file() {
    rm $OUTPUT_FILE
    exit 0
}

trap "rm_temp_output_file" 2

if [[ $1 = "" ]]
then
    DIR="."
else
    DIR=$1
fi

make compilemain

for filename in $( ls $1 | grep -o ".*\.c" );
do
    LD_LIBRARY_PATH=./build/app ./watermark <<< "$OPT
    $WATERMARK
    y
    $1/$filename" > $OUTPUT_FILE

    if [[ $? -eq 0 ]]
    then
        BEST_FIT_FUNCTION=`cat $OUTPUT_FILE | grep "^best fit function is: '.*'$" | cut -d "'" -f 2`
        SCORE=`cat $OUTPUT_FILE | grep -A 3 "^best fit function is: '.*'$" | grep score | grep -o "[[:digit:]]*"`
        ENTRY_POINT=`cat $OUTPUT_FILE | grep -A 3 "^best fit function is: '.*'$" | grep entry_point | grep -o "[[:digit:]]*"`
        DIJKSTRA_CODE=`cat $OUTPUT_FILE | grep -A 3 "^best fit function is: '.*'$" | grep "dijkstra code" | grep -o "[[:digit:]]*"`

        echo -e "$filename:$BEST_FIT_FUNCTION:\n\tscore: $SCORE\n\tentry point: $ENTRY_POINT\n\tdijkstra code: $DIJKSTRA_CODE"
    fi
done

rm_temp_output_file
