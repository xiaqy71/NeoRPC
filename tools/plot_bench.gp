set datafile separator ","
set key left top
set grid
set terminal pngcairo size 1000,600

set output "throughput_by_mode.png"
set title "NeoRPC Throughput by Mode"
set xlabel "clients"
set ylabel "msg/sec"
plot "bench.csv" using (strcol(2) eq "echo" ? $1 : 1/0):8 with linespoints linewidth 2 title "echo", \
     "bench.csv" using (strcol(2) eq "ping" ? $1 : 1/0):8 with linespoints linewidth 2 title "ping"

set output "latency_echo.png"
set title "NeoRPC Latency - echo"
set xlabel "clients"
set ylabel "latency (ms)"
plot "bench.csv" using (strcol(2) eq "echo" ? $1 : 1/0):9 with linespoints linewidth 2 title "p50", \
     "bench.csv" using (strcol(2) eq "echo" ? $1 : 1/0):10 with linespoints linewidth 2 title "p95", \
     "bench.csv" using (strcol(2) eq "echo" ? $1 : 1/0):11 with linespoints linewidth 2 title "p99"

set output "latency_ping.png"
set title "NeoRPC Latency - ping"
set xlabel "clients"
set ylabel "latency (ms)"
plot "bench.csv" using (strcol(2) eq "ping" ? $1 : 1/0):9 with linespoints linewidth 2 title "p50", \
     "bench.csv" using (strcol(2) eq "ping" ? $1 : 1/0):10 with linespoints linewidth 2 title "p95", \
     "bench.csv" using (strcol(2) eq "ping" ? $1 : 1/0):11 with linespoints linewidth 2 title "p99"
