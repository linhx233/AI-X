/*
 * Copyright (c) 2008 Princeton University
 * Copyright (c) 2016 Georgia Institute of Technology
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met: redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer;
 * redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the distribution;
 * neither the name of the copyright holders nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */


#include "mem/ruby/network/garnet/RoutingUnit.hh"

#include "base/cast.hh"
#include "base/compiler.hh"
#include "debug/RubyNetwork.hh"
#include "mem/ruby/network/garnet/InputUnit.hh"
#include "mem/ruby/network/garnet/Router.hh"
#include "mem/ruby/network/garnet/OutputUnit.hh"
#include "mem/ruby/slicc_interface/Message.hh"

namespace gem5
{

namespace ruby
{

namespace garnet
{

RoutingUnit::RoutingUnit(Router *router)
{
    m_router = router;
    m_routing_table.clear();
    m_weight_table.clear();
}

void
RoutingUnit::addRoute(std::vector<NetDest>& routing_table_entry)
{
    if (routing_table_entry.size() > m_routing_table.size()) {
        m_routing_table.resize(routing_table_entry.size());
    }
    for (int v = 0; v < routing_table_entry.size(); v++) {
        m_routing_table[v].push_back(routing_table_entry[v]);
    }
}

void
RoutingUnit::addWeight(int link_weight)
{
    m_weight_table.push_back(link_weight);
}

bool
RoutingUnit::supportsVnet(int vnet, std::vector<int> sVnets)
{
    // If all vnets are supported, return true
    if (sVnets.size() == 0) {
        return true;
    }

    // Find the vnet in the vector, return true
    if (std::find(sVnets.begin(), sVnets.end(), vnet) != sVnets.end()) {
        return true;
    }

    // Not supported vnet
    return false;
}

/*
 * This is the default routing algorithm in garnet.
 * The routing table is populated during topology creation.
 * Routes can be biased via weight assignments in the topology file.
 * Correct weight assignments are critical to provide deadlock avoidance.
 */
std::pair<int,int>
RoutingUnit::lookupRoutingTable(int vnet, NetDest msg_destination)
{
    // First find all possible output link candidates
    // For ordered vnet, just choose the first
    // (to make sure different packets don't choose different routes)
    // For unordered vnet, randomly choose any of the links
    // To have a strict ordering between links, they should be given
    // different weights in the topology file

    int output_link = -1;
    int min_weight = INFINITE_;
    std::vector<int> output_link_candidates;
    int num_candidates = 0;

    // Identify the minimum weight among the candidate output links
    for (int link = 0; link < m_routing_table[vnet].size(); link++) {
        if (msg_destination.intersectionIsNotEmpty(
            m_routing_table[vnet][link])) {

        if (m_weight_table[link] <= min_weight)
            min_weight = m_weight_table[link];
        }
    }

    // Collect all candidate output links with this minimum weight
    for (int link = 0; link < m_routing_table[vnet].size(); link++) {
        if (msg_destination.intersectionIsNotEmpty(
            m_routing_table[vnet][link])) {

            if (m_weight_table[link] == min_weight) {
                num_candidates++;
                output_link_candidates.push_back(link);
            }
        }
    }

    if (output_link_candidates.size() == 0) {
        fatal("Fatal Error:: No Route exists from this Router.");
        exit(0);
    }

    // Randomly select any candidate output link
    int candidate = 0;
    if (!(m_router->get_net_ptr())->isVNetOrdered(vnet))
        candidate = rand() % num_candidates;

    output_link = output_link_candidates.at(candidate);
    return std::make_pair(output_link,-1);
}


void
RoutingUnit::addInDirection(PortDirection inport_dirn, int inport_idx)
{
    m_inports_dirn2idx[inport_dirn] = inport_idx;
    m_inports_idx2dirn[inport_idx]  = inport_dirn;
}

void
RoutingUnit::addOutDirection(PortDirection outport_dirn, int outport_idx)
{
    m_outports_dirn2idx[outport_dirn] = outport_idx;
    m_outports_idx2dirn[outport_idx]  = outport_dirn;
}

int
RoutingUnit::chooseAdaptiveOutport(
    const std::vector<PortDirection>& candidates)
{
    int best_outport = -1;
    int best_metric = -1;

    for (auto& dir : candidates) {
        auto it = m_outports_dirn2idx.find(dir);
        if (it == m_outports_dirn2idx.end()) continue;

        int outport_idx = it->second;
        OutputUnit* outunit = m_router->getOutputUnit(outport_idx);

        int metric = outunit->countFreeVCs();
        if (metric > best_metric) {
            best_metric = metric;
            best_outport = outport_idx;
        }
    }

    if (best_outport == -1) {
        panic("Adaptive routing: no valid outport\n");
    }

    return best_outport;
}

// outportCompute() is called by the InputUnit
// It calls the routing table by default.
// A template for adaptive topology-specific routing algorithm
// implementations using port directions rather than a static routing
// table is provided here.

std::pair<int,int>
RoutingUnit::outportCompute(RouteInfo route, int inport, int invc,
                            PortDirection inport_dirn)
{
    std::pair<int,int> outport_outvc = std::make_pair(-1,-1);

    if (route.dest_router == m_router->get_id()) {

        // Multiple NIs may be connected to this router,
        // all with output port direction = "Local"
        // Get exact outport id from table
        return lookupRoutingTable(route.vnet, route.net_dest);
    }

    // Routing Algorithm set in GarnetNetwork.py
    // Can be over-ridden from command line using --routing-algorithm = 1
    RoutingAlgorithm routing_algorithm =
        (RoutingAlgorithm) m_router->get_net_ptr()->getRoutingAlgorithm();

    switch (routing_algorithm) {
        case TABLE_:  outport_outvc =
            lookupRoutingTable(route.vnet, route.net_dest); break;
        case XY_:     outport_outvc =
            outportComputeXY(route, inport, inport_dirn); break;
        // any custom algorithm
        case CUSTOM_: outport_outvc =
            outportComputeCustom(route, inport, inport_dirn); break;
        case RING_:   outport_outvc =
            outportComputeRing(route, inport, invc, inport_dirn); break;
        case _3DDOR_: outport_outvc =
            outportCompute3DDOR(route, inport, inport_dirn); break;
        case _3DOPAR_: outport_outvc =
            outportCompute3DOPAR(route, inport, inport_dirn); break;
        case _3DPAR_: outport_outvc =
            outportCompute3DPAR(route, inport, inport_dirn); break;
        case _3DNEW_: outport_outvc =
            outportCompute3Dnew(route, inport, inport_dirn); break;
        default: outport_outvc =
            lookupRoutingTable(route.vnet, route.net_dest); break;
    }

    int outport = outport_outvc.first;
    int outvc = outport_outvc.second;
    
    assert(outport != -1 && outvc != -1);
    return outport_outvc;
}

// XY routing implemented using port directions
// Only for reference purpose in a Mesh
// By default Garnet uses the routing table
std::pair<int,int>
RoutingUnit::outportComputeXY(RouteInfo route,
                              int inport,
                              PortDirection inport_dirn)
{
    PortDirection outport_dirn = "Unknown";

    [[maybe_unused]] int num_rows = m_router->get_net_ptr()->getNumRows();
    int num_cols = m_router->get_net_ptr()->getNumCols();
    assert(num_rows > 0 && num_cols > 0);

    int my_id = m_router->get_id();
    int my_x = my_id % num_cols;
    int my_y = my_id / num_cols;

    int dest_id = route.dest_router;
    int dest_x = dest_id % num_cols;
    int dest_y = dest_id / num_cols;

    int x_hops = abs(dest_x - my_x);
    int y_hops = abs(dest_y - my_y);

    bool x_dirn = (dest_x >= my_x);
    bool y_dirn = (dest_y >= my_y);

    // already checked that in outportCompute() function
    assert(!(x_hops == 0 && y_hops == 0));

    if (x_hops > 0) {
        if (x_dirn) {
            assert(inport_dirn == "Local" || inport_dirn == "West");
            outport_dirn = "East";
        } else {
            assert(inport_dirn == "Local" || inport_dirn == "East");
            outport_dirn = "West";
        }
    } else if (y_hops > 0) {
        if (y_dirn) {
            // "Local" or "South" or "West" or "East"
            assert(inport_dirn != "North");
            outport_dirn = "North";
        } else {
            // "Local" or "North" or "West" or "East"
            assert(inport_dirn != "South");
            outport_dirn = "South";
        }
    } else {
        // x_hops == 0 and y_hops == 0
        // this is not possible
        // already checked that in outportCompute() function
        panic("x_hops == y_hops == 0");
    }

    return std::make_pair(m_outports_dirn2idx[outport_dirn], -1);
}

// Template for implementing custom routing algorithm
// using port directions. (Example adaptive)
std::pair<int,int>
RoutingUnit::outportComputeCustom(RouteInfo route,
                                 int inport,
                                 PortDirection inport_dirn)
{
    panic("%s placeholder executed", __FUNCTION__);
}

std::pair<int,int>
RoutingUnit::outportComputeRing(RouteInfo route,
                               int inport, int invc,
                               PortDirection inport_dirn)
{
    PortDirection outport_dirn = "Unknown";

    int my_id = m_router->get_id();
    int dest_id = route.dest_router;
    int num_routers = m_router->get_net_ptr()->getNumRouters();
    
    // Clockwise distance
    int dist_cw = (dest_id - my_id + num_routers) % num_routers;  
    // Counter-clockwise distance
    int dist_ccw = (my_id - dest_id + num_routers) % num_routers;

    outport_dirn = (dist_cw <= dist_ccw) ? "Clockwise" : "Counterclockwise";

    int outvc = invc; // keep the same vc
    assert(outvc != -1);
    if (outport_dirn == "Clockwise" 
        && my_id == num_routers - 1) {
        outvc = 1; 
    }
    if (outport_dirn == "Counterclockwise" 
        && my_id == 0) {
        outvc = 1; 
    }
    return std::make_pair(m_outports_dirn2idx[outport_dirn], outvc);
}

std::pair<int,int>
RoutingUnit::outportCompute3DDOR(RouteInfo route,
                                 int inport,
                                 PortDirection inport_dirn)
{
    PortDirection outport_dirn = "Unknown";

    int num_cols  = m_router->get_net_ptr()->getNumCols();
    int num_rows  = m_router->get_net_ptr()->getNumRows();
    int num_depth = m_router->get_net_ptr()->getNumDepths();

    int my_id = m_router->get_id();
    int my_x = my_id % num_cols;
    int my_y = (my_id / num_cols) % num_rows;
    int my_z = my_id / (num_cols * num_rows);

    int dest_id = route.dest_router;
    int dest_x = dest_id % num_cols;
    int dest_y = (dest_id / num_cols) % num_rows;
    int dest_z = dest_id / (num_cols * num_rows);

    int x_hops = dest_x - my_x;
    int y_hops = dest_y - my_y;
    int z_hops = dest_z - my_z;

    // already checked in outportCompute()
    assert(!(x_hops == 0 && y_hops == 0 && z_hops == 0));

    if (x_hops != 0) {
        outport_dirn = (x_hops > 0) ? "East" : "West";
    } else if (y_hops != 0) {
        outport_dirn = (y_hops > 0) ? "North" : "South";
    } else if (z_hops != 0) {
        outport_dirn = (z_hops > 0) ? "Up" : "Down";
    }

    return std::make_pair(m_outports_dirn2idx[outport_dirn],-1);
}

std::pair<int,int>
RoutingUnit::outportCompute3DOPAR(RouteInfo route,
                                  int inport,
                                  PortDirection inport_dirn)
{
    int num_cols  = m_router->get_net_ptr()->getNumCols();
    int num_rows  = m_router->get_net_ptr()->getNumRows();

    int my_id = m_router->get_id();
    int my_x = my_id % num_cols;
    int my_y = (my_id / num_cols) % num_rows;
    int my_z = my_id / (num_cols * num_rows);

    int dest_id = route.dest_router;
    int dest_x = dest_id % num_cols;
    int dest_y = (dest_id / num_cols) % num_rows;
    int dest_z = dest_id / (num_cols * num_rows);

    int dx = dest_x - my_x;
    int dy = dest_y - my_y;
    int dz = dest_z - my_z;

    assert(!(dx == 0 && dy == 0 && dz == 0));

    // ---- Z 优先 ----
    if (dz != 0) {
        return std::make_pair(m_outports_dirn2idx[(dz > 0) ? "Up" : "Down"], -1);
    }

    // ---- XY 平面 (Odd-Even) ----
    std::vector<PortDirection> candidates;

    if (dx > 0) { // East
        if (!((my_x % 2 == 1) && (dy > 0))) {
            candidates.push_back("East");
        }
        if (dy != 0) candidates.push_back((dy > 0) ? "North" : "South");
    } else if (dx < 0) { // West
        if (!((my_x % 2 == 0) && (dy < 0))) {
            candidates.push_back("West");
        }
        if (dy != 0) candidates.push_back((dy > 0) ? "North" : "South");
    } else { // dx == 0
        candidates.push_back((dy > 0) ? "North" : "South");
    }

    return std::make_pair(chooseAdaptiveOutport(candidates),-1);
}

std::pair<int,int>
RoutingUnit::outportCompute3DPAR(RouteInfo route,
                                 int inport,
                                 PortDirection inport_dirn)
{
    int num_cols  = m_router->get_net_ptr()->getNumCols();
    int num_rows  = m_router->get_net_ptr()->getNumRows();

    int my_id = m_router->get_id();
    int my_x = my_id % num_cols;
    int my_y = (my_id / num_cols) % num_rows;
    int my_z = my_id / (num_cols * num_rows);

    int dest_id = route.dest_router;
    int dest_x = dest_id % num_cols;
    int dest_y = (dest_id / num_cols) % num_rows;
    int dest_z = dest_id / (num_cols * num_rows);

    int dx = dest_x - my_x;
    int dy = dest_y - my_y;
    int dz = dest_z - my_z;

    assert(!(dx == 0 && dy == 0 && dz == 0));

    // ---- Z 优先 ----
    if (dz != 0) {
        return std::make_pair(m_outports_dirn2idx[(dz > 0) ? "Up" : "Down"], -1);
    }

    // ---- XY 平面 ----
    if (dx != 0 && dy != 0) {
        std::vector<PortDirection> candidates;
        candidates.push_back((dx > 0) ? "East" : "West");
        candidates.push_back((dy > 0) ? "North" : "South");
        return std::make_pair(chooseAdaptiveOutport(candidates), -1);
    } else if (dx != 0) {
        return std::make_pair(m_outports_dirn2idx[(dx > 0) ? "East" : "West"], -1);
    } else {
        return std::make_pair(m_outports_dirn2idx[(dy > 0) ? "North" : "South"], -1);
    }
}

std::pair<int,int>
RoutingUnit::outportCompute3Dnew(RouteInfo route,
                                  int inport,
                                  PortDirection inport_dirn)
{
    PortDirection outport_dirn = "Unknown";

    int num_cols  = m_router->get_net_ptr()->getNumCols();
    int num_rows  = m_router->get_net_ptr()->getNumRows();
    int num_depth = m_router->get_net_ptr()->getNumDepths();

    int my_id = m_router->get_id();
    int my_x = my_id % num_cols;
    int my_y = (my_id / num_cols) % num_rows;
    int my_z = my_id / (num_cols * num_rows);

    int dest_id = route.dest_router;
    int dest_x = dest_id % num_cols;
    int dest_y = (dest_id / num_cols) % num_rows;
    int dest_z = dest_id / (num_cols * num_rows);

    int dx = dest_x - my_x;
    int dy = dest_y - my_y;
    int dz = dest_z - my_z;

    assert(!(dx == 0 && dy == 0 && dz == 0));

    return std::make_pair(-1,-1);
}

} // namespace garnet
} // namespace ruby
} // namespace gem5
