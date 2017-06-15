# Nathan Tsai
# nwtsai@gmail.com
# 304575323

# Script for using gnuplot

# General plot parameters
set terminal png
set datafile separator ","

# GRAPH 1
set title "Scalability-1: Throughput of Synchronized Lists"
set xlabel "Threads"
set logscale x 2
set ylabel "Throughput (operations/sec)"
set logscale y 10
set output 'lab2b_1.png'
set key left top
plot \
     "< grep -e 'list-none-m,[0-9]*,1000,1,' lab2b_list.csv" using ($2):(1000000000)/($7) \
	title 'list ins/lookup/delete w/mutex' with linespoints lc rgb 'orange', \
     "< grep -e 'list-none-s,[0-9]*,1000,1,' lab2b_list.csv" using ($2):(1000000000)/($7) \
	title 'list ins/lookup/delete w/spin' with linespoints lc rgb 'purple'

# GRAPH 2
set title "Scalability-2: Per-operation Times for Mutex-Protected List Operations"
set xlabel "Threads"
set logscale x 2
set ylabel "mean time/operation (ns)"
set logscale y 10
set output 'lab2b_2.png'
set key left top
plot \
     "< grep 'list-none-m,[0-9]*,1000,1,' lab2b_list.csv | grep -v 'list-none-m,12,1000,1,'" using ($2):($7) \
	title 'completion time' with linespoints lc rgb 'orange', \
     "< grep 'list-none-m,[0-9]*,1000,1,' lab2b_list.csv | grep -v 'list-none-m,12,1000,1,'" using ($2):($8) \
	title 'wait for lock' with linespoints lc rgb 'red'

# GRAPH 3
set title "Scalability-3: Correct Synchronization of Partitioned Lists"
set xlabel "Threads"
set logscale x 2
set xrange [0.75:]
set ylabel "Successful Iterations"
set logscale y 10
set output 'lab2b_3.png'
# note that unsuccessful runs should have produced no output
plot \
     "< grep 'list-id-none,[0-9]*,[0-9]*,4,' lab2b_list.csv" using ($2):($3) \
	title 'unprotected' with points lc rgb 'red', \
     "< grep 'list-id-m,[0-9]*,[0-9]*,4,' lab2b_list.csv" using ($2):($3) \
	title 'Mutex' with points lc rgb 'green', \
     "< grep 'list-id-s,[0-9]*,[0-9]*,4,' lab2b_list.csv" using ($2):($3) \
	title 'Spin-Lock' with points lc rgb 'blue'

# GRAPH 4
set title "Scalability-4: Throughput of Mutex-Synchronized Partitioned Lists"
set xlabel "Threads"
set logscale x 2
set ylabel "Throughput (operations/sec)"
set logscale y 10
set output 'lab2b_4.png'
plot \
	"< grep 'list-none-m,[0-9]*,1000,1,' lab2b_list.csv | grep -v 'list-none-m,16,1000,1,' | grep -v 'list-none-m,24,1000,1,'" using ($2):(1000000000)/($7) \
	title 'lists=1' with linespoints lc rgb 'purple', \
	"< grep 'list-none-m,[0-9]*,1000,4,' lab2b_list.csv" using ($2):(1000000000)/($7) \
	title 'lists=4' with linespoints lc rgb 'green', \
	"< grep 'list-none-m,[0-9]*,1000,8,' lab2b_list.csv" using ($2):(1000000000)/($7) \
	title 'lists=8' with linespoints lc rgb 'blue', \
	"< grep 'list-none-m,[0-9]*,1000,16,' lab2b_list.csv" using ($2):(1000000000)/($7) \
	title 'lists=16' with linespoints lc rgb 'orange'

# GRAPH 5
set title "Scalability-5: Throughput of Spin-Lock-Synchronized Partitioned Lists"
set xlabel "Threads"
set logscale x 2
set ylabel "Throughput (operations/sec)"
set logscale y 10
set output 'lab2b_5.png'
plot \
	"< grep 'list-none-s,[0-9]*,1000,1,' lab2b_list.csv | grep -v 'list-none-s,16,1000,1,' | grep -v 'list-none-s,24,1000,1,'" using ($2):(1000000000)/($7) \
	title 'lists=1' with linespoints lc rgb 'purple', \
	"< grep 'list-none-s,[0-9]*,1000,4,' lab2b_list.csv" using ($2):(1000000000)/($7) \
	title 'lists=4' with linespoints lc rgb 'green', \
	"< grep 'list-none-s,[0-9]*,1000,8,' lab2b_list.csv" using ($2):(1000000000)/($7) \
	title 'lists=8' with linespoints lc rgb 'blue', \
	"< grep 'list-none-s,[0-9]*,1000,16,' lab2b_list.csv" using ($2):(1000000000)/($7) \
	title 'lists=16' with linespoints lc rgb 'orange'