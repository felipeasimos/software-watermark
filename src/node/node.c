#include "node/node.h"

NODE* node_empty(GRAPH* graph, unsigned long idx){

	//allocate memory for struct
	NODE* node = malloc(sizeof(NODE));

	//set all values inside struct to zero
	memset(node, 0x00, sizeof(NODE));

    node->graph = graph;
    node->graph_idx = idx;
	return node;
}

void node_alloc(NODE* node, unsigned long data_len){

	//if no node was given, nothing happens
	if( !node ) return;

	//deallocate any previous data stored
	if( node->data ) free( node->data );

    if(node->data_len != data_len) {
        //update data_len attribute to the one given
        node->data_len = data_len;

        //allocate memory according to new data_len
        node->data = malloc( node->data_len );
    }
}

NODE* node_set_data(NODE* node, void* data, unsigned long data_len) {

    node_alloc(node, data_len);
    if(data && data_len) {
        memcpy(node->data, data, data_len);
    }
	return node;
}

void* node_get_data(NODE* node) {

    INFO_NODE* info_node = node_get_info_struct(node);
    return info_node ? info_node->data : node->data;
}

unsigned long node_get_data_len(NODE* node) {

    INFO_NODE* info_node = node_get_info_struct(node);
    return info_node ? info_node->data_len : node->data_len;
}

uint8_t node_oriented_disconnect(NODE* node_from, NODE* node_to){

	//if one of the given pointers is NULL, nothing happens
	if( !node_from || !node_to ) return 0;

	//delete node_to from node_from's connection's list
    //and update counts if connection existed
    uint8_t deleted=0;
	if( connection_delete_out_neighbour( node_from->out, node_to ) ) {
        node_from->num_connections--;
        node_from->num_out_neighbours--;
        deleted=1;
    }
    if( connection_delete_in_neighbour( node_to->in, node_from ) ) {

        node_to->num_connections--;
        node_to->num_in_neighbours--;
        deleted=1;
    };
    return deleted;
}

void node_oriented_connect(NODE* node_from, NODE* node_to){

	//if one of the given pointers is NULL, nothing happens
	if( !node_from || !node_to ) return;

	//insert node_to in node_from's connections list
    if(node_from->out) {
        connection_insert_out_neighbour( node_from->out, node_to );
    } else {
        node_from->out = connection_create(node_from, node_from, node_to, OUT);
    }
    if(node_to->in) {
        connection_insert_in_neighbour( node_to->in, node_from );
    } else {
        node_to->in = connection_create(node_to, node_from, node_to, IN);
    }

    // update counts
    node_to->num_connections++;
    node_to->num_in_neighbours++;
    node_from->num_connections++;
    node_from->num_out_neighbours++;
}

CONNECTION* node_get_connection(NODE* node_from, NODE* node_to) {

    return connection_search_out_neighbour(node_from->out, node_to);
}

void node_disconnect(NODE* node1, NODE* node2){

	//close connection between node1 to node2 if it exists
	node_oriented_disconnect(node1, node2);

	//close connection between node2 to node1 if it exists
	node_oriented_disconnect(node2, node1);
}

void node_connect(NODE* node1, NODE* node2){

	//create connection from node1 to node2
	node_oriented_connect(node1, node2);

	//create connection from node2 to node1
	node_oriented_connect(node2, node1);
}

void node_free(NODE* node){

    if(!node) return;
    node_unload_all_info(node);
    graph_isolate(node);
    free(node->data);
    free(node);
}

void node_default_print_func(FILE* f, NODE* node){

    void* data = node_get_data(node);
    unsigned long data_len = node_get_data_len(node);
    fprintf(f, " \x1b[33m[\x1b[0m%lu\x1b[33m]\x1b[0m", node->graph_idx);
	if( !data ) {
		fprintf(f, "\x1b[33m(null)\x1b[0m");
	} else if ( data_len == sizeof(unsigned long) ) {
		fprintf(f, "\x1b[33m(%lu)\x1b[0m", *(unsigned long*)data);
	} else if ( data_len == sizeof(unsigned int) ) {
		fprintf(f, "\x1b[33m(%u)\x1b[0m", *(unsigned int*)data);
	} else if ( data_len == sizeof(unsigned short) ) {
		fprintf(f, "\x1b[33m(%hu)\x1b[0m", *(unsigned short*)data);
	} else if (data_len == sizeof(char)) {
		fprintf(f, "\x1b[33m(%hhu)\x1b[0m", *(char*)data);
	} else {
		fprintf(f, "\x1b[33m(%p)\x1b[0m", data);
	}
}

void node_write(NODE* node, FILE* file, void (*print_func)(FILE*, NODE*)) {

    if(!node) return;

	(print_func ? print_func : node_default_print_func)( file, node );
}

void node_print(NODE* node, void (*print_func)(FILE*, NODE*) ){

	//if no node node was given nothing happens
	if( !node ) return;

	//calls default print function if none was given
	(print_func ? print_func : node_default_print_func)( stdout, node );
}

void node_load_info(NODE* node, void* info, unsigned long info_len) {

    INFO_NODE info_node = {
        .data = node->data,
        .data_len = node->data_len,
        .info = info,
        .info_len = info_len
    };

    node->data_len = sizeof(INFO_NODE);
    node->data = malloc( node->data_len );
    memcpy(node->data, &info_node, node->data_len);
    node->num_info++;
}

void node_unload_info(NODE* node) {

    if(!node || !node->data) return;

    INFO_NODE* info_node = (INFO_NODE*)node->data;

    node->data_len = info_node->data_len;
    node->data = info_node->data;
    free(info_node);
    node->num_info--;
}

void node_unload_all_info(NODE* node) {

    if(!node->num_info) return;
    while(node->num_info) node_unload_info(node);
}

void node_free_info(NODE* node) {

    if(!node || !node->data) return;
    INFO_NODE* info_node = (INFO_NODE*)node->data;

    node->data_len = info_node->data_len;
    node->data = info_node->data;
    free(info_node->info);
    free(info_node);
    node->num_info--;
}

void node_free_all_info(NODE* node) {

    if(!node->num_info) return;
    while(node->num_info) node_free_info(node);
}

INFO_NODE* node_get_info_struct(NODE* node) {

    if(!node->num_info) return NULL;
    unsigned long num_info = node->num_info - 1;
    void* data = node->data;
    while(num_info--) data = ((INFO_NODE*)data)->data;
    return data;
}

void* node_get_info(NODE* node) {

    return node->num_info ? ((INFO_NODE*)node->data)->info : NULL;
}

unsigned long node_get_info_len(NODE* node) {

    return node->num_info ? ((INFO_NODE*)node->data)->info_len : ULONG_MAX;
}

void node_set_info(NODE* node, void* info, unsigned long info_len) {

    if(!node->num_info) {
        node_load_info(node, info, info_len);
    } else {
        INFO_NODE* info_node = node->data;
        info_node->info = info;
        info_node->info_len = info_len;
    }
}

void node_transfer_out_connections(NODE* from, NODE* to) {

    // copy all out connections
    for(CONNECTION* conn = from->out; conn; conn = conn->next) graph_oriented_connect(to, conn->to);
    // remove all out connections
    while(from->out) graph_oriented_disconnect(from, from->out->to);
}

void node_transfer_in_connections(NODE* from, NODE* to) {

    // copy all in connections
    for(CONNECTION* conn = from->in; conn; conn = conn->next) graph_oriented_connect(conn->to, to);
    // remove all out connections
    while(from->in) graph_oriented_disconnect(from->in->from, from);
}

NODE* node_expand_to_sequence(NODE* node) {

    GRAPH* graph = node->graph;
    graph_insert(graph, node->graph_idx+1);
    node_transfer_out_connections(node, graph->nodes[node->graph_idx+1]);
    graph_oriented_connect(node, graph->nodes[node->graph_idx+1]);
    return graph->nodes[node->graph_idx+1];
}
NODE* node_expand_to_repeat(NODE* node) {

    GRAPH* graph = node->graph;
    graph_insert(graph, node->graph_idx+1);
    graph_insert(graph, node->graph_idx+2);
    NODE* middle_node = graph->nodes[node->graph_idx+1];
    NODE* sink_node = graph->nodes[node->graph_idx+2];
    node_transfer_out_connections(node, sink_node);
    graph_oriented_connect(node, middle_node);
    graph_oriented_connect(middle_node, node);
    graph_oriented_connect(middle_node, sink_node);
    return sink_node;
}
NODE* node_expand_to_while(NODE* node) {

    GRAPH* graph = node->graph;
    graph_insert(graph, node->graph_idx+1);
    graph_insert(graph, node->graph_idx+2);
    NODE* connect_back_node = graph->nodes[node->graph_idx+1];
    NODE* sink_node = graph->nodes[node->graph_idx+2];
    node_transfer_out_connections(node, sink_node);
    graph_oriented_connect(node, connect_back_node);
    graph_oriented_connect(connect_back_node, node);
    graph_oriented_connect(node, sink_node);
    return sink_node;
}
NODE* node_expand_to_if_then(NODE* node) {

    GRAPH* graph = node->graph;
    graph_insert(graph, node->graph_idx+1);
    graph_insert(graph, node->graph_idx+2);
    NODE* middle_node = graph->nodes[node->graph_idx+1];
    NODE* sink_node = graph->nodes[node->graph_idx+2];
    node_transfer_out_connections(node, sink_node);
    graph_oriented_connect(node, middle_node);
    graph_oriented_connect(node, sink_node);
    graph_oriented_connect(middle_node, sink_node);
    return sink_node;
}
NODE* node_expand_to_if_then_else(NODE* node) {

    node_expand_to_p_case(node, 2);
    return node->graph->nodes[node->graph_idx+3];
}
NODE* node_expand_to_p_case(NODE* node, unsigned long p) {

    GRAPH* graph = node->graph;

    // insert new nodes in the graph
    p++;
    for(unsigned long i = 0; i < p; i++) graph_insert(graph, node->graph_idx+i+1);
    NODE* sink_node = graph->nodes[node->graph_idx+p];
    node_transfer_out_connections(node, sink_node);
    p--;
    // connect to all nodes, and connect all nodes to sink_node
    for(unsigned long i = 0; i < p; i++) {
        NODE* middle_node = graph->nodes[node->graph_idx+i+1];
        graph_oriented_connect(node, middle_node);
        graph_oriented_connect(middle_node, sink_node);
    }
    return sink_node;
}
