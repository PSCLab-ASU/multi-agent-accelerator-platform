//ACL Kernel
__kernel void vector_add (__global float* a, __global float* b, __global float* c){
	//get unique id of work item
	int idx = get_global_id (0);
	printf("Global ID: %d", idx);

	//add a and b, then store in c
	c[idx] = a[idx] + b[idx];
}

