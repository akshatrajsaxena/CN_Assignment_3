#!/bin/bash
input_file="tcp-example.tr"
output_file="queue_data.txt"

# Initialize queue size
queue_size=0
    
# Create the output file and write the header
echo -e "#Time\tQueueSize" > "$output_file"

# Parse the trace file
while read -r line; do
  event_type=$(echo "$line" | awk '{print $1}')
  event_time=$(echo "$line" | awk '{print $2}')

  # Handle enqueue and dequeue events
  if [[ "$event_type" == "+" ]]; then
    queue_size=$((queue_size + 1))
    echo -e "$event_time\t$queue_size" >> "$output_file"
  elif [[ "$event_type" == "-" ]]; then
    queue_size=$((queue_size - 1))
    echo -e "$event_time\t$queue_size" >> "$output_file"
  fi
done < "$input_file"
