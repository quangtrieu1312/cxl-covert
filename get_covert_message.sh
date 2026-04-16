#!/usr/bin/env bash
printUsage() {
	echo "Usage: $0 <threshold> <input file>"
}
threshold=$1
if [[ -z "$threshold" ]]; then
	printUsage
	exit 1
fi
input=$2
if [[ -z "$input" ]]; then
	printUsage
	exit 1
fi
cat "$input" | grep -aoE '^[0-9]+,[0-9]+$' | while read -r data; do 
	total_cycles=$(echo $data | awk -F, '{print $1}')
	total_access=$(echo $data | awk -F, '{print $2}')
	cycles=$(bc -l <<< "$total_cycles/$total_access")
	if [[ "$(echo "$cycles>=$threshold" | bc)" = "1" ]]; then 
		echo "1    $cycles"
	else
	    echo "0    $cycles"
	fi
done

