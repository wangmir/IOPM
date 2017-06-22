#include "main.h"
#include "iopm.h"
#include "iopm_map.h"
#include "init.h"
#include "error.h"
#include "list.h"
#include "nand.h"

/*
	select feasible stream for the write, and return # of stream
*/
int select_stream(int LPN, int IO_type) {
	
	_SIT *psit = NULL;
	int cluster = CLUSTER_FROM_LPN(LPN);

	// find a fitting stream first at the active stream list
	list_for_each_entry(_SIT, psit, &active_stream_pool, SIT_list) {
		
		// because this is active pool, SIT should have its LPN
		assert(psit->recentLPN != -1);
		
		if (psit->recentLPN < LPN && CLUSTER_FROM_LPN(psit->recentLPN) == cluster) {
			// fitting stream get!
			goto result;
		}
	}

	psit = NULL;

	// there are no available stream, find plan B
	if (!list_empty(&free_stream_pool)) { 
		psit = list_first_entry(&free_stream_pool, _SIT, SIT_list);
		goto result;
	}
	// select LRU stream as a victim
	psit = list_last_entry(&active_stream_pool, _SIT, SIT_list);

result:
	list_del(&psit->SIT_list);
	list_add(&psit->SIT_list, &active_stream_pool);
	return psit->stream_num;
}

/*
	get a new partition structure
*/
int allocate_partition(int flag) {
	
	assert(!list_empty(&free_partition_pool));

	_PVB *ppvb = list_first_entry(&free_partition_pool, _PVB, p_list);
	list_del(&ppvb->p_list);
	free_partition--;

	init_Partition(ppvb->partition_num);

	return ppvb->partition_num;
}

/*
	link the allocated partition into the cluster
*/
void insert_partition_into_cluster(int cluster, int partition) {
	list_add(&PVB[partition].p_list, &CLUSTER[cluster].p_list);
	CLUSTER[cluster].num_partition++;
}

void do_gc_if_needed(int IOtype) {
	if (IOtype == IO_WRITE) {
		while (free_partition < PARTITION_LIMIT || free_block < BLOCK_LIMIT) {
			if (free_partition < PARTITION_LIMIT) {
				PartitionGC();
			}
			if (free_block < BLOCK_LIMIT) {
				BlockGC();
			}
		}
	}
}

static void put_partition(int partition) {
	_PVB *ppvb = &PVB[partition];

	list_del(&ppvb->p_list);
	list_add(&ppvb->p_list, &free_partition_pool);
	free_partition++;
    
	init_Partition(partition);
}

void remove_partition_from_cluster(int partition, int IO_type) {

	
	_PVB *ppvb = &PVB[partition];
	int cluster = CLUSTER_FROM_LPN(ppvb->startLPN);
	_CLUSTER *pcluster = &CLUSTER[cluster];
	pcluster->valid -= ppvb->valid;
	pcluster->num_partition--;
		
	// partition free
    put_partition(partition);
}

void free_full_invalid_partition(int partition) {

	COUNT.null_partition++;
	_PVB *ppvb = &PVB[partition];

	int block_num = 0;

	// if partition is in active state, close the partition and refresh the stream
	if (ppvb->active_flag == 1)
		for (int i = 0; i < NUMBER_STREAM; i++)
			if (SIT[i].activePartition == partition)
				close_partition(i, IO_WRITE);

	// remove partition link from associated blocks
	unlink_partition_from_BIT(partition);

	// remove partition link from associated cluster
	remove_partition_from_cluster(partition, IO_WRITE);	
}

void link_partition_to_BIT(int partition, int block) {

	_BIT *pbit = &BIT[block];

	if (list_empty(&free_r_plist)) {
		// ERROR: there must be free entries
		printf("ERR: free_r_plist is empty\n");
		getchar();
	}

	_LIST_MAP *r_map = list_first_entry(&free_r_plist, _LIST_MAP, list);

	list_del(&r_map->list);
	list_add(&r_map->list, &pbit->linked_partition);

	pbit->num_partition++;
}

void unlink_partition_from_BIT(int partition) {

	_PVB *ppvb = &PVB[partition];

	for (int i = 0; ppvb->blocknum; i++) {

		if (ppvb->block[i] == PVB_BLOCK_RECLAIMED)
			continue;

		_BIT *pbit = &BIT[ppvb->block[i]];

		_LIST_MAP *plist = NULL;
		
		list_for_each_entry(_LIST_MAP, plist, &pbit->linked_partition, list) {

			if (plist->value == partition) {
				list_del(&plist->list);
				list_add(&plist->list, &free_r_plist);
				break;
			}
		}

		pbit->num_partition--;
	}
}

int find_bitmap(int partition, int offset, int read) {
	// return 1 : o return 0 : x
	int n = offset / 32;
	MEM_COUNT_IO(read);
	int off = offset % 32;
	MEM_COUNT_IO(read);
	int off2 = 31 - off;
	int new = PVB[partition].bitmap[n];
	MEM_COUNT_IO(read);
//	MEM

	if (new == 0) {
		MEM_COUNT_IO(read);
		return 0;
	}

	if (((new >> off2) & 0x00000001) == 1) {
		MEM_COUNT_IO(read);
		return 1;
	}
	else {
		return 0;
	}
}

void insert_bitmap(int partition, int offset) {

	int n = offset / 32;
	int off = offset % 32;
	int off2 = 31 - off;
	unsigned int add = 1 << off2;

	PVB[partition].bitmap[n] = PVB[partition].bitmap[n] + add;
}

int bitCount_u32_HammingWeight(unsigned int n, int IO_type) {
	int m1 = 0x55555555; //01 01 01 01...
	int m2 = 0x33333333; //00 11 00 11 ...
	int m3 = 0x0f0f0f0f; //00 00 11 11
	int h1 = 0x01010101; //00 00 00 01
	n = n - ((n >> 1) & m1);
	MEM_COUNT_IO(IO_type);
	MEM_COUNT_IO(IO_type);
	MEM_COUNT_IO(IO_type);
	n = (n & m2) + ((n >> 2) & m2);
	MEM_COUNT_IO(IO_type);
	MEM_COUNT_IO(IO_type);
	MEM_COUNT_IO(IO_type);
	return (((n + (n >> 4)) & m3) * h1) >> 24;

}


int get_offset_fast(unsigned int *a, int n, int IO_type) {
	int m1 = 0x55555555; //01 01 01 01...
	int m2 = 0x33333333; //00 11 00 11 ...
	int m3 = 0x0f0f0f0f; //00 00 11 11
	int h1 = 0x01010101; //00 00 00 01

	int loop = n / 32;
	MEM_COUNT_IO(IO_type);
	int off = n % 32;
	MEM_COUNT_IO(IO_type);
	int i;
	int count = 0;

	for (i = 0; i<loop; i++) {
		MEM_COUNT_IO(IO_type);
		count = count + bitCount_u32_HammingWeight(a[i], IO_type);
	}

	if (off != 0) {
		unsigned int cha = a[loop] >> (32 - off);
		MEM_COUNT_IO(IO_type);
		MEM_COUNT_IO(IO_type);
		count = count + bitCount_u32_HammingWeight(cha, IO_type);
		MEM_COUNT_IO(IO_type);
	}
	return count;
}

int allocate_block() {

	assert(!list_empty(&free_block_pool));

	_BIT *pbit = NULL;

	pbit = list_first_entry(&free_block_pool, _BIT, b_list);
	assert(pbit->free_flag == 1);

	pbit->free_flag = 0;

	list_add(&pbit->b_list, &allocated_block_pool);

	free_block--;
	return pbit->block_num;
}

void swap(int* a, int* b) {
	int tmp = *a;
	*a = *b;
	*b = tmp;
}

int LPN2Partition(int LPN, int IO_type) {

	int cluster = CLUSTER_FROM_LPN(LPN);

	_CLUSTER *cl = &CLUSTER[cluster];
	_PVB *ppvb = NULL;

	list_for_each_entry(_PVB, ppvb, &cl->p_list, p_list) {

		int startLPN = ppvb->startLPN;
		int offset = LPN - startLPN;

		if (offset >= 0) {
			if (find_bitmap(ppvb->partition_num, offset, IO_type) == 1) {
				return ppvb->partition_num;
			}
		}
	}

	// there are no valid page for LPN
	return -1;
}

int Partition2PPN(int LPN, int partition, int IO_type) {

	int PPN = -1;

	// Find the logical offset
	int log_offset = logical_offset(partition, LPN, IO_type);
	
	// find the physical offset(자기 앞까지의 1의 개수)
	int phy_offset = physical_offset(partition, log_offset, IO_type);

	int start_off = PVB[partition].startPPN % PAGE_PER_BLOCK;
	MEM_COUNT_IO(IO_type);
	int start_off_off = PAGE_PER_BLOCK - start_off;
	MEM_COUNT_IO(IO_type);
	int block_num = -1;
	int offset = -1;
	int last_block = BLOCK_PER_PARTITION + 1;
	int off;
	int block;
	int flag = -1;

	if (phy_offset >= start_off_off) {
		MEM_COUNT_IO(IO_type);
		flag = 0;
		off = phy_offset - start_off_off;
		MEM_COUNT_IO(IO_type);
		block = off / PAGE_PER_BLOCK;		// start를 제외. 0이면 그 다음 block
		MEM_COUNT_IO(IO_type);
		offset = off%PAGE_PER_BLOCK;
		MEM_COUNT_IO(IO_type);
		PPN = PVB[partition].block[block+1] * PAGE_PER_BLOCK + offset;
		MEM_COUNT_IO(IO_type);
	}
	else {
		// first block
		flag = 1;
		block_num = PVB[partition].startPPN / PAGE_PER_BLOCK;
		MEM_COUNT_IO(IO_type);
		offset = phy_offset;
		PPN = PVB[partition].startPPN + offset;
		MEM_COUNT_IO(IO_type);
	}
	error_LPN_PPN(LPN, PPN);


	return PPN;
}

// count 1's algorithm
int physical_offset(int partition, int log_offset, int read) {

	int phy_offset = 0;

	return get_offset_fast(PVB[partition].bitmap, log_offset, read);

}

int logical_offset(int partition, int LPN, int IO_type) {
	int log_offset = -1;
	MEM_COUNT_IO(IO_type);
	int startLPN = PVB[partition].startLPN;
	MEM_COUNT_IO(IO_type);
	log_offset = LPN - startLPN;
	MEM_COUNT_IO(IO_type);
	
	if (log_offset > PAGE_PER_PARTITION) {
		printf("ERROR : logical_offset > partition size\n");
		exit(1);
	}

	return log_offset;
}


void two_bitmap(unsigned int n) {
	int i = 31;
	for (i; i >= 0; --i) {
		printf("%d", (n >> i) & 1);
	}
	printf("\n");
}

void quickSort(int arr[], int b[], int left, int right) {
	int i = left, j = right;
	int pivot = arr[(left + right) / 2];
	int temp;
	do {
		while (arr[i] < pivot)
			i++;
		while (arr[j] > pivot)
			j--;
		if (i <= j) {
			temp = arr[i];
			arr[i] = arr[j];
			arr[j] = temp;
			temp = b[i];
			b[i] = b[j];
			b[j] = temp;

			i++;
			j--;
		}
	} while (i <= j);

	/* recursion */
	if (left < j)
		quickSort(arr, b, left, j);

	if (i < right)
		quickSort(arr, b, i, right);
}

void close_partition(int stream, int IO_type) {

	_SIT *psit = &SIT[stream];
	int partition = psit->activePartition;
	_PVB *ppvb = &PVB[partition];

	// close the partition
	ppvb->endPPN = psit->recentPPN;
	ppvb->active_flag = 0;

	// refresh active partition for the stream
	psit->activePartition = -1;
	psit->recentLPN = -1;
}

void put_block(int block, int flag) {

    _BIT *pbit = &BIT[block];
    
    pbit->free_flag = 1;
    pbit->invalid = 0;
    pbit->num_partition = 0;

    list_del(&pbit->b_list);
    list_add(&pbit->b_list, &free_block_pool);

	free_block++;
}

void unlink_block_from_PVB(int block) {

	_BIT *pbit = &BIT[block];
	_LIST_MAP *plist = NULL;

	list_for_each_entry(_LIST_MAP, plist, &pbit->linked_partition, list) {

		int partition = plist->value;

		for (int i = 0; i < PVB[partition].blocknum; i++) {
			if (PVB[partition].block[i] == block) 
				PVB[partition].block[i] = PVB_BLOCK_RECLAIMED;
		}
	}
}

int select_victim_block() {

	_BIT *pbit = NULL;

	int victim_block, max_invalid = 0;

	list_for_each_entry(_BIT, pbit, &allocated_block_pool, b_list) {
		if (pbit->is_active)
			continue;

		if (pbit->invalid > max_invalid)
			victim_block = pbit->block_num;
	}

	return victim_block;
}

int select_victim_cluster() {

	int victim_cluster = -1;
	int victim_mean_valid = PAGE_PER_PARTITION;

	for (int i = 0; i < NUMBER_CLUSTER; i++) {

		_CLUSTER *pcluster = &CLUSTER[i];

		// at least 2 partitions are needed to handle partition GC
		if (pcluster->num_partition < 2)
			continue;

		int mean_valid = pcluster->valid / pcluster->num_partition;

		if (victim_mean_valid > mean_valid)
			victim_cluster = pcluster->cluster_num;
	}

	if (victim_cluster == -1) {
		printf("ERR: there are no available cluster for partition GC\n");
		getchar();
	}

	return victim_cluster;
}

void select_victim_partition(int cluster, int *p_array) {

	_CLUSTER *pcluster = &CLUSTER[cluster];
	_PVB *ppvb = NULL;
	int valid_array[MAX_NUM_PARTITION_PGC];

	for (int i = 0; i < MAX_NUM_PARTITION_PGC; i++)
		p_array[i] = -1;

	// pick <MAX_NUM_PARTITION_PGC> partitions, which have minimum number of valid pages, from victim cluster
	list_for_each_entry(_PVB, ppvb, &pcluster->p_list, p_list) {

		// if there are enough partition to handle the pgc, then exclude the active partition
		if (pcluster->num_partition > 2 && ppvb->active_flag)
			continue;

		for (int i = 0; i < MAX_NUM_PARTITION_PGC; i++) {

			if (p_array[i] == -1) {
				p_array[i] = ppvb->partition_num;
				valid_array[i] = ppvb->valid;
				break;
			}

			if (valid_array[i] > ppvb->valid) {

				for (int j = MAX_NUM_PARTITION_PGC - 1; j == i; j--) {
					p_array[j + 1] = p_array[j];
					valid_array[j + 1] = valid_array[j];
				}
				p_array[i] = ppvb->partition_num;
				valid_array[i] = ppvb->valid;
			}
		}
	}
}
