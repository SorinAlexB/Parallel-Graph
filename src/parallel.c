// SPDX-License-Identifier: BSD-3-Clause

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <time.h>

#include "os_graph.h"
#include "os_threadpool.h"
#include "log/log.h"
#include "utils.h"

#define NUM_THREADS		4

static int sum;
static os_graph_t *graph;
static os_threadpool_t *tp;
pthread_mutex_t tmutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t g_mutex = PTHREAD_MUTEX_INITIALIZER;
/* TODO: Define graph task argument. */

void add_to_sum(int info)
{
	pthread_mutex_lock(&tmutex);
	sum += info;
	pthread_mutex_unlock(&tmutex);
}

static void process_node(unsigned int idx)
{
	/* TODO: Implement thread-pool based processing of graph. */
	os_node_t *node;

	pthread_mutex_lock(&g_mutex);
	node = graph->nodes[idx];
	pthread_mutex_unlock(&g_mutex);
	add_to_sum(node->info);
	pthread_mutex_lock(&g_mutex);
	graph->visited[idx] = DONE;

	for (unsigned int i = 0; i < node->num_neighbours; i++) {
		if (graph->visited[node->neighbours[i]] == NOT_VISITED) {
			graph->visited[node->neighbours[i]] = PROCESSING;
			os_task_t *g_task = create_task((void *)process_node, (void *)(long)node->neighbours[i], NULL);

			enqueue_task(tp, g_task);
		}
	}

	pthread_mutex_unlock(&g_mutex);
}

int main(int argc, char *argv[])
{
	FILE *input_file;

	if (argc != 2) {
		fprintf(stderr, "Usage: %s input_file\n", argv[0]);
		exit(EXIT_FAILURE);
	}

	input_file = fopen(argv[1], "r");
	DIE(input_file == NULL, "fopen");

	graph = create_graph_from_file(input_file);

	/* TODO: Initialize graph synchronization mechanisms. */
	tp = create_threadpool(NUM_THREADS);
	process_node(0);

	pthread_mutex_lock(&tp->xmutex);
	tp->x = 1;
	pthread_mutex_unlock(&tp->xmutex);
	wait_for_completion(tp);
	destroy_threadpool(tp);
	pthread_mutex_destroy(&tmutex);
	pthread_mutex_destroy(&g_mutex);
	printf("%d", sum);

	return 0;
}
