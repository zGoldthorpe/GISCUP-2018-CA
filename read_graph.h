/* GRAPH CONSTRUCTOR
 * author: Zach Goldthorpe
 *
 * The file provides the method for either quickly or robustly taking the input
 * and constructing the graph, performing the two reductions specified in the
 * main file.
 */

#ifndef read_graph_h
#define read_graph_h

#include <cstdio>
#include <fstream>
#include <string>
#include <vector>
#include <cstdint>
#include <unordered_map>

#ifdef ROBUST
    #include "json.h"
#else
    #include "json_fast.h"
#endif

// ==== reduction 2 defines ==== //
#define HEAD 0 // underlying main starting point
#define TAIL 1 // underlying main controller
#define REAL(v) (v > TAIL) // marks if the vertex is part of the original graph

using uintf = uint_fast32_t;
using uintp = std::pair<uintf, uintf>;

extern std::vector<std::vector<uintp> > graph;
extern std::vector<std::string> name, edgename;
extern std::vector<bool> badedge;
extern uintf nodes;

/* reads the JSON file provided by the @filename and populates @graph with the
 * graph it encodes. As specified by the GIS Cup, the JSON file should contain
 * the key "rows" which maps to a list of edge encodings, which are fields
 * namely storing the keys "viaGlobalId", "fromGlobalId", "toGlobalId"
 * specifying the edge name, and the vertices involved in the edge respectively.
 * The JSON file should additionally contain the key "controllers" which maps to
 * a list of fields namely storing the key "globalId" identifying the features
 * that are controllers. All other keys are ignored.
 *
 * after the construction of the graph, it reads the @startingpoints text file
 * for the keys and adjusts the graph according to the two reduction steps.
 */
void read_graph(const char *filename, const char *startingpoints);


// ==== DEFINITIONS ==== //

// edgenodes[e] stores a vector of all <u, v> pairs edge e represents
static std::vector<std::vector<uintp> > edgenodes;
// idx, edgeidx are the respective converses to name and edgename
static std::unordered_map<std::string, uintf> idx, edgeidx;
// edgevtx[e] maps each edge broken down (by reduction 1) to a vector of all
// the vertices it became (since these have non-unique ID's)
static std::unordered_map<uintf, std::vector<uintf> > edgevtx;

void read_graph(const char *filename, const char *startingpoints) {
    name.emplace_back(); // HEAD name is unimportant
    graph.emplace_back();
    name.emplace_back(); // TAIL name is unimportant
    graph.emplace_back();

    FILE *file = fopen(filename, "r");
    nodes = 2; // the two nodes are HEAD and TAIL

    uintf edges = 0, controllers = 0;

    #ifdef ROBUST
    // this is the key-order-independent implementation of the graph reader
    scan_field(file, { "\"rows\"", "\"controllers\"" }, [&, file](uintf i) {
        switch(i) {
            case 0: // rows
            scan_list(file, [&, file](void) {
                std::string edge, source, target;
                uintf edgev, sourcev, targetv; // corresponding index values
                std::unordered_map<std::string, uintf>::iterator it;

                if (scan_field(file, {
                    "\"viaGlobalId\"", "\"fromGlobalId\"", "\"toGlobalId\""
                }, [&, file](uintf i) {
                    switch(i) {
                        case 0: // viaGlobalId
                        extract_string(file, edge);
                        return;
                        case 1: // fromGlobalId
                        extract_string(file, source);
                        return;
                        case 2: // toGlobalId
                        extract_string(file, target);
                        return;
                        default:
                        return;
                    }
                })) {
                    if ((it = idx.find(source)) == idx.end()) {
                        // source vertex does not already exist
                        name.push_back(source);
                        graph.emplace_back();
                        idx[source] = sourcev = nodes++;
                    } else sourcev = it->second;
                    if ((it = idx.find(target)) == idx.end()) {
                        // target vertex does not already exist
                        name.push_back(target);
                        graph.emplace_back();
                        idx[target] = targetv = nodes++;
                    } else targetv = it->second;
                    if ((it = edgeidx.find(edge)) == edgeidx.end()) {
                        // edge identifier does not already exist
                        edgenodes.emplace_back();
                        edgename.push_back(edge);
                        badedge.push_back(false);
                        edgeidx[edge] = edgev = edges++;
                    } else edgev = it->second;

                    // append <u, v> std::pair to edge
                    edgenodes[edgev].emplace_back(sourcev, targetv);

                    // append edge to graph, bidirectionally
                    graph[sourcev].emplace_back(targetv, edgev);
                    graph[targetv].emplace_back(sourcev, edgev);
                }
            });
            // create an additional dummy edge for reduction 1
            edgename.emplace_back();
            badedge.push_back(false);
            return;
            case 1: // controllers
            scan_list(file, [&, file](void) {
                std::string ctrl;
                if (scan_field(file, { "\"globalId\"" }, [file, &ctrl](int) {
                    extract_string(file, ctrl);
                })) {
                    ++controllers;
                    std::unordered_map<std::string, uintf>::iterator it;
                    if ((it = idx.find(ctrl)) != idx.end()) {
                        // the controller is a vertex
                        // so apply reduction 2
                        graph[TAIL].emplace_back(it->second, edges);
                        graph[it->second].emplace_back(TAIL, edges);
                    }
                    if ((it = edgeidx.find(ctrl)) != edgeidx.end()) {
                        // the controller is an edge
                        if (!badedge[it->second]) {
                            // the edge has not been decomposed yet
                            badedge[it->second] = true; // delete the edge
                            std::unordered_map<uintf, std::vector<uintf> >::iterator jt = edgevtx.emplace(it->second, std::vector<uintf>()).first;
                            for (const uintp &p : edgenodes[it->second]) {
                                // first apply reduction 1
                                // add edge as vertex
                                name.push_back(ctrl);
                                graph.emplace_back();
                                uintf source = nodes++;
                                jt->second.push_back(source);
                                // connect new edge to TAIL (reduction 2)
                                graph[TAIL].emplace_back(source, edges);
                                graph[source].emplace_back(TAIL, edges);
                                // complete reduction 1 by reconnecting edge
                                // to its endpoints
                                graph[source].emplace_back(p.first, edges);
                                graph[p.first].emplace_back(source, edges);
                                graph[source].emplace_back(p.second, edges);
                                graph[p.second].emplace_back(source, edges);
                            }
                        } else {
                            // edge has already been decomposed
                            // so just apply reduction 2
                            for (const uintf &v : edgevtx[it->second]) {
                                graph[TAIL].emplace_back(v, edges);
                                graph[v].emplace_back(TAIL, edges);
                            }
                        }
                    }
                }
            });
            return;
            default:
            return;
        }
    });
    #else
    // this is the key-order-dependent algorithm, which behaves analogously with
    // the additional assumption that keys come in precisely the order specified
    // below
    begin_field(file);
    read_to_key(file, "\"rows\"");
    for (begin_list(file); begin_field(file); end_field(file)) {
        std::string edge, source, target;
        uintf edgev, sourcev, targetv;
        std::unordered_map<std::string, uintf>::iterator it;

        read_to_key(file, "\"viaGlobalId\"");
        extract_string(file, edge);
        read_to_key(file, "\"fromGlobalId\"");
        extract_string(file, source);
        read_to_key(file, "\"toGlobalId\"");
        extract_string(file, target);

        if ((it = idx.find(source)) == idx.end()) {
            name.push_back(source);
            graph.emplace_back();
            idx[source] = sourcev = nodes++;
        } else sourcev = it->second;
        if ((it = idx.find(target)) == idx.end()) {
            name.push_back(target);
            graph.emplace_back();
            idx[target] = targetv = nodes++;
        } else targetv = it->second;
        if ((it = edgeidx.find(edge)) == edgeidx.end()) {
            edgenodes.emplace_back();
            edgename.push_back(edge);
            badedge.push_back(false);
            edgeidx[edge] = edgev = edges++;
        } else edgev = it->second;

        edgenodes[edgev].emplace_back(sourcev, targetv);

        graph[sourcev].emplace_back(targetv, edgev);
        graph[targetv].emplace_back(sourcev, edgev);
    }

    edgename.emplace_back();
    badedge.push_back(false); // dummy edge for graph modification

    read_to_key(file, "\"controllers\"");
    std::string ctrl;
    for (begin_list(file); begin_field(file); end_field(file)) {
        read_to_key(file, "\"globalId\"");
        extract_string(file, ctrl);

        ++controllers;
        std::unordered_map<std::string, uintf>::iterator it;
        if ((it = idx.find(ctrl)) != idx.end()) {
            graph[TAIL].emplace_back(it->second, edges);
            graph[it->second].emplace_back(TAIL, edges);
        }
        if ((it = edgeidx.find(ctrl)) != edgeidx.end()) {
            if (!badedge[it->second]) {
                badedge[it->second] = true;
                std::unordered_map<uintf, std::vector<uintf> >::iterator jt = edgevtx.emplace(it->second, std::vector<uintf>()).first;
                for (auto &p : edgenodes[it->second]) {
                    name.push_back(ctrl);
                    graph.emplace_back();
                    uintf source = nodes++;
                    jt->second.push_back(source);
                    graph[TAIL].emplace_back(source, edges);
                    graph[source].emplace_back(TAIL, edges);

                    graph[source].emplace_back(p.first, edges);
                    graph[p.first].emplace_back(source, edges);
                    graph[source].emplace_back(p.second, edges);
                    graph[p.second].emplace_back(source, edges);
                }
            } else {
                for (const uintf &v : edgevtx[it->second]) {
                    graph[TAIL].emplace_back(v, edges);
                    graph[v].emplace_back(TAIL, edges);
                }
            }
        }
    }
    end_field(file);
    #endif

    fclose(file);

    // starting points, done in a similar fashion to controllers
    std::string in;
    std::ifstream fin(startingpoints);
    while (std::getline(fin, in)) {
        in.push_back('\n');
        std::unordered_map<std::string, uintf>::iterator it;
        if ((it = idx.find(in)) != idx.end()) {
            // starting point is a vertex
            // so apply reduction 2
            graph[HEAD].emplace_back(it->second, edges);
            graph[it->second].emplace_back(HEAD, edges);
        }
        if ((it = edgeidx.find(in)) != edgeidx.end()) {
            // starting point is an edge
            if (!badedge[it->second]) {
                badedge[it->second] = true;
                std::unordered_map<uintf, std::vector<uintf> >::iterator jt = edgevtx.emplace(it->second, std::vector<uintf>()).first;
                for (const uintp &p : edgenodes[it->second]) {
                    // first apply reduction 1
                    name.push_back(in);
                    graph.emplace_back();
                    uintf source = nodes++;
                    jt->second.push_back(source);
                    // add point to HEAD (reduction 2)
                    graph[HEAD].emplace_back(source, edges);
                    graph[source].emplace_back(HEAD, edges);
                    // complete reduction 1
                    graph[source].emplace_back(p.first, edges);
                    graph[p.first].emplace_back(source, edges);
                    graph[source].emplace_back(p.second, edges);
                    graph[p.second].emplace_back(source, edges);
                }
            } else {
                for (const uintf &v : edgevtx[it->second]) {
                    // apply reduction 2
                    graph[HEAD].emplace_back(v, edges);
                    graph[v].emplace_back(HEAD, edges);
                }
            }
        }
    }
    fin.close();
}

#endif
