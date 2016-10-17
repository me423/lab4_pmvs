fuser:
	gcc -Wall filesystem.c -D_FILE_OFFSET_BITS=64 -I/usr/include/fuse  -pthread -lfuse -lrt -ldl -o filesys
