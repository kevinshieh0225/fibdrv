reset
set xlabel 'F(n)'
set ylabel 'time (ns)'
set title 'Fibonacci runtime'
set term png enhanced font 'Verdana,10'
set output 'plot_statistic.png'
set grid
plot [0:1000][0:100000] \
'plot_input_statistic' \
using 1:2 with linespoints linewidth 2 title "bn fib sequence",\
'plot_input_statistic' \
using 1:3 with linespoints linewidth 2 title "bn fast doubling",\
# 'plot_input_statistic' \
# using 1:4 with linespoints linewidth 2 title "system call overall",\
