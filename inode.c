#include "inode.h"
#include "pages.h"
#include "storage.h"
#include "bitmap.h"

#include "util.h"



int
alloc_inode(){
	void* inode_bm = get_inode_bitmap();
	for(int ii = 0; ii < 72; ++ii){
		if(!bitmap_get(inode_bm, ii)){
			bitmap_put(inode_bm, ii, 1);
			printf("inode allocated, %d \n", ii);
			return ii;
		}
	}
	return -1;
}

inode*
get_inode(int inum){
	return get_inode_bitmap() + 9 + (inum * sizeof(inode));
}



int
grow_inode(inode* node, int size){
	int new_size = bytes_to_pages(size);
	int start = bytes_to_pages(node->size);
	for(int ii = start; ii <= (new_size + start); ii++){
		if(ii < 2){
			if(node->ptrs[ii] == 0){
				node->ptrs[ii] = alloc_page();
			}
		}else{
			if(node->iptr == 0){
				node->iptr = alloc_page();
			}
			int* plist = (int*)pages_get_page(node->iptr);
			if(plist[ii] == 0){
				plist[ii] = alloc_page();
			}
		}
	}
	return 0;
}

int
inode_get_pnum(inode* node, int fpn){
	if(fpn < 2){
		return node->ptrs[fpn];
	}else{
		int* plist = (int*)pages_get_page(node->iptr);
		return plist[fpn];
	}
}
