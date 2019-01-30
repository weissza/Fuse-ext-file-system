
#include "bitmap.h"

int
bitmap_get(void* bm, int ii){
	char bit = ((char*)bm)[ii/8] & (1 << (ii % 8));
	return bit != 0;
}

void
bitmap_put(void* bm, int ii, int vv){
	((char*)bm)[ii/8] |= (vv << (ii % 8)); 
}

void
bitmap_print(void* bm, int size){

}
