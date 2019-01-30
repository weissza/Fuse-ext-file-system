
#include <stdio.h>
#include <errno.h>
#include <stdint.h>
#include <assert.h>
#include <sys/types.h>
#include <time.h>
#include <stdlib.h>

#include "pages.h"
#include "storage.h"
#include "bitmap.h"
#include "inode.h"
#include "directory.h"
#include "util.h"


#include <string.h>
#define NUM_INODES 64;
#define NUM_BLOCKS 256;

void
storage_init(const char* path){
	pages_init(path);
	directory_init();
}


int
storage_stat(const char* path, struct stat* st){
	int inum = tree_lookup(path);
	if (inum < 0 ){
		printf("-------------------- not found\n");
		return inum;
	}
	inode* node = get_inode(inum);
	st->st_mode = node->mode;
	st->st_size = node->size;
	st->st_uid = getuid();
	st->st_blksize = 4096;
	st->st_gid = getgid();
	st->st_ino = inum;
	st->st_nlink = node->refs;
	//printf("path %s wow mode %d, inum, %d \n",path, node->mode, inum);
	return 0;
}

int
storage_read(const char* path, char* buf, size_t size, off_t offset){
	int inum = tree_lookup(path);
	inode* node = get_inode(inum);

	//int grow = node->size - offset + size;
	//grow_inode(node, grow);

	int start_page = offset / 4096;
	int s_position = offset % 4096;

	int page = 0;
	int written = 0;
	int max = 0;

	while(written < size){
		int pnum = inode_get_pnum(node, start_page + page);
		char* data = (char*)pages_get_page(pnum);
		if(size - written > 4096){
			max = 4096;
			page++;
		}else{
			max = size - written;
		}
		strncpy(buf + written, data + s_position, max);
		s_position = 0;
		written += max;
	}
	time_t now = time(0);
	node->atime = now;
	//node->size += grow;
	return size;


/*	int inum = tree_lookup(path);
	inode* node = get_inode(inum);

	char* data = (char*)pages_get_page(node->ptrs[0]);
	data += offset;
	int length = strlen(data);
	if( size < length){
		length = size;
	}
	strncpy(buf,data,length);
	return length;*/
}


int
storage_write(const char* path, const char* buf, size_t size, off_t offset){
	int inum = tree_lookup(path);
	inode* node = get_inode(inum);

	int grow = node->size - offset + size;
	grow_inode(node, grow);

	int start_page = offset / 4096;
	int s_position = offset % 4096;

	int page = 0;
	int written = 0;
	int max = 0;

	while(written < size){
		int pnum = inode_get_pnum(node, start_page + page);
		char* data = (char*)pages_get_page(pnum);
		if(size - written > 4096){
			max = 4096;
			page++;
		}else{
			max = size - written;
		}
		strncpy(data + s_position, buf + written, max);
		s_position = 0;
		written += max;
	}
	time_t now = time(0);
	node->mtime = now;
	node->ctime = now;

	node->size += grow;
	return size;
}





int
storage_mknod (const char *path, int mode){
	char* parent = (char*)malloc(strlen(path));
	char* name = (char*)malloc(strlen(path));
	char* temp_p = parent;
	char* temp_n = name;
	strcpy(name,path);
	strcpy(parent,path);
	name = strrchr(name,'/');
	name += 1;
	int len = strlen(path) - strlen(name);
	parent[len] = 0;
	printf("name = %s, parent = %s \n", name,parent);

	int pinum = tree_lookup(parent);
	inode* pinode = get_inode(pinum);
	int inum_new_inode = alloc_inode();
	inode* node_new = get_inode(inum_new_inode);

	node_new->mode = mode;
	node_new->size = 0;
	node_new->refs = 1;
	node_new->ptrs[0] = alloc_page();
	time_t now = time(0);
	node_new->atime = now;
	node_new->ctime = now;
	node_new->mtime = now;

	printf("%d pinode, %s name, %d inum\n", pinum, name, inum_new_inode);
	int rv = directory_put(pinode, name, inum_new_inode);
	if (rv < 0){
		printf("wowwwwwwwwwwww\n");

	}
	free(temp_p);
	free(temp_n);
	return rv;
}


slist*
storage_list(const char* path){
	int inum = tree_lookup(path);
	inode* node = get_inode(inum);
	time_t now = time(0);
	node->atime = now;
	return directory_list(path);
}

int
storage_set_time(const char* path, const struct timespec ts[2]){
	int inum = tree_lookup(path);
	inode* node = get_inode(inum);
	node->atime = ts[0].tv_sec;
	node->mtime = ts[1].tv_sec;
	return 0;
}


int
storage_link(const char *from, const char *to){
	printf("here\n");

	int inum = tree_lookup(from);
	if (inum < 0 ){
		return inum;
	}
	inode *node = get_inode(inum);

	char* parent = (char*)malloc(strlen(to));
	char* name = (char*)malloc(strlen(to));
	char* temp_p = parent;
	char* temp_n = name;
	strcpy(name,to);
	strcpy(parent,to);
	name = strrchr(name,'/');
	name += 1;
	int len = strlen(to) - strlen(name);
	parent[len] = 0;

	node->refs++;
	int pinum = tree_lookup(parent);
	if(pinum < 0){
		return pinum;
	}
	inode *pnode = get_inode(pinum);
	directory_put(pnode, name, inum);
	free(temp_p);
	free(temp_n);
	return 0;
}


int
storage_unlink(const char* path){
	char* parent = (char*)malloc(strlen(path));
	char* name = (char*)malloc(strlen(path));
	char* temp_p = parent;
	char* temp_n = name;
	strcpy(name,path);
	strcpy(parent,path);
	name = strrchr(name,'/');
	name += 1;
	int len = strlen(path) - strlen(name);
	parent[len] = 0;
	int pinum = tree_lookup(parent);
	int inum = tree_lookup(path);
	int rv = 0;
	if (inum < 0){
		return inum;
	}
	if(pinum < 0){
		return pinum;
	}
	inode* node = get_inode(inum);
	if(node->refs > 1){
		node->refs--;
		return directory_delete(get_inode(pinum),name);
	}else{
		free_page(node->ptrs[0]);
		void* bm = get_inode_bitmap();
		bitmap_put(bm,inum, 0);
		rv = directory_delete(get_inode(pinum),name);
		free(temp_p);
		free(temp_n);
		return rv;
	}
}


int
storage_rename(const char* from, const char* to){
	int inum = tree_lookup(from);
	if (inum < 0){
		return inum;
	}

	int rv = storage_link(from,to);
	if (rv < 0){
		return rv;
	}

	return storage_unlink(from);
/*	char* parent = malloc(strlen(from));
	char* name = malloc(strlen(from));
	char* temp_p = parent;
	char* temp_n = name;

	strcpy(parent, from);
	strcpy(name, from);
	name = strrchr(name,'/');
	name += 1;
	int len = strlen(from) - strlen(name);
	parent[len] = 0;
	int pinum = tree_lookup(parent);
	if(pinum < 0){
		return pinum;
	}
	inode* node = get_inode(pinum);
	dirent* block = (dirent*)pages_get_page(node->ptrs[0]);
	for(int ii = 0; ii < (4096/sizeof(dirent)); ++ii){
		if(strcmp(name,block[ii].name) == 0){
			strcpy(block[ii].name, to);
			return 0;
		}
	}
	return -ENOENT;*/
}

int storage_chmod(const char* path, mode_t mode) {


    int inum = tree_lookup(path);
    if(inum < 0) {
	    return -ENOENT;
    }
    inode* inode = get_inode(inum);
    inode->mode = mode;
    return 0;
}	
