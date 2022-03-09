reset
set xlabel 'F(n)'
set ylabel 'time (ns)'
set title 'Fibonacci runtime'
set term png enhanced font 'Verdana,10'
set output 'plot_statistic.png'
set grid
plot [0:92][0:300] \
'plot_input_statistic' \
using 1:2 with linespoints linewidth 2 title "iterative",\
'' using 1:3 with linespoints linewidth 2 title "1U << 31",\
'' using 1:4 with linespoints linewidth 2 title "1U << 16",\
'' using 1:5 with linespoints linewidth 2 title "1U << 6",\
'' using 1:6 with linespoints linewidth 2 title "1U << (31 - builtinclz(k))"