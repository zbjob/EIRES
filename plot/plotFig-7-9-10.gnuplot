#!/usr/local/bin/gnuplot
set terminal postscript eps color  enhanced "Helvetica-Bold" 65 
set size 4, 2 
set border lw 5 

# LINE STYLES

set style line 1 lt -1 lw 8 lc rgb "grey" # Percentiles
set style line 2 lt -1 lw 8 lc rgb "grey" # Median
set style line 3 lt -1 ps 8 pt 1  lw 15  # NS : non-shedding 
set style line 4 lt -1 ps 8 pt 2  lw 15 lc rgb "olive" # RIS
set style line 5 lt -1 ps 8 pt 6  lw 15 lc rgb "dark-green" # PDfetch 
set style line 6 lt -1 ps 8 pt 4  lw 15 lc rgb "magenta" # RPMS
set style line 7 lt -1 ps 8 pt 3  lw 15 lc rgb "blue" # Pfetch 
set style line 8 lt -1 ps 8 pt 8  lw 15 lc rgb "red" # Dfetch 

set style line 22 lt -1 ps 8 pt 8  lw 20 lc rgb "black" #baseline1 
set style line 23 lt -1 ps 8 pt 8  lw 15 lc rgb "gray" #baseline2 

set style line 9 lt -1 ps 6 pt 8  lw 20 lc rgb "blue" # event
set style line 10 lt -1 ps 6 pt 14 lw 20 lc rgb "red" # PM 

set style line 11 lt -1 ps 5 pt 8  lw 12 lc rgb "blue" # Precision 
set style line 12 lt -1 ps 5 pt 14 lw 12 lc rgb "red" # Recall 


set style line 13 dt 5  pt 1  lw 5 lc rgb "dark-green" #PD
set style line 14 dt 5  pt 1  lw 5 lc rgb "blue"  #P
set style line 15 dt 5  pt 1  lw 5 lc rgb "red" #D


set style line 16 lt -1 ps 8 pt 1  lw 15 lc rgb "green" #PDblock
set style line 17 lt -1 ps 8 pt 2  lw 15 lc rgb "dark-blue"  #Pblock
set style line 18 lt -1 ps 8 pt 3  lw 15 lc rgb "dark-pink"  #Dblock


set style line 20 lt -1  pt 1  lw 10 lc rgb "dark-green" #PD95pc
set style line 21 dt  5  pt 1  lw 10 lc rgb "dark-green" #PD50pc


Precision=11
Recall=12

Pfetch=7
Dfetch=8
PDfetch=5

baseline1=22
baseline2=23

PfetchPM=7
DfetchPM=8
PDfetchPM=16

PBlock=17
DBlock=18
PDBlock=5

P95pc=14
D95pc=15
PD95pc=13

PDCtrl95pc=20
PDCtrl50pc=21

RISBox="fs solid 1 lt -1 lw 6 lc rgb 'olive'"
RPMSBox="fs solid 1 lt -1 lw 6 lc rgb 'magenta'" 
SlISBox="fs solid 1 lt -1 lw 6 lc rgb 'green'"
SlPMSBox="fs solid  1 lt -1 lw 6 lc rgb 'blue'"
HybridBox="fs solid 1 lt -1 lw 6 lc rgb 'red'"

#IdxBox="fs pattern 0 lt -1 lw 6"
#NoIdxBox="fs solid 0.5 lt -1 lw 6"
NoIdxBox="fs solid  1 lt -1 lw 6 lc rgb 'blue'"
IdxBox="fs solid 1 lt -1 lw 6 lc rgb 'red'"

#plot "ACC_plt.dat" using 2:xtic(1) ti col  fs solid 0.25 lt -1 lw 6 , '' u 3 ti col  fs pattern 1 lt -1 lw 6 , '' u 4 ti col  fs pattern 5 lt -1 lw 6, '' u 5 ti col  fs pattern 4 lt -1 lw 6, '' u 6 ti col fs pattern 3 lt -1 lw 6

scientificNotation="\"%2.1t{/Symbol \264}10^{%L}\""

BoxSizeCache=150
BoxSizeFetchLatency=0.25
BoxSizeFetchRatio=1.5

# TITLES

# (X,Y)-AXIS LABELS

CLEAR="\
unset boxwidth;   \
unset style fill; \
unset log  x;     \
unset log  y;     \
unset log y2;     \
unset xrange;     \
unset yrange;     \
unset y2range;    \
unset xlabel;     \
unset ylabel;     \
unset y2label;    \
unset xtics;      \
set   xtics;      \
unset ytics;      \
set   ytics;      \
set format y \"%.0f\";\
set format y2 \"%.0f\";\
set xtics font \",65\"; \
set ytics font \",65\"; \
unset y2tics;     \
unset key;        \
set key font \",65\"; \
set terminal postscript eps color enhanced \"Helvetica-Bold\" 65"
TwoInOneColumnFont="\
set terminal postscript eps color enhanced \"Helvetica\" 48; \
set key font \",32\""

ThreeInTwoColumnsFont="\
set terminal postscript eps color enhanced \"Helvetica\" 40; \
set key font \",30\""


############ Figure 9. Sensitivity analysis for utility weighting factor.
set output "bin_prefetch/Figure9b.eps"
set size 2,3

set border lw 10

set ylabel "Latency (us)"
set xlabel "Weight {/Symbol w} in Equation 5"
set xrange [0:6] 
set xtics("0.1" 1, "0.3" 2, "0.5" 3, "0.7" 4, "0.9" 5)
set boxwidth 0.7 
set ytics 0, 500, 4000
#set format y @scientificNotation
set yrange [0:4000]

plot \
'result_latency_weight_cache.dat' using ($1):($6):($5):($9):($8) with candlesticks ls baseline1 notitle whiskerbars, \
'' using ($1):($7):($7):($7):($7) with candlesticks ls baseline1 notitle
@CLEAR

set output "bin_prefetch/Figure9a.eps"
set size 2,3

set border lw 10

set ylabel "Latency (us)"
set xlabel "Weight {/Symbol w} in Equation 5"
set xrange [0:6] 
set xtics("0.1" 1, "0.3" 2, "0.5" 3, "0.7" 4, "0.9" 5)
set boxwidth 0.7 
set ytics 0, 300, 1500 
#set format y @scientificNotation
set yrange [0:1500]

plot \
'result_latency_weight_fetch.dat' using ($1):($6):($5):($9):($8) with candlesticks ls baseline1 notitle whiskerbars, \
'' using ($1):($7):($7):($7):($7) with candlesticks ls baseline1 notitle
@CLEAR

################## Figure 10. Case studies.
set output "bin_prefetch/Figure10a.eps"
set size 2,2

set border lw 10

set ylabel "Latency (ms)"
set xtic rotate by -30 scale 0
set xrange [0:7] 
set xtics("BL1" 1, "BL2" 2, "BL3" 3, "PFetch" 4, "LzEval" 5, "Hybrid" 6)
set boxwidth 0.7 
set ytics("0" 0, "50" 50000, "100" 100000, "150" 150000, "200" 200000) 
#set format y @scientificNotation
set yrange [0:200000]

plot \
'result_latency_bushfire.dat' using ($1):($6):($5):($9):($8) with candlesticks ls baseline1 notitle whiskerbars, \
'' using ($1):($7):($7):($7):($7) with candlesticks ls baseline1 notitle
@CLEAR


#########################
#set output "bin_prefetch/google_cluster_general_BL3.eps"
#set format y @scientificNotation
#set size 2,3
#set multiplot 
#set border lw 10
#set ylabel "Latency (ms)" offset 0,4.0,0
#
#set xtic rotate by -30 scale 0
#set xrange [0:7] 
#set xtics("BL1" 1, "BL2" 2, "BL3" 3, "PFetch" 4, "LzEval" 5, "Hybrid" 6)
#set boxwidth 0.7 
#
#unset title                          
#unset border                         
#set border 1+2+8                     
#set origin 0,0                       
#set size 2,2    
#set ytics("   0" 0, "10" 10000, "20" 20000, "30" 30000, "40" 40000) 
#set format y @scientificNotation
#set yrange [0:40000]
#
#plot \
#'prefetch/google_cluster/google_cluster_BL3.dat' using ($1):($6):($5):($9):($8) with candlesticks ls baseline1 notitle whiskerbars, \
#'' using ($1):($7):($7):($7):($7) with candlesticks ls baseline1 notitle
#
#unset border                         
#set border 4+2+8                     
#set origin 0,2                       
#set size 2,1                         
#set ylabel " "                       
#unset xlabel                         
#unset xtics                          
#set ytics auto                       
#set yrange [50000:3700000]          
#set ytics 50000, 1000000, 3700000 
##set format y @scientificNotation
#set ytics("500" 500000, "1500" 1500000, "2500" 2500000, "3500" 3500000) 
#plot \
#'prefetch/google_cluster/google_cluster_BL3.dat' using ($1):($6):($5):($9):($8) with candlesticks ls baseline1 notitle whiskerbars, \
#'' using ($1):($7):($7):($7):($7) with candlesticks ls baseline1 notitle
#unset multiplot
#@CLEAR
#set border lw 5
#set border 1+4+2+8
####################

#########################
set output "bin_prefetch/Figure10b.eps"
set format y @scientificNotation
set size 2,2
set origin 0,0                       
set multiplot 
set border lw 10
set ylabel "Latency (ms)" offset 0,2.1,0

set xtic rotate by -30 scale 0
set xrange [0:7] 
set xtics("BL1" 1, "BL2" 2, "BL3" 3, "PFetch" 4, "LzEval" 5, "Hybrid" 6)
set boxwidth 0.7 

unset title                          
unset border                         
set border 1+2+8                     
set origin 0,0                       
set size 2,1.45    
#set ytics("   0" 0, "10" 10000, "20" 20000, "30" 30000, "40" 40000) 
set ytics("   0" 0, "5" 5000, "10" 10000, "15" 15000, "20" 20000, "25" 25000) 
#set ytics("   0" 0, "5" 5000, "10" 10000, "20" 20000, "30" 30000, "40" 40000) 
set format y @scientificNotation
set yrange [0:28000]

plot \
'result_latency_google_cluster.dat' using ($1):($6):($5):($9):($8) with candlesticks ls baseline1 notitle whiskerbars, \
'' using ($1):($7):($7):($7):($7) with candlesticks ls baseline1 notitle

unset border                         
set border 4+2+8                     
set origin 0,1.3                       
set size 2,0.7                         
set ylabel " "                       
unset xlabel                         
unset xtics                          
set ytics auto                       
#set yrange [50000:3700000]          
set yrange [100000:1000000]          
set ytics 50000, 1000000, 3700000 
#set format y @scientificNotation
set ytics("100" 100000, "500" 500000, "1000" 1000000) 
plot \
'result_latency_google_cluster.dat' using ($1):($6):($5):($9):($8) with candlesticks ls baseline1 notitle whiskerbars, \
'' using ($1):($7):($7):($7):($7) with candlesticks ls baseline1 notitle
unset multiplot
@CLEAR
set border lw 5
set border 1+4+2+8

#################### Figure 7. Throughput for Q1 under non-greedy selection###############
set output "bin_prefetch/Figure7a.eps"
set size 2,2
set border lw 10
set ylabel "Events per second"
set xtic rotate by -30 scale 0
set xrange [0:7] 
set xtics("BL1" 1, "BL2" 2, "BL3" 3, "PFetch" 4, "LzEval" 5, "Hybrid" 6)
set boxwidth 0.7 
unset ytics
set ytics 0, 20000, 80000
set format y @scientificNotation
set yrange [0:80000]

plot \
'result_throughput_cost_non_greedy.dat' using ($1):($6):($5):($9):($8) with candlesticks ls baseline1 notitle whiskerbars, \
'' using ($1):($7):($7):($7):($7) with candlesticks ls baseline1 notitle
@CLEAR

set output "bin_prefetch/Figure7b.eps"
set size 2,2
set border lw 10
set ylabel "Events per second"
set xtic rotate by -30 scale 0
set xrange [0:7] 
set xtics("BL1" 1, "BL2" 2, "BL3" 3, "PFetch" 4, "LzEval" 5, "Hybrid" 6)
set boxwidth 0.7 
unset ytics
set ytics 0, 20000, 80000
set format y @scientificNotation
set yrange [0:80000]

plot \
'result_throughput_LRU_non_greedy.dat' using ($1):($6):($5):($9):($8) with candlesticks ls baseline1 notitle whiskerbars, \
'' using ($1):($7):($7):($7):($7) with candlesticks ls baseline1 notitle
@CLEAR
