import matplotlib.pyplot as plt
import re

netwk_stats = open("network_stats.txt", "r")

name = ""
names = []
inj_rate = []
avg_packet_latency = []
avg_hops = []
type_variable = ""

plt.rcParams["lines.markersize"] = 3
plt.title("Average Packet Latency vs Injection Rate")
plt.xlabel("Injection Rate (Packet/Node/Cycle)")
plt.ylabel("Average Packet Latency (Tick)")
plt.yscale("log")


def extract_numbers(text):
    pattern = r"-?\d+\.?\d*"
    numbers = re.findall(pattern, text)
    result = []
    for num_str in numbers:
        if "." in num_str:
            result.append(float(num_str))
        else:
            result.append(int(num_str))
    return result


line = netwk_stats.readline()
while line:
    if "SYNTHETIC TRAFFIC" in line:
        type_variable = "SYNTHETIC_TRAFFIC"
        if name != "":
            plt.plot(inj_rate, avg_packet_latency, marker="o", label=name)
        name = line.split()[-1]
        names.append(name)
        inj_rate.clear()
        avg_packet_latency.clear()
        avg_hops.clear()
    elif "VCS PER VNET" in line:
        type_variable = "VCS_PER_VNET"
        if name != "":
            plt.plot(inj_rate, avg_packet_latency, marker="o", label=name)
        name = line.split()[-1]
        names.append(name)
        inj_rate.clear()
        avg_packet_latency.clear()
        avg_hops.clear()
    elif "ROUTER LATENCY" in line:
        type_variable = "ROUTER_LATENCY"
        if name != "":
            plt.plot(inj_rate, avg_packet_latency, marker="o", label=name)
        name = line.split()[-1]
        names.append(name)
        inj_rate.clear()
        avg_packet_latency.clear()
        avg_hops.clear()
    elif "LINK WIDTH BITS" in line:
        type_variable = "LINK_WIDTH_BITS"
        if name != "":
            plt.plot(inj_rate, avg_packet_latency, marker="o", label=name)
        name = line.split()[-1]
        names.append(name)
        inj_rate.clear()
        avg_packet_latency.clear()
        avg_hops.clear()
    elif "VC TYPE" in line:
        type_variable = "VC_TYPE"
        if name != "":
            plt.plot(inj_rate, avg_packet_latency, marker="o", label=name)
        name = line.split()[-1]
        names.append(name)
        inj_rate.clear()
        avg_packet_latency.clear()
        avg_hops.clear()
    elif line != "\n":
        data = extract_numbers(line)
        inj_rate.append(data[0])
        avg_packet_latency.append(data[-2])
        avg_hops.append(data[-1])
    line = netwk_stats.readline()
if name != "":
    plt.plot(inj_rate, avg_packet_latency, marker="o", label=name)

plt.legend(names)
plt.savefig("lab3-" + type_variable + ".png")
