#include <sys/time.h>
#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>

uint64_t time_get_64() {
	struct timeval tv;
	if (gettimeofday(&tv, NULL) != 0) {
		fprintf (stderr, "Error while getting time, cannot recover from this: %s\n", strerror(errno));
		exit (EXIT_FAILURE);
	}
	uint64_t total = 1000 * tv.tv_sec;
	total += tv.tv_usec / 1000;
	return total;
}
