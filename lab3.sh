#! /bin/bash

NUM_CPUS=16
SIM_CYCLES=10000

echo > network_stats.txt

for SYNTH in uniform_random shuffle transpose tornado neighbor
do
	echo "SYNTHETIC TRAFFIC: $SYNTH" >> network_stats.txt
	for INJ_RATE in 0.02 0.04 0.06 0.08 0.10 0.12 0.14 0.16 0.20 0.24 0.28 0.32 0.36 0.40 0.44 0.48 0.56 0.64 0.72 0.76 0.80
	do
		./build/NULL/gem5.opt \
		configs/example/garnet_synth_traffic.py \
		--network=garnet --num-cpus=$NUM_CPUS --num-dirs=64 \
		--topology=Ring --routing-algorithm=3\
		--inj-vnet=0 --synthetic=$SYNTH \
		--sim-cycles=$SIM_CYCLES --injectionrate=$INJ_RATE
		INJ_TOT=$(grep -Eo "packets_injected::total\s*[0-9.]*" m5out/stats.txt | grep -Eo "[0-9.]*")
		RECV_TOT=$(grep -Eo "packets_received::total\s*[0-9.]*" m5out/stats.txt | grep -Eo "[0-9.]*")
		RECV_RATE=$(echo "scale=6;$RECV_TOT/$NUM_CPUS/$SIM_CYCLES" | bc)
		AVG_PKT_QUEUE_LATENCY=$(grep -Eo "average_packet_queueing_latency\s*[0-9.]*" m5out/stats.txt | grep -Eo "[0-9.]*")
		AVG_PKT_NETWK_LATENCY=$(grep -Eo "average_packet_network_latency\s*[0-9.]*" m5out/stats.txt | grep -Eo "[0-9.]*")
		AVG_PKT_LATENCY=$(grep -Eo "average_packet_latency\s*[0-9.]*" m5out/stats.txt | grep -Eo "[0-9.]*")
		AVG_HOPS=$(grep -Eo "average_hops\s*[0-9.]*" m5out/stats.txt | grep -Eo "[0-9.]*")
		echo "[$INJ_RATE, $INJ_TOT, $RECV_TOT, $RECV_RATE, $AVG_PKT_QUEUE_LATENCY, $AVG_PKT_NETWK_LATENCY, $AVG_PKT_LATENCY, $AVG_HOPS]" >> network_stats.txt
	done
	echo >> network_stats.txt
done

python3 plot.py

NUM_CPUS=64

echo > network_stats.txt
echo "VC TYPE: Wormhole" >> network_stats.txt
for INJ_RATE in 0.02 0.04 0.06 0.08 0.10 0.12 0.14 0.16 0.20 0.24 0.28 0.32 0.36 0.40 0.44 0.48 0.56 0.64 0.72 0.76 0.80
	do
		./build/NULL/gem5.opt \
		configs/example/garnet_synth_traffic.py \
		--network=garnet --num-cpus=$NUM_CPUS --num-dirs=64 \
		--topology=Mesh_XY --mesh-rows=8 \
		--inj-vnet=0 --synthetic=uniform_random \
		--sim-cycles=$SIM_CYCLES --injectionrate=$INJ_RATE --wormhole
		INJ_TOT=$(grep -Eo "packets_injected::total\s*[0-9.]*" m5out/stats.txt | grep -Eo "[0-9.]*")
		RECV_TOT=$(grep -Eo "packets_received::total\s*[0-9.]*" m5out/stats.txt | grep -Eo "[0-9.]*")
		RECV_RATE=$(echo "scale=6;$RECV_TOT/$NUM_CPUS/$SIM_CYCLES" | bc)
		AVG_PKT_QUEUE_LATENCY=$(grep -Eo "average_packet_queueing_latency\s*[0-9.]*" m5out/stats.txt | grep -Eo "[0-9.]*")
		AVG_PKT_NETWK_LATENCY=$(grep -Eo "average_packet_network_latency\s*[0-9.]*" m5out/stats.txt | grep -Eo "[0-9.]*")
		AVG_PKT_LATENCY=$(grep -Eo "average_packet_latency\s*[0-9.]*" m5out/stats.txt | grep -Eo "[0-9.]*")
		AVG_HOPS=$(grep -Eo "average_hops\s*[0-9.]*" m5out/stats.txt | grep -Eo "[0-9.]*")
		echo "[$INJ_RATE, $INJ_TOT, $RECV_TOT, $RECV_RATE, $AVG_PKT_QUEUE_LATENCY, $AVG_PKT_NETWK_LATENCY, $AVG_PKT_LATENCY, $AVG_HOPS]" >> network_stats.txt
	done
	echo >> network_stats.txt


echo "VC TYPE: Single" >> network_stats.txt
for INJ_RATE in 0.02 0.04 0.06 0.08 0.10 0.12 0.14 0.16 0.20 0.24 0.28 0.32 0.36 0.40 0.44 0.48 0.56 0.64 0.72 0.76 0.80
	do
		./build/NULL/gem5.opt \
		configs/example/garnet_synth_traffic.py \
		--network=garnet --num-cpus=$NUM_CPUS --num-dirs=64 \
		--topology=Mesh_XY --mesh-rows=8 \
		--inj-vnet=0 --synthetic=uniform_random \
		--sim-cycles=$SIM_CYCLES --injectionrate=$INJ_RATE --vcs-per-vnet=1
		INJ_TOT=$(grep -Eo "packets_injected::total\s*[0-9.]*" m5out/stats.txt | grep -Eo "[0-9.]*")
		RECV_TOT=$(grep -Eo "packets_received::total\s*[0-9.]*" m5out/stats.txt | grep -Eo "[0-9.]*")
		RECV_RATE=$(echo "scale=6;$RECV_TOT/$NUM_CPUS/$SIM_CYCLES" | bc)
		AVG_PKT_QUEUE_LATENCY=$(grep -Eo "average_packet_queueing_latency\s*[0-9.]*" m5out/stats.txt | grep -Eo "[0-9.]*")
		AVG_PKT_NETWK_LATENCY=$(grep -Eo "average_packet_network_latency\s*[0-9.]*" m5out/stats.txt | grep -Eo "[0-9.]*")
		AVG_PKT_LATENCY=$(grep -Eo "average_packet_latency\s*[0-9.]*" m5out/stats.txt | grep -Eo "[0-9.]*")
		AVG_HOPS=$(grep -Eo "average_hops\s*[0-9.]*" m5out/stats.txt | grep -Eo "[0-9.]*")
		echo "[$INJ_RATE, $INJ_TOT, $RECV_TOT, $RECV_RATE, $AVG_PKT_QUEUE_LATENCY, $AVG_PKT_NETWK_LATENCY, $AVG_PKT_LATENCY, $AVG_HOPS]" >> network_stats.txt
	done
	echo >> network_stats.txt


echo "VC TYPE: Multiple" >> network_stats.txt
for INJ_RATE in 0.02 0.04 0.06 0.08 0.10 0.12 0.14 0.16 0.20 0.24 0.28 0.32 0.36 0.40 0.44 0.48 0.56 0.64 0.72 0.76 0.80
	do
		./build/NULL/gem5.opt \
		configs/example/garnet_synth_traffic.py \
		--network=garnet --num-cpus=$NUM_CPUS --num-dirs=64 \
		--topology=Mesh_XY --mesh-rows=8 \
		--inj-vnet=0 --synthetic=uniform_random \
		--sim-cycles=$SIM_CYCLES --injectionrate=$INJ_RATE --vcs-per-vnet=16
		INJ_TOT=$(grep -Eo "packets_injected::total\s*[0-9.]*" m5out/stats.txt | grep -Eo "[0-9.]*")
		RECV_TOT=$(grep -Eo "packets_received::total\s*[0-9.]*" m5out/stats.txt | grep -Eo "[0-9.]*")
		RECV_RATE=$(echo "scale=6;$RECV_TOT/$NUM_CPUS/$SIM_CYCLES" | bc)
		AVG_PKT_QUEUE_LATENCY=$(grep -Eo "average_packet_queueing_latency\s*[0-9.]*" m5out/stats.txt | grep -Eo "[0-9.]*")
		AVG_PKT_NETWK_LATENCY=$(grep -Eo "average_packet_network_latency\s*[0-9.]*" m5out/stats.txt | grep -Eo "[0-9.]*")
		AVG_PKT_LATENCY=$(grep -Eo "average_packet_latency\s*[0-9.]*" m5out/stats.txt | grep -Eo "[0-9.]*")
		AVG_HOPS=$(grep -Eo "average_hops\s*[0-9.]*" m5out/stats.txt | grep -Eo "[0-9.]*")
		echo "[$INJ_RATE, $INJ_TOT, $RECV_TOT, $RECV_RATE, $AVG_PKT_QUEUE_LATENCY, $AVG_PKT_NETWK_LATENCY, $AVG_PKT_LATENCY, $AVG_HOPS]" >> network_stats.txt
	done
	echo >> network_stats.txt

python3 plot.py
