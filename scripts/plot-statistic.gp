reset
set xlabel 'F(n)'
set ylabel 'time (ns)'
set title 'Fibonacci runtime'
set term png enhanced font 'Verdana,10'
set output 'plot_statistic.png'
set grid
plot [0:10000][0:240000] \
'plot_bn_fd_v2' \
using 1:2 with linespoints linewidth 2 title "bn fd v2",\
'plot_bn_fd_v3_bn_mult' \
using 1:2 with linespoints linewidth 2 title "bn fd v3: bn mult",\
# 'plot_input_statistic' \
# using 1:4 with linespoints linewidth 2 title "system call overall",\
