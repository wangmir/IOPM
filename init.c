#include "iopm.h"
#include "main.h"
#include "init.h"


void init() {
	
	init_SIT();
	init_CLUSTER();
	init_BIT();
	init_PVB();
	init_COUNT();

	init_partition_pool();
	init_block_pool();

	free_partition = NUMBER_PARTITION;
	free_block = FREE_BLOCK;
	ALLOC_PARTITION = 0;


	victim_page_LPN = (int *)malloc(sizeof(int)*(PARTITION_PER_CLUSTER + 2)*(PAGE_PER_PARTITION));
	victim_page_PPN = (int *)malloc(sizeof(int)*(PARTITION_PER_CLUSTER + 2)*(PAGE_PER_PARTITION));
	victim_partition = (int *)malloc(sizeof(int)*(PARTITION_PER_CLUSTER + 2));

	remove_block_in_partition = (int *)malloc(sizeof(int) * BLOCK_PER_PARTITION);
	victim_cluster_pool = (_cluster **)malloc(sizeof(_cluster)*PAGE_PER_CLUSTER);
	victim_cluster_pool_num = (int *)malloc(sizeof(int) * PAGE_PER_CLUSTER);
	int i;
	for (i = 0; i < PAGE_PER_CLUSTER; i++) {
		victim_cluster_pool[i] = NULL;
		victim_cluster_pool_num[i] = 0;
	}
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

	for (i = 0; i<NUMBER_PARTITION; i++) {

		PVB[i].bitmap = (int *)malloc(sizeof(int) * PAGE_PER_PARTITION_32);

		for (j = 0; j<PAGE_PER_PARTITION_32; j++) {
			PVB[i].bitmap[j] = 0;
		}
		PVB[i].startLPN = -1;
		PVB[i].startPPN = -1;
		PVB[i].endPPN = -1;
		PVB[i].valid = 0;
		PVB[i].block = (int *)malloc(sizeof(int)*(BLOCK_PER_PARTITION + 1));
		PVB[i].blocknum = 0;
		PVB[i].active_flag = 0;
		PVB[i].allocate_free = NULL;

		PVB[i].cluster = (_partition *)malloc(sizeof(_partition));
		PVB[i].cluster->partition_num = i;
		PVB[i].cluster->prev = NULL;
		PVB[i].cluster->next = NULL;

		PVB[i].victim_partition_list = (_partition *)malloc(sizeof(_partition));
		PVB[i].victim_partition_list->partition_num = i;
		PVB[i].victim_partition_list->prev = NULL;
		PVB[i].victim_partition_list->next = NULL;

		PVB[i].victim_partition_flag = 0;

		// start/end를 제외한 가운데 block
		for (j = 0; j < BLOCK_PER_PARTITION + 1; j++) {
			PVB[i].block[j] = -1;
		}

	}
}

void init_BIT() {
	int i;

	BIT = (_BIT *)malloc(sizeof(_BIT) * FREE_BLOCK);

	for (i = 0; i<FREE_BLOCK; i++) {

		BIT[i].invalid = 0;
		BIT[i].num_partition = 0;
		BIT[i].partition = (int*)malloc(sizeof(int)*PAGE_PER_BLOCK);
		BIT[i].alloc_free = NULL;
	}
}

void init_partition_pool() {
	int i;

	allocated_partition_pool = NULL;
	for (i = 0; i < NUMBER_PARTITION; i++) {
		_partition * p = (_partition *)malloc(sizeof(_partition));
		p->partition_num = i;
		p->free_flag = 1;
		if (free_partition_pool == NULL) {
			p->next = p;
			p->prev = p;
			free_partition_pool = p;
		}
		else {
			//_partition *temp = free_partition_pool;
			_partition *prev = free_partition_pool->prev;
			_partition *next = free_partition_pool->next;
			p->next = free_partition_pool;
			free_partition_pool->prev = p;
			p->prev = prev;
			prev->next = p;
		}
		PVB[i].allocate_free = p;
	}
}

void init_block_pool() {

	int i;
	full_invalid_block_num = 0;
	full_invalid_block_pool = NULL;

	allocated_block_pool = NULL;
	for (i = 0; i < FREE_BLOCK;  i++) {
		_block * b = (_block *)malloc(sizeof(_block));
		b->block_num = i;
		b->free_flag = 1;
		if (free_block_pool == NULL) {
			b->next = b;
			b->prev = b;
			free_block_pool = b;
		}
		else {
			_block *prev = free_block_pool->prev;
			_block *next = free_block_pool->next;
			b->next = free_block_pool;
			free_block_pool->prev = b;
			b->prev = prev;
			prev->next = b;
		}
		BIT[i].alloc_free = b;
	}
}

void init_SIT() {
	int i;

	SIT = (_SIT *)malloc(sizeof(_SIT) * NUMBER_STREAM);

	for (i = 0; i<NUMBER_STREAM; i++) {
		SIT[i].recentPartition = -1;
		SIT[i].recentLPN = -1;
		SIT[i].recentPPN = -1;
	}
}

void init_CLUSTER() {
	int i;

	CLUSTER = (_CLUSTER *)malloc(sizeof(_CLUSTER) * NUMBER_CLUSTER);

	for (i = 0; i<NUMBER_CLUSTER; i++) {
		CLUSTER[i].valid = 0;
		CLUSTER[i].num_partition = 0;
		CLUSTER[i].victim_valid = 0;
		CLUSTER[i].victim_partition_num = 0;
		CLUSTER[i].cluster = (_cluster *)malloc(sizeof(_cluster));
		CLUSTER[i].cluster->cluster_num = i;
		CLUSTER[i].cluster->prev = NULL;
		CLUSTER[i].cluster->next = NULL;
		CLUSTER[i].partition_list = NULL;
		CLUSTER[i].victim_partition_list = NULL;
	}
}


void init_victim_cluster() {


}

void init_Partition(int partition) {

	int j;

	for (j = 0; j<PAGE_PER_PARTITION_32; j++) {
		PVB[partition].bitmap[j] = 0;
	}

	PVB[partition].startLPN = -1;
	PVB[partition].startPPN = -1;
	PVB[partition].endPPN = -1;
	PVB[partition].valid = 0;
	PVB[partition].blocknum = 0;
	PVB[partition].active_flag = 0;
	PVB[partition].allocate_free->free_flag = 1;
	PVB[partition].victim_partition_flag = 0;
	PVB[partition].cluster->next = NULL;
	PVB[partition].cluster->prev = NULL;
	PVB[partition].victim_partition_list->next = NULL;
	PVB[partition].victim_partition_list->prev = NULL;
	// start/end를 제외한 가운데 block
	for (j = 0; j < BLOCK_PER_PARTITION + 1; j++) {
		PVB[partition].block[j] = -1;
	}
}
