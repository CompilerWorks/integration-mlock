#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/mman.h>
#include <getopt.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>

int
main(int argc, char *argv[])
{
	int c;
	int onfault = 0, hugetlb = 0;
	while ((c = getopt(argc, argv, "hFt")) != -1) {
		switch (c) {
			case 'F':
				onfault = 1;
				break;
			case 't':
				hugetlb = 1;
				break;
			case 'h':
			default:	/* ? */
				fprintf(stderr, "Usage: %s [-Ft] <file>\n"
					"  -t: Use MAP_HUGETLB\n"
					"  -F: Use MLOCK_ONFAULT (unimplemented)\n",
					argv[0]);
				exit(EXIT_FAILURE);
		}
	}

	printf("Starting memory-locker, process id %d\n", getpid());

	for (int i = optind; i < argc; i++) {
		printf("Opening file %s\n", argv[i]);
		int fd = open(argv[i], O_RDONLY);
		if (fd < 0) {
			fprintf(stderr, "Failed to open %s: %s\n", argv[i], strerror(errno));
			exit(EXIT_FAILURE);
		}
		// printf("File descriptor is %d\n", fd);

		struct stat statbuf;
		if (fstat(fd, &statbuf) < 0) {
			fprintf(stderr, "Failed to stat %s: %s\n", argv[i], strerror(errno));
			exit(EXIT_FAILURE);
		}

		int flags = MAP_SHARED;
		// MAP_HUGETLB isn't available on macOS and in older Linux versions (e.g., Ubuntu 12.04)
#if defined MAP_HUGETLB
		flags |= (hugetlb ? MAP_HUGETLB : 0);
#endif

		void *ptr = mmap(NULL, statbuf.st_size, PROT_READ, flags, fd, 0);
		// printf("Mapped address %p\n", ptr);
		if (ptr == MAP_FAILED || ptr == 0) {
			fprintf(stderr, "Failed to map %s (%jd bytes): %s\n", argv[i], statbuf.st_size, strerror(errno));
			exit(EXIT_FAILURE);
		}

		printf("Locking file %s (%jd bytes)\n", argv[i], statbuf.st_size);
		if (mlock(ptr, statbuf.st_size) < 0) {
			fprintf(stderr, "Failed to lock %s (%jd bytes): %s\n", argv[i], statbuf.st_size, strerror(errno));
			exit(EXIT_FAILURE);
		}
	}

	printf("Waiting for Godot...\n");
	for (;;)
		pause();

	return 0;
}
