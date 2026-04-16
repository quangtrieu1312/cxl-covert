#!/usr/bin/env bash
echo "Start checking next_bit_window in log files and find discrepencies."
echo "If this doesnt print anything then you are good."
grep "next_bit_window" /tmp/cxl_covert_sender.log | while read line; do if [[ $(grep -c "$line" /tmp/cxl_covert_reader.log) -eq 0 ]]; then echo $line; fi; done
