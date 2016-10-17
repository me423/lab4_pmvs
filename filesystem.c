#define FUSE_USE_VERSION 26

#include <fuse.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#define NAME_LENGTH 255
static int *file_offset_end;
static char **file_name;
static int file_count = 0;
#define STORE_FILE "/home/user/pmvs4/all_file"
#define BUF_FILE "/home/user/pmvs4/buffer_file"

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
		stbuf->st_size = size;
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
	FILE *file_buf;
	int length = 0;
	int start = index == 0 ? 0 : file_offset_end[index-1];
	if(index != file_count - 1) {
		file_buf = fopen(BUF_FILE, "rb+");
		fseek(file_buf, 0, SEEK_SET);
		fseek(file_in, 0, SEEK_END);
		length = file_offset_end[file_count-1]-file_offset_end[index];
		fseek(file_in, file_offset_end[index], SEEK_SET);
		int buf_tr[4];
		int j = 0;
		for(j = 0; j < 4; j++) {
			buf_tr[j] = 0;
		}
		while(fread(buf_tr, sizeof(int), 4, file_in) != 0) {
			fwrite(buf_tr, sizeof(int), 4, file_buf);
		}
	}
	fseek(file_in, start, SEEK_SET);
	int buf_len =  strlen(buf);
	fwrite(buf, buf_len, 1, file_in);	
	printf("%d", start);
	printf("%s\n", buf);
	printf("%d\n", buf_len);
	int prev_len = file_offset_end[index]-start;
	file_offset_end[index] = start + buf_len;
	if(index != file_count - 1) {
		printf("+\n");
		int buf_tr[4];
		int j = 0;
		for(j = 0; j<4; j++) {
			buf_tr[j] = 0;
		}
		fseek(file_buf, 0, SEEK_SET);
		int curr_off = 0;
		int read = 0;
		while((read= fread(buf_tr, sizeof(int), 4, file_buf)) != 0) {
			curr_off+=read;
			fwrite(buf_tr, sizeof(int), 4, file_in);
			if(curr_off>=length) {
				break;
			}
		}
		for(j = index+1; j< file_count; j++) {
			file_offset_end[j]+=(buf_len-prev_len);
		}
		fclose(file_buf);
	}
	fclose(file_in);
	return buf_len;
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
		for(i = 0; i< file_count; i++) {
			buf_name[i] = (char*)malloc(NAME_LENGTH*sizeof(char));
		}
		for(i  = 0; i< file_count-1; i++) {
			buf[i] = file_offset_end[i];
			memset(buf_name[i], 0, NAME_LENGTH);
			strcpy(buf_name[i], file_name[i]);
		}
		if(file_count!=1) {
			for(i = 0; i < file_count-2; i++) {
				free(file_name[i]);
			}
			free(file_name);
			free(file_offset_end);
		}
		for(i = 0; i < file_count-1; i++) {
			printf("%s\n", buf_name[i]);
		}
		buf[file_count-1] = file_count==1 ? 0 : buf[file_count-2];
		memset(buf_name[file_count-1], 0, NAME_LENGTH);
		strcpy(buf_name[file_count-1], path);
		for(i = 0; i < file_count; i++) {
			printf("%s\n", buf_name[i]);
			printf("%d\n", buf[i]);
		}
		file_name = buf_name;
		file_offset_end = buf;
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
  return fuse_main(argc, argv, &fuse_example_operations, NULL);
}
