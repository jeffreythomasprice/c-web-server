#include <stdio.h>

#include "../shared/worker_thread_pool.h"

typedef struct {
	int a;
	int b;
} task_data;

int callback(int id, void *data) {
	task_data *d = data;
	printf("task executing on worker %i, a=%i, b=%i", id, d->a, d->b);
	fflush(stdout);
	return d->a + d->b;
}

int do_task(worker_thread_pool *pool, int a, int b) {
	task_data d;
	d.a = a;
	d.b = b;
	int result;
	printf("enqeueing a=%i, b=%i\n", a, b);
	fflush(stdout);
	worker_thread_pool_enqueue(&pool, &d, &result);
	return result;
}

int main(int argc, char **argv) {
	worker_thread_pool pool;
	if (worker_thread_pool_init(&pool, callback, 2, 10)) {
		return 1;
	}

	printf("%i\n", do_task(&pool, 1, 2));
	printf("%i\n", do_task(&pool, 3, 4));
	printf("%i\n", do_task(&pool, 5, 6));
	printf("%i\n", do_task(&pool, 7, 8));
	printf("%i\n", do_task(&pool, 9, 10));

	worker_thread_pool_destroy(&pool);
	return 0;
}