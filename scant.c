#define _GNU_SOURCE 1
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

char *name = "scant";
char *usage = "NAME\n"
"    %s - report file sparseness\n"
"\n"
"SYNOPSIS\n"
"    %s [-h] [-a] [-q] <files>...\n"
"\n"
"DESCRIPTION\n"
"    Report whether files are sparse, or empty. For each sparse file prints the\n"
"    percentage of sparse content.\n"
"\n"
"OPTIONS\n"
"    -h\n"
"        Show this help.\n\n"
"    -a\n"
"       Report all files.\n\n"
"    -q\n"
"        Do not print filenames.\n\n"
"EXIT STATUS\n"
"    0 if at least one of the files is sparse, 1 otherwise.\n"
;

void print_usage() {
	printf(usage, name, name);

}

off_t count_holes(int fd, size_t size) {
	off_t pos_hole = lseek(fd, 0, SEEK_HOLE);
	if (pos_hole < 0) {
		perror("lseek SEEK_HOLE");
		return -1;
	}

	off_t nholes = 0;

	while (pos_hole < size) {
		off_t data = lseek(fd, pos_hole+1, SEEK_DATA);
		if (data < 0) {
			nholes += size-pos_hole;
			return nholes;
		}
		nholes += data-pos_hole;
		// printf("added %ld\n", data-pos_hole);
		pos_hole = lseek(fd, data, SEEK_HOLE);
		if (pos_hole < 0) {
			perror("lseek SEEK_HOLE 2");
			return -1;
		}
	}


	return nholes;
}

int main(int argc, char *argv[])
{
	int i;
	int quiet = 0;
	int all = 0;
	for (i = 1; i < argc; i++) {
		if (!strcmp(argv[i], "-h")) {
			print_usage();
			return 0;
		}
		if (!strcmp(argv[i], "-a")) {
			all = 1;
		}
		if (!strcmp(argv[i], "-q")) {
			quiet = 1;
		}
		if (argv[i][0] != '-') {
			break;
		}

	}

	if (argc - i < 1) {
		fprintf(stderr, "usage: %s <filename...>\n", name);
		return 1;
	}
	//printf("DEBUG total %d files\n", argc-i);

	int ret = 0;
	struct stat buf;
	for (; i < argc; i++) {
		int fd = open(argv[i], O_RDONLY);
		if (fd < 0) {
			perror(argv[i]);
			continue;
		}

		if (fstat(fd, &buf) < 0) {
			perror(argv[i]);
			close(fd);
			return -1;
		}

		if (buf.st_size == 0) {
			printf("%7d %s\n", 0, argv[i]);
			close(fd);
			continue;
		}


		off_t nholes = count_holes(fd, buf.st_size);
		close(fd);
		if (nholes < 0) {
			// uh-oh something's not right
			printf("%s\n", argv[i]);
			if (ret == 0) {
				ret = 1;
			}
			continue;
		}

		if (nholes > 0 || all) {
			if (!quiet) {
				printf("%6.2f%% %s\n", 100.0*nholes/buf.st_size, argv[i]);
			}
		} else if (ret == 0) {
			ret = 1;
		}
	}
	return ret;
}
