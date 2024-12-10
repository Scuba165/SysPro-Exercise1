#!/bin/bash
if [ $# -ne 1 ]; then
    echo "Wrong input. I expected a file."
    exit 1
fi

inputFile="$1"
if [ ! -f "$inputFile" ]; then 
    echo "File not found. $inputFile"
    exit 1
fi

while IFS= read -r line; do
    ./jobCommander issueJob "$line"
done < "$inputFile"
exit 0