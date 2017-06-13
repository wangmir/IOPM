#include "iopm.h"
#include "main.h"
#include "init.h"
#include "list.h"


void init() {
	
	init_SIT();
	init_CLUSTER();
	init_BIT();
	init_PVB();
	init_COUNT();

	init_partition_pool();
	init_block_pool();

	free_block = FREE_BLOCK;
	ALLOC_PARTITION = 0;


	victim_page_LPN = (int *)malloc(sizeof(int)*(PARTITION_PER_CLUSTER + 2)*(PAGE_PER_PARTITION));
	victim_page_PPN = (int *)malloc(sizeof(int)*(PARTITION_PER_CLUSTER + 2)*(PAGE_PER_PARTITION));
	victim_partition = (int *)malloc(sizeof(int)*(PARTITION_PER_CLUSTER + 2));

	remove_block_in_partition = (int *)malloc(sizeof(int) * BLOCK_PER_PARTITION);
}

void init_COUNT() {
	COUNT.read = 0;
	COUNT.write = 0;
	COUNT.IO_mem = 0;
	COUNT.IO_mem_M = 0;
	COUNT.partition.gc = 0;
	COUNT.partition.gc_read = 0;
	COUNT.partition.gc_write = 0;
	COUNT.partition.mem = 0;
	COUNT.block.gc = 0;
	COUNT.block.gc_read = 0;
	COUNT.block.gc_write = 0;
	COUNT.block.mem = 0;
	COUNT.null_p = 0;
	COUNT.overwrite = 0;
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
	INIT_LIST_HEAD(&full_invalid_block_pool);

	BIT = (_BIT *)malloc(sizeof(_BIT) * FREE_BLOCK);

	for (int i = 0; i < FREE_BLOCK; i++) {
		BIT[i].invalid = 0;
		BIT[i].num_partition = 0;
		BIT[i].partition = (int*)malloc(sizeof(int)*PAGE_PER_BLOCK);
		INIT_LIST_HEAD(&BIT[i].b_list);

		list_add(&BIT[i].b_list, &free_block_pool);
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
	
	LIST_INIT_HEAD(&PVB[partition].p_list);

	// start/end를 제외한 가운데 block
	for (int i = 0; i < BLOCK_PER_PARTITION + 1; i++) {
		PVB[partition].block[i] = -1;
	}
}
