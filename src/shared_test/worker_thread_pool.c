#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include "../shared/worker_thread_pool.h"

typedef struct {
	int a;
	int b;
	int result;
} task_data;

int callback(int id, void *data) {
	task_data *d = data;
	printf("task executing on worker %i, a=%i, b=%i\n", id, d->a, d->b);
	fflush(stdout);
	d->result = d->a + d->b;
	return 0;
}

int do_task_wait_complete(worker_thread_pool *pool, int a, int b) {
	task_data d;
	d.a = a;
	d.b = b;
	printf("enqeueing a=%i, b=%i\n", a, b);
	fflush(stdout);
	int result;
	worker_thread_pool_enqueue(pool, &d, &result, WORKER_THREAD_POOL_ENQUEUE_MODE_WAIT_COMPLETE);
	printf("worker status code result %i\n", result);
	return d.result;
}

int main(int argc, char **argv) {
	worker_thread_pool pool;
	if (worker_thread_pool_init(&pool, callback, 2, 10)) {
		return 1;
	}

	srand(time(NULL));

	const int test_set_len = 10000;
	task_data *expected = malloc(sizeof(task_data) * test_set_len);
	for (int i = 0; i < test_set_len; i++) {
		expected[i].a = rand();
		expected[i].b = rand();
		expected[i].result = expected[i].a + expected[i].b;
	}

	for (int i = 0; i < test_set_len; i++) {
		assert(do_task_wait_complete(&pool, expected[i].a, expected[i].b) == expected[i].result);
	}

	for (int i = 0; i < test_set_len; i++) {
		/*
		TODO JEFF WORKER_THREAD_POOL_ENQUEUE_MODE_WAIT_ENQUEUE
		prove all tasks eventually complete, no enqueue errors
		*/
	}

	for (int i = 0; i < test_set_len; i++) {
		/*
		TODO JEFF WORKER_THREAD_POOL_ENQUEUE_MODE_NO_WAIT
		prove just the first few tasks complete, and the rest error with a queue full error
		*/
	}

	free(expected);
	worker_thread_pool_destroy(&pool);
	return 0;
}