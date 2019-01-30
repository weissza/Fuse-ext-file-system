
#include <assert.h>
#include "directory.h"
#include "pages.h"
#include "slist.h"
#include <errno.h>
#include <string.h>

void
directory_init(){
	int rootn = alloc_inode();
	assert(rootn >= 0);
	inode* root = get_inode(rootn);
	root->refs = 1;
	root->mode = 040755;
	root->size = 0;
	root->ptrs[0] = alloc_page();
	printf("root mode %d \n", root->mode);
}


int
directory_lookup(inode* dd, const char* name){
	dirent* block = (dirent*)pages_get_page(dd->ptrs[0]);
	if(strcmp(name, "") == 0){
		//root node
		return 0;
	}

	for(int ii = 0; ii < (4096 / sizeof(dirent)); ++ii){
		dirent curr = block[ii];
		if(strcmp(name, curr.name) == 0){
			return curr.inum;
		}
	}
	return -ENOENT;
}


int
tree_lookup(const char* path){
	slist* parts = s_split(path, '/');
	slist* temp = parts;
	int dnum = 0;

	while(parts != NULL){
		inode* dir = get_inode(dnum);
		dnum = directory_lookup(dir, parts->data);
		parts = parts->next;
	}
	s_free(temp);
	return dnum;
}


int
directory_put(inode* dd, const char* name, int inum){
	dirent* list = (dirent*)pages_get_page(dd->ptrs[0]);
	for(int ii = 0; ii < (4096/sizeof(dirent)); ii++){
		if(strcmp(list[ii].name, "") == 0){
			strcpy(list[ii].name, name);
			list[ii].inum = inum;
			printf("name = %s\n", name);
			return 0;
		}
		if(strcmp(name, "dir2") == 0){
			printf("%s\n",list[ii].name);
		}
	}
	printf("also wowwwwww");
	return -ENOENT;
}

int
directory_delete(inode* dd, const char* name){
	dirent* list = (dirent*)pages_get_page(dd->ptrs[0]);
	for(int ii = 0; ii < (4096/sizeof(dirent)); ++ii){
		if(strcmp(list[ii].name, name) == 0){
			list[ii].name[0] = 0;
			list[ii].inum = 0;
			return 0;
		}
	}
	return -ENOENT;
}



slist*
directory_list(const char* path){
	int inum = tree_lookup(path);
	inode* node = get_inode(inum);
	dirent* block = pages_get_page(node->ptrs[0]);
	slist* list = NULL;
	for(int ii = 0; ii < (4096/sizeof(dirent)); ++ii){
		if(strcmp(block[ii].name, "") != 0){
			list = s_cons(block[ii].name, list);
		}
	}
	return list;
}
