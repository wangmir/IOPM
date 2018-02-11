#include "iopm.h"
#include "main.h"
#include "init.h"


void init() {
	
	init_SIT();
	init_CLUSTER();
	init_BIT();
	init_PVB();
	init_COUNT();

	free_block = FREE_BLOCK;
	ALLOC_PARTITION = 0;


	GC_temp_LPN = (int *)malloc(sizeof(int)*(PARTITION_PER_CLUSTER + 2)*(PAGE_PER_PARTITION));
	GC_temp_PPN = (int *)malloc(sizeof(int)*(PARTITION_PER_CLUSTER + 2)*(PAGE_PER_PARTITION));

	PGC_plist = (int *)malloc(sizeof(int) * PAGE_PER_BLOCK);

	remove_block_in_partition = (int *)malloc(sizeof(int) * BLOCK_PER_PARTITION);
}

void init_COUNT() {
	
	memset(&stat, 0x00, sizeof(_STAT));
}

void init_PVB() {

	int i, j;

	PVB = (_PVB *)malloc(sizeof(_PVB) * NUMBER_PARTITION);

	memset(PVB, 0x00, sizeof(_PVB) * NUMBER_PARTITION);

	INIT_LIST_HEAD(&free_partition_pool);

	for (i = 0; i<NUMBER_PARTITION; i++) {

		PVB[i].bitmap = (int *)malloc(sizeof(int) * PAGE_PER_PARTITION_32);
		memset(PVB[i].bitmap, 0x00, sizeof(int) * PAGE_PER_PARTITION_32);

		PVB[i].startLPN = -1;
		PVB[i].startPPN = -1;
		PVB[i].endPPN = -1;

		PVB[i].block = (int *)malloc(sizeof(int)*(BLOCK_PER_PARTITION + 1));

		INIT_LIST_HEAD(&PVB[i].p_list);

		PVB[i].partition_num = i;

		// mid block (if the partition can allocate more than 2 blocks
		for (j = 0; j < BLOCK_PER_PARTITION + 1; j++)
			PVB[i].block[j] = -1;

		list_add(&PVB[i].p_list, &free_partition_pool);
	}

	free_partition = NUMBER_PARTITION;
}

void init_BIT() {

	INIT_LIST_HEAD(&allocated_block_pool);
	INIT_LIST_HEAD(&free_block_pool);

	BIT = (_BIT *)malloc(sizeof(_BIT) * FREE_BLOCK);
	memset(BIT, 0x00, sizeof(_BIT) * FREE_BLOCK);

	for (int i = 0; i < FREE_BLOCK; i++) {
		BIT[i].block_num = i;
		BIT[i].free_flag = 1;
		INIT_LIST_HEAD(&BIT[i].b_list);
		INIT_LIST_HEAD(&BIT[i].linked_partition);

		list_add(&BIT[i].b_list, &free_block_pool);
	}

	INIT_LIST_HEAD(&free_r_plist);

	// block needs to map linked partition to handle block GC 
	// at block GC, linked partition for the victim block should delete the block map at the PVB
	int num_r_plist = 2 * NUMBER_PARTITION * (BLOCK_PER_PARTITION + 1);
	r_plist = (_LIST_MAP *)malloc(sizeof(_LIST_MAP) * num_r_plist);

	num_allocated_r_plist = 0;

	for (int i = 0; i < num_r_plist; i++) {
		r_plist[i].value = -1;
		list_add(&r_plist[i].list, &free_r_plist);
	}

}

void init_SIT() {

	INIT_LIST_HEAD(&active_stream_pool);
	INIT_LIST_HEAD(&free_stream_pool);

	SIT = (_SIT *)malloc(sizeof(_SIT) * NUMBER_STREAM);

	for (int i = 0; i<NUMBER_STREAM; i++) {
		SIT[i].activePartition = -1;
		SIT[i].recentLPN = -1;
		SIT[i].recentPPN = -1;

		SIT[i].stream_num = i;
		INIT_LIST_HEAD(&SIT[i].SIT_list);
		list_add(&SIT[i].SIT_list, &free_stream_pool);
	}
}

void init_CLUSTER() {

	CLUSTER = (_CLUSTER *)malloc(sizeof(_CLUSTER) * NUMBER_CLUSTER);

	memset(CLUSTER, 0x00, sizeof(_CLUSTER) * NUMBER_CLUSTER);

	for (int i = 0; i<NUMBER_CLUSTER; i++) {
		INIT_LIST_HEAD(&CLUSTER[i].p_list);
		INIT_LIST_HEAD(&CLUSTER[i].c_list);

		CLUSTER[i].cluster_num = i;
	}

	INIT_LIST_HEAD(&victim_cluster_list);
}

void init_Partition(int partition) {

	memset(PVB[partition].bitmap, 0x00, sizeof(int) * PAGE_PER_PARTITION_32);

	PVB[partition].startLPN = -1;
	PVB[partition].startPPN = -1;
	PVB[partition].endPPN = -1;
	PVB[partition].valid = 0;
	PVB[partition].blocknum = 0;
	PVB[partition].active_flag = 0;
	PVB[partition].free_flag = 1;
	PVB[partition].victim_partition_flag = 0;

	// start/end를 제외한 가운데 block
	for (int i = 0; i < BLOCK_PER_PARTITION + 1; i++) {
		PVB[partition].block[i] = -1;
	}
}
