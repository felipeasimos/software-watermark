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

    unsigned long num_info = node->num_info;
    void* data = node->data;
    while(num_info--) data = ((INFO_NODE*)data)->data;
    return data;
}

unsigned long node_get_data_len(NODE* node) {

    if(!node->num_info) return node->data_len;
    unsigned long num_info = node->num_info - 1;
    void* data = node->data;
    while(num_info--) data = ((INFO_NODE*)data)->data;
    return ((INFO_NODE*)data)->data_len;
}
void node_oriented_disconnect(NODE* node_from, NODE* node_to){

	//if one of the given pointers is NULL, nothing happens
	if( !node_from || !node_to ) return;

	//delete node_to from node_from's connection's list
	connection_delete_out_neighbour( node_from->connections, node_to );
    connection_delete_in_neighbour( node_to->connections, node_from );

    // update counts
    node_from->num_connections--;
    node_from->num_out_neighbours--;
    node_to->num_connections--;
    node_to->num_in_neighbours--;
}

void node_oriented_connect(NODE* node_from, NODE* node_to){

	//if one of the given pointers is NULL, nothing happens
	if( !node_from || !node_to ) return;

	//insert node_to in node_from's connections list
    if(node_from->connections) {
        connection_insert_out_neighbour( node_from->connections, node_to );
    } else {
        node_from->connections = connection_create(node_from, node_from, node_to);
    }
    if(node_to->connections) {
        connection_insert_in_neighbour( node_to->connections, node_from );
    } else {
        node_to->connections = connection_create(node_to, node_from, node_to);
    }

    // update counts
    node_to->num_connections++;
    node_to->num_in_neighbours++;
    node_from->num_connections++;
    node_from->num_out_neighbours++;
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

void node_isolate(NODE* node_to_isolate){

	//if one of the arguments is NULL nothing happens
	if( !node_to_isolate ) return;

    // go through all nodes this one connects to
    CONNECTION* next_conn = node_to_isolate->connections;
    for(CONNECTION* conn = next_conn; conn; conn = next_conn) {
        next_conn = next_conn->next;
        // disconnect from every node
        node_disconnect(conn->from, conn->to);
    }
}

void node_free(NODE* node){

    if(!node) return;
    node_unload_all_info(node);
    free(node->data);
    connection_free(node->connections);
    free(node);
}

void node_default_print_func(NODE* node, void* data, unsigned long data_len){

    printf(" \x1b[33m[\x1b[0m%lu\x1b[33m]\x1b[0m", node->graph_idx);
	if( !data ) {
		printf("\x1b[33m(null) \x1b[0m");
	} else if ( data_len == sizeof(unsigned long) ) {
		printf("\x1b[33m(%lu) \x1b[0m", *(unsigned long*)data);
	} else if ( data_len == sizeof(unsigned int) ) {
		printf("\x1b[33m(%u) \x1b[0m", *(unsigned int*)data);
	} else if ( data_len == sizeof(unsigned short) ) {
		printf("\x1b[33m(%hu) \x1b[0m", *(unsigned short*)data);
	} else if (data_len == sizeof(char)) {
		printf("\x1b[33m(%hhu) \x1b[0m", *(char*)data);
	} else {
		printf("\x1b[33m(%p) \x1b[0m", data);
	}
}

void node_print(NODE* node, void (*print_func)(NODE*, void*, unsigned long) ){

	//if no node node was given nothing happens
	if( !node ) return;

	//calls default print function if none was given
	(print_func ? print_func : node_default_print_func)( node, node->data, node->data_len );
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

    unsigned long info = node->num_info;
    if(!info) return;
    while(info--) node_unload_info(node);
}

void* node_get_info(NODE* node) {

    if(!node->num_info) return NULL;
    unsigned long num_info = node->num_info - 1;
    void* data = node->data;
    while(num_info--) data = ((INFO_NODE*)data)->data;
    return ((INFO_NODE*)data)->info;
}

unsigned long node_get_info_len(NODE* node) {

    if(!node->num_info) return 0;
    unsigned long num_info = node->num_info - 1;
    void* data = node->data;
    while(num_info--) data = ((INFO_NODE*)data)->data;
    return ((INFO_NODE*)data)->info_len;
}

NODE* node_copy(NODE* node) {

    if(!node) return NULL;
    node_load_copy(node);
    // get first node from copy
    NODE* copy = node_get_info(node);
    node_unload_all_info(node);
    // unload the node
    return copy;
}

void node_load_copy(NODE* node) {

    if(!node) return;

    //get root
    for(; node->prev; node = node->prev);

    for(NODE* node = node; node; node = node->next) {

        // load copy of this node
        node_load_info(
                node,
                node_create(
                    node->data,
                    node->data_len
                ), // create copy
                sizeof(NODE)
            );
        // link the copied nodes
        if( node->prev ) {

            NODE* current_copy = node_get_info(node);
            NODE* prev_copy = node_get_info(node->prev);
            prev_copy->next = current_copy;
            current_copy->prev = prev_copy;
        }
    }
    // iterate over nodes again, this time making connections to correspondent nodes
    for(NODE* cursor = node; cursor; cursor = cursor->next) {

        // look for copy counter-part
        NODE* copy = node_get_info(cursor);
        // iterate over the connections of this node
        for(CONNECTION* conn = cursor->connections; conn; conn = conn->next) {
            NODE* neighbour = node_get_info(conn->node);
            node_oriented_connect(copy, neighbour);
        }
    }
}

void* node_serialize(NODE* node, unsigned long* num_bytes) {

    if(!node) return NULL;

    // load indices to node (remember to free the indices later!)
    unsigned long i = 0;
    for(NODE* node = node; node; i++) {

        unsigned long* i_copy = malloc(sizeof(i));
        memcpy(i_copy, &i, sizeof(i));
        node_load_info(node, i_copy, sizeof(i));
        node = node->next;
    }

    uint8_t* node_serialized = malloc( sizeof(unsigned long) );
    *num_bytes = sizeof(unsigned long);

    // write number of nodes
    memcpy(node_serialized, &i, sizeof(unsigned long));

    for(NODE* node = node; node; node = node->next) {

        // 1. data_len (unsigned long, 8 bytes)
        // 2. data (variable length)
        // 3. number of neighbours (unsigned long)
        // 4. idx of neighbours (variable length)

        unsigned long num_neighbours = node_order_from(node);

        unsigned long offset = 
            sizeof(unsigned long) + // data_len
            ((INFO_NODE*)node->data)->data_len + // data
            sizeof(unsigned long) + // num_neighbours
            sizeof(unsigned long) * num_neighbours; // list of neighbours idxs

        *num_bytes += offset;
        node_serialized = realloc(node_serialized, *num_bytes);

        // 1. data_len
        memcpy(
            node_serialized + ( (*num_bytes) - offset ),
            &((INFO_NODE*)node->data)->data_len,
            sizeof(unsigned long)
        );

        // 2. data
        if( ((INFO_NODE*)node->data)->data_len ) {
            memcpy(
                node_serialized + ( (*num_bytes) - offset + sizeof(unsigned long) ),
                    ((INFO_NODE*)node->data)->data,
                ((INFO_NODE*)node->data)->data_len
            );
        }

        // 3. number of neighbours
        memcpy(
            node_serialized + ( (*num_bytes) - sizeof(unsigned long) * (1+num_neighbours) ),
            &num_neighbours,
            sizeof(unsigned long)
        );

        // 4. list of neighbours idxs
        unsigned long neighbour_pos = 0;
        for(CONNECTION* conn = node->connections; conn; conn = conn->next) {
            memcpy(
                node_serialized + ( (*num_bytes) - sizeof(unsigned long) * (num_neighbours-neighbour_pos) ),
                node_get_info(conn->node),
                sizeof(unsigned long)
            );
            neighbour_pos++;
        }
    }

    // unload all nodes
    for(NODE* node=node; node; node = node->next) {

        void* info = node_get_info(node);
        free(info);
        node_unload_info(node);
    }

    return node_serialized;
}

NODE* node_deserialize(uint8_t* data) {

    if(!data) return NULL;

    // get number of nodes
    unsigned long n;
    memcpy( &n, data, sizeof(unsigned long) );
    data += sizeof(unsigned long);

    // populate the array with empty nodes
    NODE* nodes[n];
    for(unsigned long i = 0; i < n; i++) {
        nodes[i] = node_empty();
        if(i > 0) {
            node_insert(nodes[i-1], nodes[i]);
        }
    }

    for(unsigned long i = 0; i < n; i++) {

        // 1. data_len
        // 2. data
        // 3. num_neighbours
        // 4. list of neighbour idxs

        // 1. data_len
        memcpy(
            &nodes[i]->data_len,
            data,
            sizeof(unsigned long)
        );
        data += sizeof(unsigned long);
        if( nodes[i]->data_len ) {
            node_alloc(nodes[i], nodes[i]->data_len);

            // 2. data
            memcpy(
                nodes[i]->data,
                data,
                nodes[i]->data_len
            );
            data += nodes[i]->data_len;
        }

        // 3. num_neighbours
        unsigned long num_neighbours;
        memcpy(
            &num_neighbours,
            data,
            sizeof(unsigned long)
        );
        data += sizeof(unsigned long);

        // 4. list of neighbour idxs
        for(unsigned long j = 0; j < num_neighbours; j++) {

            unsigned long neighbour_idx;
            memcpy(
                &neighbour_idx,
                data,
                sizeof(unsigned long)
            );
            data += sizeof(unsigned long);
            node_oriented_connect(
                    nodes[i],
                    nodes[neighbour_idx]
            );
        }
    }
    return nodes[0];
}
