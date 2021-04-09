#include "utils/utils.h"

GRAPH* search_backedge(GRAPH* node) {
	for(CONNECTION* conn = node->connections; conn; conn = conn->next) {

		if( conn->node->data_len ) return conn->node;
	}
	return NULL;
}
