#include "metrics/metrics.h"

unsigned long watermark_num_edges(GRAPH* graph) {

	unsigned long num_edges = 0;

	// visit each node and see the connections
	for(; graph; graph = graph->next)

		// get size of connections list
		for(CONNECTION* connection = graph->connections; connection; connection = connection->next)
			num_edges++;

	return num_edges;
}

unsigned long watermark_num_hamiltonian_edges2014(GRAPH* graph) {

	// in a 2014 graph there are only backedges and hamiltonian edges
	// hamiltonian edge is removed, so we only need to get the number
	// of bits. n_bits = n_nodes - 1, because of the last null node, 
	// and since the number of edges is the number of nodes - 1, this
	// just give us the right result immediately.

	return get_num_bits(graph);
}

unsigned long watermark_num_hamiltonian_edges(GRAPH* graph) {

	unsigned long num_edges = 0;

	// nullify data in every node
	get_num_bits(graph);

	for(; graph; graph = graph->next) {

		if( get_hamiltonian_connection(graph) ) num_edges++;
	}

	return num_edges;
}

unsigned long watermark_num_nodes(GRAPH* graph) {

	unsigned long num_nodes = 0;
	for(; graph; graph = graph->next) {
		num_nodes++;
	}

	return num_nodes;
}

unsigned long watermark_cyclomatic_complexity(GRAPH* graph) {

	// there is only one exit always
	// cyclomatic complexity = # edges - # nodes + 2 * # nodes with exit points
	// cyclomatic complexity = # edges - # nodes + 2

	return watermark_num_edges(graph) - watermark_num_nodes(graph) + 2;
}

unsigned long watermark_num_cycles(GRAPH* graph) {

    // only backedges form cycles
    // so #cycles == #backedges

    unsigned long num_cycles = 0;
    for(GRAPH* node = graph; node; node = node->next) {

        // if a connection is not a forward edge or hamiltonian edge
        // then it must be a backedge
        for(CONNECTION* conn = node->connections; conn; conn = conn->next) {

            if( conn->node != conn->parent->next && ( !conn->parent->next || ( conn->parent->next && conn->parent->next->next != conn->node ) ) ) {
                num_cycles++;
                break; // only one backedge per node
            }
        }
    }
    return num_cycles;
}
