#!/bin/bash

OUTFILE="collected_results.txt"
: > "$OUTFILE"

# 遍历 runs 下所有实验目录
for rundir in runs/*; do
  stats="$rundir/m5out/stats.txt"
  if [[ -f $stats ]]; then
    # 从目录名提取配置信息
    cfg=$(basename "$rundir")

    inj=$(grep "system.ruby.network.packets_injected::total" $stats | awk '{print $2}')
    rec=$(grep "system.ruby.network.packets_received::total" $stats | awk '{print $2}')
    ratio=$(awk -v a="$rec" -v b="$inj" 'BEGIN{if (b>0) printf "%.6f", a/b; else print "NaN"}')

    avg_net_lat=$(grep "system.ruby.network.average_packet_network_latency" $stats | awk '{print $2}')
    avg_que_lat=$(grep "system.ruby.network.average_packet_queueing_latency" $stats | awk '{print $2}')
    avg_lat=$(grep "system.ruby.network.average_packet_latency" $stats | awk '{print $2}')
    avg_hops=$(grep "system.ruby.network.average_hops" $stats | awk '{print $2}')
    recv_rate=$(grep "system.ruby.network.reception_rate" $stats | awk '{print $2}')

    {
      echo "====== $cfg ======"
      echo "packets_injected = ${inj:-NaN}"
      echo "packets_received = ${rec:-NaN}"
      echo "packets_received_ratio = ${ratio:-NaN}"
      echo "avg_network_latency = ${avg_net_lat:-NaN}"
      echo "avg_queueing_latency = ${avg_que_lat:-NaN}"
      echo "average_latency = ${avg_lat:-NaN}"
      echo "average_hops = ${avg_hops:-NaN}"
      echo "throughput = ${recv_rate:-NaN}"
      echo
    } >> "$OUTFILE"
  else
    echo "====== $(basename "$rundir") ======" >> "$OUTFILE"
    echo "NO STATS (maybe deadlock or crash)" >> "$OUTFILE"
    echo >> "$OUTFILE"
  fi
done

echo "✅ 提取完成，结果已保存到 $OUTFILE"
