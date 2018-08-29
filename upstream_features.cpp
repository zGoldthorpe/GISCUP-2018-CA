/* ACM SIGSPATIAL GIS Cup 2018 Upstream Feature Analyser
 * author: Zach Goldthorpe
 *
 * Provided with a spatial network with identified starting points and
 * controllers, the program will output the global ID's of all upstream features
 * (defined as nodes and edges lying on any simple path from some starting point
 * to some controller).
 *
 * The program performs two reductions to the graph before solving the problem:
 *
 * Reduction 1: all starting points and controllers are vertices
 * ------------
 * For each edge identified as a starting point or a controller, it is replaced
 * with a node adopting its global ID adjacent to the two nodes each instance of
 * the edge was originally connecting.
 *
 * Reduction 2: there is a single starting point HEAD and controller TAIL
 * ------------
 * Two new nodes HEAD and TAIL are introduced to the graph which are then
 * connected to each starting point and each controller respectively.
 *
 * The problem has now reduced to finding all edges and nodes that lie on a
 * simple path from HEAD to TAIL. This is achieved in two (theoretical) steps:
 *
 * Step 1: decompose the graph into biconnected components
 * -------
 * A biconnected component of a graph is a maximal biconnected subgraph, which
 * is a graph where the deletion of any vertex keeps the graph connected. As a
 * consequence, there exists a simple path going through any three vertices in
 * a biconnected graph (otherwise, we could remove a vertex from the non-simple
 * part and disconnect the graph).
 * Any graph decomposes into biconnected components with "portals" between the
 * components being the articulation points: any other vertex is non-articulate
 * and thus can be cut without disconnecting the graph.
 *
 * Step 2: find the upstream features in the block-cut tree
 * -------
 * Since the biconnected components are connected via articulation points, the
 * induced structure is a forest (cycles would contradict the definition of an
 * articulation point) and thus there is at most one path from the component
 * containing HEAD to the component containing TAIL. By the properties of
 * biconnected components, the upstream features will be precisely the nodes and
 * edges of the biconnected components along this path.
 *
 * This algorithm implements both steps simultaneously, even identifying all of
 * the upstream vertices in the first sweep. The second sweep is necessary for
 * retrieving all of the upstream edges as well.
 */

#include <cstdio>
#include <fstream>
#include <string>
#include <vector>
#include <stack>
#include <cstdint>
#include "read_graph.h"
using namespace std;

using uintf = uint_fast32_t;
using uintp = pair<uintf, uintf>;

// graph[v] stores a vector of <neighbour, edge_idx> pairs for the vertex v
vector<vector<uintp> > graph;
// name, edgename map indices to the original names provided by the JSON
vector<string> name, edgename;
// upstream[v] indicates that vertex v is an upstream feature
// badedge[e] indicates whether edge e has been deleted or not (reduction 1)
vector<bool> upstream, badedge;
// dfs[v] stores <dfs_count, dfs_low> pairs for the DFS that finds articulation
// points (step 1)
vector<uintp> dfs;
// tracks how many nodes are in the graph;
uintf nodes = 0;

/* traverse through the constructed graph in a non-recursive DFS to find the
 * biconnected components which lie along the path from HEAD to TAIL, recording
 * the info into "upstream"
 */
void traverse();

int main(int argc, char **argv) {
    if (argc != 4) {
        fprintf(stderr, "Usage: %s <data.json> <startingpoints.txt> <output.txt>\n", argv[0]);
        return -1;
    }
    // build the graph
    read_graph(argv[1], argv[2]);

    // recurse and find the upstream vertices
    upstream = vector<bool>(nodes, false);
    dfs = vector<uintp>(nodes, make_pair(0, 0));
    traverse();

    // use this information to find and print the upstream features
    ofstream fout(argv[3]);
    stack<uintf> find_upstream;
    find_upstream.push(0);
    dfs[0].first = 0;
    // dfs[v].first being 0 will indicate that it has already been recursed on
    while (!find_upstream.empty()) {
        uintf v = find_upstream.top();
        find_upstream.pop();
        fout << name[v];
        for (const uintp &p : graph[v]) {
            uintf u = p.first;
            if (!upstream[u])
                continue;
            if (u > v)
                fout << edgename[p.second];
            if (dfs[u].first) {
                dfs[u].first = 0;
                find_upstream.push(u);
                fout << name[u];
            }
        }
    }
    fout.close();

    return 0;
}

// ==== DEFINITIONS ==== //

void traverse() {
    // struct representing the recursion stack frame for the original DFS
    struct frame {
        // @v denotes the vertex of the DFS, @parent its parent
        // @i denotes the index of the edge it has last checked (originally -1)
        // @best stores the best value of dfs_count encountered [see @dfs]
        // @reached stores if @v can reach TAIL in the DFS
        // @started stores if any edge has been explored yet
        uintf v, i, best;
        bool reached, started;
        frame(uintf v, uintf i, uintf best, bool reached, bool started)
            : v(v), i(i), best(best), reached(reached), started(started) {}
    };
    // count tracks dfs_count for the entire recursion
    uintf count = 1;
    // acc accumulates vertices during the DFS until a biconnected component is
    // fully realised
    stack<uintf> acc;
    // stk serves as the substitute for the recursion stack
    stack<frame> stk;
    stk.emplace(0, 0, count++, false, false);
    // child_reached will store the return value of the recursive call
    bool child_reached = false;

    while (!stk.empty()) {
        frame fm = stk.top();
        stk.pop();
        fm.reached |= child_reached; // fetch return value
        if (fm.started) {
            // we have already seen at least one edge, so check for a
            // biconnected component
            uintf u = graph[fm.v][fm.i].first;
            if (dfs[u].second >= dfs[fm.v].first) {
                // we have found an articulation point
                while (acc.top() != u) {
                    // this biconnected component is upstream
                    upstream[acc.top()] = upstream[acc.top()]|child_reached;
                    acc.pop();
                }
                upstream[u] = upstream[u]|child_reached;
                acc.pop();
            } else fm.best = min(fm.best, dfs[u].second);

            ++fm.i; // examine next edge
        } else {
            // otherwise, we have seen this vertex for the first time
            dfs[fm.v] = make_pair(fm.best, fm.best);
            acc.push(fm.v);
        }
        while (fm.i < graph[fm.v].size()
                && (dfs[graph[fm.v][fm.i].first].first > 0
                    || badedge[graph[fm.v][fm.i].second])) {
            // skip the edges that have already  been recursed or have been
            // deleted from reduction 1
            if (!badedge[graph[fm.v][fm.i].second]) {
                // check if we can reach TAIL but otherwise fetch already
                // obtained information
                fm.reached |= graph[fm.v][fm.i].first == TAIL;
                fm.best = min(fm.best, dfs[graph[fm.v][fm.i].first].second);
            }
            ++fm.i;
        }
        if (fm.i == graph[fm.v].size()) {
            // we have exhausted the edges of this vertex
            dfs[fm.v].second = fm.best;
            child_reached = fm.reached;
            continue;
        }
        // recurse through edge fm.i
        fm.reached |= graph[fm.v][fm.i].first == TAIL;
        fm.started = 1;
        stk.push(fm);
        stk.emplace(graph[fm.v][fm.i].first, 0, count++, false, false);
        child_reached = false; // reset return value
    }
    // after recursion, pop off the remaining accumulated features, as these are
    // part of the biconnected component of HEAD
    while (!acc.empty()) {
        upstream[acc.top()] = true;
        acc.pop();
    }
}