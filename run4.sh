#!/bin/bash

OUTFILE="results.txt"
: > "$OUTFILE"

# VC 数量
VC_LIST=(1 2 3 4)

# 路由算法 (保持和 RoutingUnit.cc 一致)
declare -A ROUTERS
ROUTERS["DOR3D"]=4
ROUTERS["OPAR3D"]=5
ROUTERS["PAR3D"]=6

# traffic pattern
PATTERNS=("uniform_random" "shuffle" "transpose" "tornado" "neighbor")

# 注入率
RATES="0.01 0.02 0.04 0.07 0.10 0.15 0.20 0.25 0.30 0.40 0.50 0.60"

CYCLES=10000

# 单个实验任务
run_one() {
    vcs=$1
    pat=$2
    algoname=$3
    algo=$4
    rate=$5

    rundir="runs/VC${vcs}_${pat}_${algoname}_rate${rate}"
    mkdir -p "$rundir"
    rm -rf "$rundir/m5out"

    echo "Running: VC=$vcs | Pattern=$pat | Algo=$algoname | rate=$rate"

    ./build/NULL/gem5.opt \
      --outdir="$rundir/m5out" \
      configs/example/garnet_synth_traffic.py \
      --network=garnet --num-cpus=64 --num-dirs=64 \
      --topology=Mesh_3D --mesh-rows=4 --mesh-cols=4 \
      --routing-algorithm=$algo \
      --inj-vnet=0 --synthetic=$pat \
      --sim-cycles=$CYCLES --injectionrate=$rate \
      --router-latency=1 --link-width-bits=16 \
      --garnet-deadlock-threshold=100000000 \
      --vcs-per-vnet=$vcs > "$rundir/run.log" 2>&1

    echo "Finished: VC=$vcs | Pattern=$pat | Algo=$algoname | rate=$rate"

    stats="$rundir/m5out/stats.txt"

    {
      echo "====== VC=$vcs | Pattern=$pat | Algo=$algoname | rate=$rate ======"
      if [[ ! -f $stats ]]; then
        echo "FAILED (no stats)"
      else
        inj=$(grep "system.ruby.network.packets_injected::total" $stats | awk '{print $2}')
        rec=$(grep "system.ruby.network.packets_received::total" $stats | awk '{print $2}')
        ratio=$(awk -v a="$rec" -v b="$inj" 'BEGIN{if (b>0) printf "%.6f", a/b; else print "NaN"}')

        avg_net_lat=$(grep "system.ruby.network.average_packet_network_latency" $stats | awk '{print $2}')
        avg_que_lat=$(grep "system.ruby.network.average_packet_queueing_latency" $stats | awk '{print $2}')
        avg_hops=$(grep "system.ruby.network.average_hops" $stats | awk '{print $2}')
        recv_rate=$(grep "system.ruby.network.reception_rate" $stats | awk '{print $2}')

        echo "packets_injected = ${inj:-NaN}"
        echo "packets_received = ${rec:-NaN}"
        echo "packets_received_ratio = ${ratio:-NaN}"
        echo "avg_network_latency = ${avg_net_lat:-NaN}"
        echo "avg_queueing_latency = ${avg_que_lat:-NaN}"
        echo "average_hops = ${avg_hops:-NaN}"
        echo "throughput = ${recv_rate:-NaN}"
      fi

      echo
    } >> "$OUTFILE"
}

export -f run_one

# 构造任务列表（逗号分隔）
JOBS=()
for vcs in "${VC_LIST[@]}"; do
  for pat in "${PATTERNS[@]}"; do
    for algoname in "${!ROUTERS[@]}"; do
      algo=${ROUTERS[$algoname]}
      for rate in $RATES; do
        JOBS+=("$vcs,$pat,$algoname,$algo,$rate")
      done
    done
  done
done

# 并行执行
printf "%s\n" "${JOBS[@]}" | while IFS=, read -r vcs pat algoname algo rate; do
  run_one "$vcs" "$pat" "$algoname" "$algo" "$rate" &
  [[ $(jobs -r -p | wc -l) -ge $(nproc) ]] && wait -n
done
wait
