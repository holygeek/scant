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
"    percentage of sparse content. TODO If a directory is given scant recurses into\n"
"    the directory.\n"
"\n"
"OPTIONS\n"
"    -h\n"
"        Show this help.\n\n"
"    -a\n"
"       Report all files.\n\n"
"    -q\n"
"        Do not print filenames.\n\n"
"EXIT STATUS\n"
"    0 if at least one of the files is sparse.\n"
"    1 if there are no sparse files, or one of the files is empty.\n"
"    2 if there was an error.\n"
;

int opt_quiet = 0;
int opt_all = 0;

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

#define EMPTY -3

// is_sparse returns:
// 0 if file is 0 bytes
// -1 if file is not sparse
// -2 on error
// -3 if file is empty
// N where N is number of sparse "holes" found in file
int is_sparse(char *filename) {
	int fd = open(filename, O_RDONLY);
	if (fd < 0) {
		perror(filename);
		return -2;
	}

	struct stat st;
	if (fstat(fd, &st) < 0) {
		perror(filename);
		close(fd);
		return -2;
	}


	if (st.st_size == 0) {
		if (!opt_quiet) {
			printf("%7d %s\n", 0, filename);
		}
		close(fd);
		return EMPTY;
	}


	off_t nholes = count_holes(fd, st.st_size);
	close(fd);
	if (nholes < 0) {
		// uh-oh something's not right
		printf("%s\n", filename);
		return -2;
	}

	if (nholes > 0 || opt_all) {
		if (!opt_quiet) {
			printf("%6.2f%% %s\n", 100.0*nholes/st.st_size, filename);
		}
	}
	return nholes;
}

int main(int argc, char *argv[])
{
	int i;
	for (i = 1; i < argc; i++) {
		if (!strcmp(argv[i], "-h")) {
			print_usage();
			return 0;
		}
		if (!strcmp(argv[i], "-a")) {
			opt_all = 1;
		}
		if (!strcmp(argv[i], "-q")) {
			opt_quiet = 1;
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

	int err = 0;
	int hasSparse = 0;
	for (; i < argc; i++) {
		switch (is_sparse(argv[i])) {
			case -1: break;
			case -2:
				 if (err == 0)
					 err = 1;
				 break;
			case EMPTY:
				 // treat empty file as sparse
				 hasSparse = 1;
				 break;
			case 0:
				 /* empty */
				 break;
			default:
				 if (!hasSparse)
					 hasSparse = 1;
				 break;
		}
	}
	if (err) {
		return 2;
	}
	return !hasSparse;
}
