#define FUSE_USE_VERSION 26

#include <fuse.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#define NAME_LENGTH 255
static int *file_offset_end;
static char **file_name;
static int *file_size;
static int file_count = 0;
#define STORE_FILE "/home/me4/Desktop/lab4_pmvs/all_file"
#define BUF_FILE "/home/me4/Desktop/lab4_pmvs//buffer_file"
struct file_info {
	char file_name[NAME_LENGTH];
	int file_size;
	int file_offset;
};
static int path_index(const char* path)
{
	int i  = 0;
	for(i = 0; i < file_count; i++) {
		if(strcmp(file_name[i], path)==0) {
			return i;
		}
	}
	return -1;
}
static int getattr_callback(const char *path, struct stat *stbuf) 
{
	memset(stbuf, 0, sizeof(struct stat));
	if (strcmp(path, "/") == 0) {
		stbuf->st_mode = S_IFDIR | 0755;
		stbuf->st_nlink = 2;
		return 0;
	} else {
		int index = path_index(path);
		if(index == -1){
			return -ENOENT;
		}
		stbuf->st_mode = S_IFREG | 0777;
		stbuf->st_nlink = 1;
		int start = index == 0 ? 0 : file_offset_end[index-1];
		int size = file_offset_end[index]-start;
		printf("%d\n", size);
		stbuf->st_size = file_size[index];
		return 0;
	}
	return -ENOENT;
}

static int readdir_callback(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi) 
{
	filler(buf, ".", NULL, 0);
	filler(buf, "..", NULL, 0);
	int i;
	for(i = 0; i < file_count; i++) {
		if(strlen(file_name[i])!= 0) {
			filler(buf, file_name[i]+1, NULL, 0);
		}
	}
	return 0;
}

static int open_callback(const char *path, struct fuse_file_info *fi) 
{
  	int index = path_index(path);
	if(index==-1)
		return -ENOENT;
	return 0;
}

static int read_callback(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi) 
{
	(void) fi;
	int index = path_index(path);
	FILE *file_in = fopen(STORE_FILE, "rb");
	int start = index == 0 ? 0 : file_offset_end[index-1];
	fseek(file_in, start, SEEK_SET);
	fread(buf, file_offset_end[index] - start, 1,file_in);
	size = file_offset_end[index]-start;
	printf("%d\n",file_offset_end[index]-start);
	printf("%s\n", buf);
	fclose(file_in);
	return size;
}
static int fst_utimens (const char *v, const struct timespec tv[2])
{
	return 0;
}
static int fst_getxattr (const char *x, const char *y, char *z, size_t f)
{
	return 0;
}
static int fst_setxattr (const char *x, const char *y, const char *z, size_t l, int f)
{
	return 0;
}
static int fst_listxattr (const char *x, char *y, size_t z)
{
	return 0;
}
static int fst_write (const char *path, const char *buf, size_t size, off_t offset, struct fuse_file_info *fi)
{
	int index = path_index(path);
	if (index == -1) {
		return -ENOENT;
	}
	FILE *file_in = fopen(STORE_FILE, "rb+");
	int start = index == 0 ? 0 : file_offset_end[index-1];
	fseek(file_in, start+offset, SEEK_SET);
	fwrite(buf, size, 1, file_in);
	printf("%d", start);
	printf("%s\n", buf);
	if(offset==0){
		file_size[index] = 0;
	}
	file_offset_end[index] = start + offset;
	file_size[index]+=size;
	fclose(file_in);
	return size;
}
static int fst_mknod (const char * path, mode_t mode, dev_t dev)
{
	int index = path_index(path);
	if(index!=-1) {
		return -ENOENT;
	}
	else {
		file_count++;
		int i  = 0;
		int* buf = (int*)malloc(file_count*sizeof(int));
		char **buf_name = (char**)malloc(file_count*sizeof(char*));
		int* size_buf = (int*)malloc(file_count*sizeof(int));
		for(i = 0; i< file_count; i++) {
			buf_name[i] = (char*)malloc(NAME_LENGTH*sizeof(char));
		}
		for(i  = 0; i< file_count-1; i++) {
			buf[i] = file_offset_end[i];
			size_buf[i] = file_size[i];
			memset(buf_name[i], 0, NAME_LENGTH);
			strcpy(buf_name[i], file_name[i]);
		}
		if(file_count!=1) {
			for(i = 0; i < file_count-2; i++) {
				free(file_name[i]);
			}
			free(file_name);
			free(file_offset_end);
			free(file_size);
		}
		for(i = 0; i < file_count-1; i++) {
			printf("%s\n", buf_name[i]);
		}
		buf[file_count-1] = file_count==1 ? 0 : buf[file_count-2];
		size_buf[file_count-1] = 0; 
		memset(buf_name[file_count-1], 0, NAME_LENGTH);
		strcpy(buf_name[file_count-1], path);
		for(i = 0; i < file_count; i++) {
			printf("%s\n", buf_name[i]);
			printf("%d\n", buf[i]);
		}
		file_name = buf_name;
		file_offset_end = buf;
		file_size = size_buf;
	}
	return 0;
}
static int fst_unlink (const char *path)
{
	int index = path_index(path);
	if(index==-1){
		return -ENOENT;
	}
	memset(file_name[index], 0, NAME_LENGTH);
	return 0;
}
static int fst_truncate (const char * z, off_t v)
{
	return 0;
}
static struct fuse_operations fuse_example_operations = {
	.getattr = getattr_callback,
	.mknod = fst_mknod,
	.open = open_callback,
	.read = read_callback,
	.readdir = readdir_callback,
	.utimens = fst_utimens,
	.setxattr	= fst_setxattr,
	.getxattr	= fst_getxattr,
	.listxattr = fst_listxattr,
	.write = fst_write,
	.truncate = fst_truncate,
	.unlink = fst_unlink
};

int main(int argc, char *argv[])
{
	FILE *file_in = fopen(BUF_FILE, "rb");
	fseek(file_in, 0, SEEK_SET);
	file_count = 0;
	fread(&file_count, sizeof(int), 1, file_in);
	//printf("file %d\n", file_count);
	if(file_count!=0) {
		file_offset_end = (int*)malloc(file_count*sizeof(int));
		file_name = (char**)malloc(file_count*sizeof(char*));
		file_size = (int*)malloc(file_count*sizeof(int));
		int i = 0;
		for(i = 0; i< file_count; i++) {
			file_name[i] = (char*)malloc(NAME_LENGTH*sizeof(char));
		}
		for(i = 0; i < file_count; i++) {
			struct file_info info;
			memset(info.file_name, 0, NAME_LENGTH);
			fread(&info, sizeof(struct file_info), 1, file_in);
			//printf("file_offset %d\n", info.file_offset);
			//printf("file_size %d\n", info.file_size);
			//printf("file_name %s\n", info.file_name);
			file_size[i] = info.file_size;
			file_offset_end[i] = info.file_offset;
			memset(file_name[i], 0, NAME_LENGTH);
			strcpy(file_name[i], info.file_name);
		}
	}
	fclose(file_in);
	int x = fuse_main(argc, argv, &fuse_example_operations, NULL);
	file_in = fopen(BUF_FILE, "rb+");
	fseek(file_in, 0, SEEK_SET);
	//printf("file %d\n", file_count);
	fwrite(&file_count, sizeof(int), 1, file_in);
	int i = 0;
	for(i = 0; i < file_count; i++) {
		struct file_info info;
		memset(info.file_name, 0, NAME_LENGTH);
		strcpy(info.file_name, file_name[i]);
		info.file_size = file_size[i];
		info.file_offset = file_offset_end[i];
		fwrite(&info, sizeof(struct file_info), 1, file_in);
		//printf("file_offset %d\n", info.file_offset);
		//printf("file_size %d\n", info.file_size);
		//printf("file_name %s\n", info.file_name);
	}
	for(i = 0; i < file_count-2; i++) {
		free(file_name[i]);
	}
	free(file_name);
	free(file_offset_end);
	free(file_size);
	fclose(file_in);
	return x;
}
