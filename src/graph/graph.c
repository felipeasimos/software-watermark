#include "graph/graph.h"

GRAPH* graph_empty(){

	//allocate memory for struct
	GRAPH* graph = malloc(sizeof(GRAPH));

	//set all values inside struct to zero
	memset(graph, 0x00, sizeof(GRAPH));

	return graph;
}

void graph_alloc(GRAPH* graph, unsigned long data_len){

	//if no graph was given, nothing happens
	if( !graph ) return;

	//deallocate any previous data stored
	if( graph->data ) free( graph->data );

	//update data_len attribute to the one given
	graph->data_len = data_len;

	//allocate memory according to new data_len
	graph->data = malloc( graph->data_len );
}

GRAPH* graph_create(void* data, unsigned long data_len) {

	GRAPH* graph = graph_empty();

	graph_alloc(graph, data_len);

	memcpy(graph->data, data, data_len);

	return graph;
}

void graph_free_node(GRAPH* graph){

	//if no node was given nothing happens
	if( !graph ) return;

	//deallocate data
	if( graph->data ) free(graph->data);
	//deallocate connections linked list
	if( graph->connections ) connection_free( graph->connections );

	//deallocate struct itself
	free( graph );
}

void graph_insert(GRAPH* graph_prev, GRAPH* graph_next){

	//if these represent the same graph nothing happens
	if( graph_prev == graph_next || !graph_prev || !graph_next ) return;

	//if both arguments are NULL, nothing happens

	//put graph_next in right place by its perspective
	if( graph_next ){
		graph_next->next = graph_prev ? graph_prev->next : graph_next->next;
		graph_next->prev = graph_prev;
	}

	//put graph_next in right place by the perspective of the rest of the list
	if( graph_prev ){
		if( graph_prev->next ) graph_prev->next->prev = graph_next;
		graph_prev->next = graph_next;
	}
}

GRAPH* graph_search(GRAPH* graph, void* data, unsigned long data_len){

	//if any of the arguments is NULL (or zero), nothing happens and NULL is returned
	if( !graph || !data || !data_len ) return NULL;

	//iterate through graph node list
	for(; graph; graph = graph->next)
		//if graph data is equal to given data, return it
		if( graph->data && graph->data_len == data_len && !memcmp(graph->data, data, data_len) )
			return graph;

	//if function reached here, no graph node was found so NULL is return
	return NULL;
}

CONNECTION* graph_connection(GRAPH* graph_from, GRAPH* graph_to){

	return connection_search(graph_from->connections, graph_to);
}

void graph_oriented_disconnect(GRAPH* graph_from, GRAPH* graph_to){

	//if one of the given pointers is NULL, nothing happens
	if( !graph_from || !graph_to ) return;

	//delete graph_to from graph_from's connection's list
	connection_delete( graph_from->connections, graph_to );

	/*
	connection_delete won't delete connection
	if it is the only one left
	(to avoid turning graph->connections into a dangling pointer)
	so we need to check if it is the only connection and if it is
	delete it manually
	*/
	if( graph_from->connections->node == graph_to ){
		CONNECTION* new_root = graph_from->connections->next;
		free( graph_from->connections );
		graph_from->connections = new_root;
	}	
}

void graph_oriented_connect(GRAPH* graph_from, GRAPH* graph_to){

	//if one of the given pointers is NULL, nothing happens
	if( !graph_from || !graph_to ) return;

	//insert graph_to in graph_from's connections list
	connection_insert( graph_from->connections, graph_to );

	/*
	connection_insert() will do nothing if graph->connections is NULL,
	so we need to check for this case and manually input the
	new connection as the first node in the list
	*/
	if( !graph_from->connections ){
		graph_from->connections = connection_create();
		graph_from->connections->node = graph_to;
	}
}

void graph_disconnect(GRAPH* graph1, GRAPH* graph2){

	//close connection between graph1 to graph2 if it exists
	graph_oriented_disconnect(graph1, graph2);

	//close connection between graph2 to graph1 if it exists
	graph_oriented_disconnect(graph2, graph1);
}

void graph_connect(GRAPH* graph1, GRAPH* graph2){

	//create connection from graph1 to graph2
	graph_oriented_connect(graph1, graph2);

	//create connection from graph2 to graph1
	graph_oriented_connect(graph2, graph1);
}

void graph_isolate(GRAPH* graph_to_isolate){

	//if one of the arguments is NULL nothing happens
	if( !graph_to_isolate ) return;

	/*
	we want to delete graph_to_isolate connections to its neighbours
	so we need to look in graph_to_isolate's connections list and
	not only delete each node from the list
	but also go to the neighbor's list of connection's and
	delete the struct pointing to graph_to_isolate there
	*/

	//since we want to do it until no connection is left,
	//the graph_to_isolate->connection should be NULL at the end
	while( graph_to_isolate->connections ) //we keep on going until this is NULL (no connection left)

		//then we close our connection with it
		graph_disconnect(graph_to_isolate, graph_to_isolate->connections->node);
}

GRAPH* graph_delete(GRAPH* graph_to_delete){

	//if graph_to_delete is NULL, nothing happens and returns NULL
	if( !graph_to_delete ) return NULL;

	//cut all connections between graph_to_delete and the rest of the graph
	graph_isolate( graph_to_delete );

	//take graph_to_delete out of the list
	graph_insert( graph_to_delete->prev, graph_to_delete->next );//now no node points to graph_to_delete

	//save pointer to valid node to return
	GRAPH* to_return = graph_to_delete->prev ? graph_to_delete->prev : graph_to_delete->next;

	graph_free_node( graph_to_delete ); //bye bye

	return to_return;
}

void graph_free(GRAPH* graph){

	//nothing given, nothing happens ¯\_(ツ)_/¯ (i <3 this emoji)
	if( !graph ) return;

	GRAPH* tmp;

	//iterate through list, freeing all nodes
	for(tmp = graph->next; graph; graph = tmp) {
		tmp = graph->next;
		graph_free_node(graph);
	}
}

void graph_default_print_func(void* data, unsigned long data_len){

	if( !data ) {
		printf("\x1b[33m null \x1b[0m");
	} else if ( data_len == sizeof(unsigned long) ) {
		printf("\x1b[33m %lu \x1b[0m", *(unsigned long*)data);
	} else if ( data_len == sizeof(unsigned int) ) {
		printf("\x1b[33m %u \x1b[0m", *(unsigned int*)data);
	} else if ( data_len == sizeof(unsigned short) ) {
		printf("\x1b[33m %hu \x1b[0m", *(unsigned short*)data);
	} else if (data_len == sizeof(char)) {
		printf("\x1b[33m %hhu \x1b[0m", *(char*)data);
	} else {
		printf("\x1b[33m %p \x1b[0m", data);
	}
}

void graph_print_node(GRAPH* graph, void (*print_func)(void*, unsigned long) ){

	//if no graph node was given nothing happens
	if( !graph ) return;

	//calls default print function if none was given
	(print_func ? print_func : graph_default_print_func)( graph->data, graph->data_len );
}

void graph_print(GRAPH* graph, void(*print_func)(void*, unsigned long)){

	//iterate through graph nodes
	for(; graph; graph = graph->next){

		//print graph_node
		graph_print_node(graph, print_func);
		if( graph->connections ){
			printf("\x1b[33m\x1b[1m -> \x1b[0m");
			//print each of its connections
			connection_print(graph->connections, print_func);
		}
		printf("\n");
	}
}

short graph_check_isolated(GRAPH* graph){
	return !graph || !graph->connections;
}

unsigned long graph_order_to(GRAPH* graph_root, GRAPH* graph_node){

	if( !graph_root || !graph_node ) return 0;
	
	unsigned order_to=0;
	
	for(; graph_root; graph_root = graph_root->next)
		if( graph_connection( graph_root, graph_node ) )
			order_to++;
	
	return order_to;
}

unsigned long graph_order_from(GRAPH* node){

	if( !node ) return 0;
	
	unsigned order_from=0;

    for(CONNECTION* conn = node->connections; conn; conn = conn->next) order_from++;

	return order_from;
}

unsigned long graph_order(GRAPH* graph_root, GRAPH* graph_node){
	
	if( !graph_root || !graph_node ) return 0;
	
	unsigned order=0;
	
	for(; graph_root; graph_root = graph_root->next)
		if( graph_connection( graph_node, graph_root ) || graph_connection( graph_root, graph_node ) )
			order++;
	
	return order;
}

unsigned long graph_num_nodes(GRAPH* graph) {

    unsigned long i = 0;
    for(; graph; graph = graph->next)i++;
    return i;
}

unsigned long graph_num_connections(GRAPH* graph) {

    unsigned long n=0;
    for(; graph; graph = graph->next) {

        n += connection_num(graph->connections);
    }
    return n;
}

typedef struct INFO_NODE {

    // value of data, stored previously in the node
    void* data;
    unsigned long data_len;

    // info used in a process
    void* info;
    unsigned long info_len;
} INFO_NODE;

void _graph_load_info(GRAPH* graph, void* info, unsigned long info_len) {

    INFO_NODE info_node = {
        .data = graph->data,
        .data_len = graph->data_len,
        .info = info,
        .info_len = info_len
    };

    graph->data_len = sizeof(INFO_NODE);
    graph->data = malloc( graph->data_len );
    memcpy(graph->data, &info_node, sizeof(INFO_NODE));
}

void _graph_unload_info(GRAPH* graph) {

    INFO_NODE* info_node = (INFO_NODE*)graph->data;

    graph->data_len = info_node->data_len;
    graph->data = info_node->data;
    free(info_node);
}

void* _graph_get_info(GRAPH* graph) {

    return ((INFO_NODE*)graph->data)->info;
}

unsigned long _graph_get_info_len(GRAPH* graph) {

    return ((INFO_NODE*)graph->data)->info_len;
}

//return deep copy of the graph
GRAPH* graph_copy(GRAPH* graph) {

    if(!graph) return NULL;

    // load nodes into array and create nodes for them, copying the data
    for(GRAPH* cursor = graph; cursor; cursor = cursor->next) {

        // load copy of this node
        _graph_load_info(
                cursor,
                graph_create(
                    cursor->data,
                    cursor->data_len
                ), // create copy
                sizeof(GRAPH)
            );
        // link the copied nodes
        if( cursor->prev ) {

            graph_insert(_graph_get_info(cursor->prev), _graph_get_info(cursor));
        }
    }
    // iterate over nodes again, this time making connections to correspondent nodes
    for(GRAPH* cursor = graph; cursor; cursor = cursor->next) {

        // look for copy counter-part
        GRAPH* copy = _graph_get_info(cursor);
        // iterate over the connections of this node
        for(CONNECTION* conn = cursor->connections; conn; conn = conn->next) {
            GRAPH* neighbour = _graph_get_info(conn->node);
            if(!neighbour) {
                #ifdef DEBUG
                    fprintf(stderr, "A node is connected to a node that doesn't belong to its graph!\n");
                #endif
                return NULL;
            }
            graph_oriented_connect(copy, neighbour);
        }
    }

    // get first node from copy
    GRAPH* copy = _graph_get_info(graph);

    // unload the graph
    for(GRAPH* cursor=graph; cursor; cursor = cursor->next) _graph_unload_info(cursor);
    return copy;
}

void* graph_serialize(GRAPH* graph, unsigned long* num_bytes) {

    if(!graph) return NULL;

    // load indices to graph (remember to free the indices later!)
    unsigned long i = 0;
    for(GRAPH* node = graph; node; i++) {

        unsigned long* i_copy = malloc(sizeof(i));
        memcpy(i_copy, &i, sizeof(i));
        _graph_load_info(node, i_copy, sizeof(i));
        node = node->next;
    }

    uint8_t* graph_serialized = malloc( sizeof(unsigned long) );
    *num_bytes = sizeof(unsigned long);

    // write number of nodes
    memcpy(graph_serialized, &i, sizeof(unsigned long));

    for(GRAPH* node = graph; node; node = node->next) {

        // 1. data_len (unsigned long, 8 bytes)
        // 2. data (variable length)
        // 3. number of neighbours (unsigned long)
        // 4. idx of neighbours (variable length)

        unsigned long num_neighbours = graph_order_from(node);

        unsigned long offset = 
            sizeof(unsigned long) + // data_len
            ((INFO_NODE*)node->data)->data_len + // data
            sizeof(unsigned long) + // num_neighbours
            sizeof(unsigned long) * num_neighbours; // list of neighbours idxs

        *num_bytes += offset;
        graph_serialized = realloc(graph_serialized, *num_bytes);

        // 1. data_len
        memcpy(
            graph_serialized + ( (*num_bytes) - offset ),
            &((INFO_NODE*)node->data)->data_len,
            sizeof(unsigned long)
        );

        // 2. data
        if( ((INFO_NODE*)node->data)->data_len ) {
            memcpy(
                graph_serialized + ( (*num_bytes) - offset + sizeof(unsigned long) ),
                    ((INFO_NODE*)node->data)->data,
                ((INFO_NODE*)node->data)->data_len
            );
        }

        // 3. number of neighbours
        memcpy(
            graph_serialized + ( (*num_bytes) - sizeof(unsigned long) * (1+num_neighbours) ),
            &num_neighbours,
            sizeof(unsigned long)
        );

        // 4. list of neighbours idxs
        unsigned long neighbour_pos = 0;
        for(CONNECTION* conn = node->connections; conn; conn = conn->next) {
            memcpy(
                graph_serialized + ( (*num_bytes) - sizeof(unsigned long) * (num_neighbours-neighbour_pos) ),
                _graph_get_info(conn->node),
                sizeof(unsigned long)
            );
            neighbour_pos++;
        }
    }

    // unload all nodes
    for(GRAPH* node=graph; node; node = node->next) {

        void* info = _graph_get_info(node);
        free(info);
        _graph_unload_info(node);
    }

    return graph_serialized;
}

GRAPH* graph_deserialize(uint8_t* data) {

    if(!data) return NULL;

    // get number of nodes
    unsigned long n;
    memcpy( &n, data, sizeof(unsigned long) );
    data += sizeof(unsigned long);

    // populate the array with empty nodes
    GRAPH* nodes[n];
    for(unsigned long i = 0; i < n; i++) {
        nodes[i] = graph_empty();
        if(i > 0) {
            graph_insert(nodes[i-1], nodes[i]);
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
            graph_alloc(nodes[i], nodes[i]->data_len);

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
            graph_oriented_connect(
                    nodes[i],
                    nodes[neighbour_idx]
            );
        }
    }
    return nodes[0];
}
