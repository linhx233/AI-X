from m5.params import *
from m5.objects import *

from common import FileSystemConfig
from topologies.BaseTopology import SimpleTopology


class Mesh_3D(SimpleTopology):
    description = "Mesh_3D"

    def __init__(self, controllers):
        self.nodes = controllers

    def makeTopology(self, options, network, IntLink, ExtLink, Router):
        nodes = self.nodes

        num_routers = options.num_cpus
        num_rows = options.mesh_rows
        num_cols = options.mesh_cols

        link_latency = options.link_latency
        router_latency = options.router_latency

        # controller 分配逻辑
        cntrls_per_router, remainder = divmod(len(nodes), num_routers)
        assert (
            num_rows > 0
            and num_cols > 0
            and num_rows * num_cols <= num_routers
        )
        num_depths = int(num_routers / (num_rows * num_cols))
        assert num_rows * num_cols * num_depths == num_routers

        # Create routers
        routers = [
            Router(router_id=i, latency=router_latency)
            for i in range(num_routers)
        ]
        network.routers = routers

        link_count = 0

        network_nodes = []
        remainder_nodes = []
        for node_index in range(len(nodes)):
            if node_index < (len(nodes) - remainder):
                network_nodes.append(nodes[node_index])
            else:
                remainder_nodes.append(nodes[node_index])

        # 把 controllers 平均挂到 routers 上
        ext_links = []
        for (i, n) in enumerate(network_nodes):
            cntrl_level, router_id = divmod(i, num_routers)
            assert cntrl_level < cntrls_per_router
            ext_links.append(
                ExtLink(
                    link_id=link_count,
                    ext_node=n,
                    int_node=routers[router_id],
                    latency=link_latency,
                )
            )
            link_count += 1

        # 剩下的都挂 router 0（一般是 DMA）
        for (i, node) in enumerate(remainder_nodes):
            assert node.type == "DMA_Controller"
            assert i < remainder
            ext_links.append(
                ExtLink(
                    link_id=link_count,
                    ext_node=node,
                    int_node=routers[0],
                    latency=link_latency,
                )
            )
            link_count += 1

        network.ext_links = ext_links

        # router_id helper
        def rid(x, y, z):
            return z * (num_rows * num_cols) + y * num_cols + x

        # int_links
        int_links = []

        # X dimension (东西向)
        for z in range(num_depths):
            for y in range(num_rows):
                for x in range(num_cols - 1):
                    n0, n1 = rid(x, y, z), rid(x + 1, y, z)
                    int_links.append(
                        IntLink(
                            link_id=link_count,
                            src_node=routers[n0],
                            dst_node=routers[n1],
                            src_outport="East",
                            dst_inport="West",
                            latency=link_latency,
                            weight=1,
                        )
                    )
                    link_count += 1
                    int_links.append(
                        IntLink(
                            link_id=link_count,
                            src_node=routers[n1],
                            dst_node=routers[n0],
                            src_outport="West",
                            dst_inport="East",
                            latency=link_latency,
                            weight=1,
                        )
                    )
                    link_count += 1

        # Y dimension (南北向)
        for z in range(num_depths):
            for x in range(num_cols):
                for y in range(num_rows - 1):
                    n0, n1 = rid(x, y, z), rid(x, y + 1, z)
                    int_links.append(
                        IntLink(
                            link_id=link_count,
                            src_node=routers[n0],
                            dst_node=routers[n1],
                            src_outport="North",
                            dst_inport="South",
                            latency=link_latency,
                            weight=2,
                        )
                    )
                    link_count += 1
                    int_links.append(
                        IntLink(
                            link_id=link_count,
                            src_node=routers[n1],
                            dst_node=routers[n0],
                            src_outport="South",
                            dst_inport="North",
                            latency=link_latency,
                            weight=2,
                        )
                    )
                    link_count += 1

        # Z dimension (上下层)
        for y in range(num_rows):
            for x in range(num_cols):
                for z in range(num_depths - 1):
                    n0, n1 = rid(x, y, z), rid(x, y, z + 1)
                    int_links.append(
                        IntLink(
                            link_id=link_count,
                            src_node=routers[n0],
                            dst_node=routers[n1],
                            src_outport="Up",
                            dst_inport="Down",
                            latency=link_latency,
                            weight=3,
                        )
                    )
                    link_count += 1
                    int_links.append(
                        IntLink(
                            link_id=link_count,
                            src_node=routers[n1],
                            dst_node=routers[n0],
                            src_outport="Down",
                            dst_inport="Up",
                            latency=link_latency,
                            weight=3,
                        )
                    )
                    link_count += 1

        network.int_links = int_links

    def registerTopology(self, options):
        for i in range(options.num_cpus):
            FileSystemConfig.register_node(
                [i], MemorySize(options.mem_size) // options.num_cpus, i
            )
