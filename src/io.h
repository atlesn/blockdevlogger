#ifndef BDL_IO_H
#define BDL_IO_H

struct io_file {
	FILE *file;
	int size;
};

void io_close (struct io_file *file);
int io_open(const char *path, const char *mode, struct io_file *file);
int io_write_block(struct io_file *file, int position, const char *data, int data_length, const char *padding, int padding_length, int verbose);
int io_read_block(struct io_file *file, int position, char *data, int data_length);

#endif
