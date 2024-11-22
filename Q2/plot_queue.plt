set terminal pngcairo size 800,600
set output 'queue_vs_time.png'

set title "Queue Size vs Time"
set xlabel "Time (seconds)"
set ylabel "Queue Size"
set grid

plot "queue_data.txt" using 1:2 with lines title "Queue Size"
