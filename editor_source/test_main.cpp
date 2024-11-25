
#include <ht_utils/fire/fire_ds.h>

#include <stdio.h>

int main() {
	
	DS_BucketArray(int) my_ints;
	DS_BkArrInit(&my_ints, DS_HEAP, 4);
	
	DS_BucketArray(float) my_floats;
	DS_BkArrInit(&my_floats, DS_HEAP, 4);
	
	DS_BkArrPush(&my_floats, 1.5f);
	DS_BkArrPush(&my_floats, 2.5f);
	DS_BkArrPush(&my_floats, 3.5f);
	DS_BkArrPush(&my_floats, 4.5f);

	DS_BkArrPush(&my_ints, 1);
	DS_BkArrPush(&my_ints, 2);
	DS_BkArrPush(&my_ints, 3);
	DS_BkArrPush(&my_ints, 4);

	for (DS_BkArrEach(&my_ints, i)) {
		int val = *DS_BkArrGet(&my_ints, i);
		
		for (DS_BkArrEach(&my_floats, j)) {
			float fval = *DS_BkArrGet(&my_floats, j);
			printf("Value is %d  %f\n", val, fval);
		}
	}

	__debugbreak();

}
