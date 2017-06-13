#include "main.h"
#include "iopm.h"
#include "iopm_map.h"
#include "init.h"
#include "error.h"
#include "list.h"

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
	
	int j;

	_partition *alloc = free_partition_pool;

	if (free_partition_pool == NULL) {
		printf("\nERROR : No free partition to allocate\n");
		exit(1);
	}
	_partition *prev = free_partition_pool->prev;
	_partition *next = free_partition_pool->next;
	prev->next = next;
	next->prev = prev;
	free_partition_pool = free_partition_pool->next;

	// move the target partition into allocated_partition list
	if (allocated_partition_pool == NULL) {
		allocated_partition_pool = alloc;
		alloc->prev = alloc;
		alloc->next = alloc;
	}
	else {
		// insert into last
		_partition *prev = allocated_partition_pool->prev;
		prev->next = alloc;
		alloc->next = allocated_partition_pool;
		allocated_partition_pool->prev = alloc;
		alloc->prev = prev;

	}
	int partition_num = alloc->partition_num;

	//// Init the partition
	//for (j = 0; j<PAGE_PER_PARTITION_32; j++) {
	//	PVB[partition_num].bitmap[j] = 0;
	//}
	memset(PVB[partition_num].bitmap, 0x00, sizeof(unsigned int) * PAGE_PER_PARTITION_32);

	PVB[partition_num].valid = 0;
	PVB[partition_num].startLPN = -1;
	PVB[partition_num].startPPN = -1;
	PVB[partition_num].endPPN = -1;
	PVB[partition_num].blocknum = 0;
	PVB[partition_num].active_flag = 1;
	PVB[partition_num].allocate_free->free_flag = 0;

	for (j = 0; j < BLOCK_PER_PARTITION + 1; j++) {
		PVB[partition_num].block[j] = -1;
	}
	
	free_partition--;

	return partition_num;
}

/*
	link the allocated partition into the cluster
*/
int allocate_partition_to_cluster(int LPN, int partition) {

	// insert
	int cluster = LPN / PAGE_PER_CLUSTER;
	// insert into first.
	if (CLUSTER[cluster].partition_list == NULL) {
		CLUSTER[cluster].partition_list = PVB[partition].cluster;
		PVB[partition].cluster->prev = PVB[partition].cluster;
		PVB[partition].cluster->next = PVB[partition].cluster;
	}
	else {
		_partition *prev = CLUSTER[cluster].partition_list->prev;
		PVB[partition].cluster->partition_num = partition;
		PVB[partition].cluster->next = CLUSTER[cluster].partition_list;
		PVB[partition].cluster->prev = prev;
		prev->next = PVB[partition].cluster;
		CLUSTER[cluster].partition_list->prev = PVB[partition].cluster;
		CLUSTER[cluster].partition_list = CLUSTER[cluster].partition_list->prev;
	}

	return cluster;
}

void check_partition_gc(int IOtype) {
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

void remove_partition_from_cluster(int victim_partition, int victim_cluster, int IO_type) {

	CLUSTER[victim_cluster].valid = CLUSTER[victim_cluster].valid - PVB[victim_partition].valid;
	CLUSTER[victim_cluster].num_partition--;

	//CLUSTER[victim_cluster].victim_valid = CLUSTER[victim_cluster].victim_valid - PVB[victim_partition].valid;
	//CLUSTER[victim_cluster].victim_partition_num--;
		
	// partition free
	allocate2free_partition_pool(victim_partition);
	
	remove_partition_in_cluster(victim_partition, victim_cluster, IO_type);
}

void free_full_invalid_partition(int partition) {
	COUNT.null_p++;
	if (partition == 2157) {
		printf("GC ->");
	}

	int i;
	int block_num = 0;
	int cluster = PVB[partition].startLPN / PAGE_PER_CLUSTER;

	if (PVB[partition].active_flag == 1) {
		for (i = 0; i < NUMBER_STREAM; i++) {
			if (SIT[i].activePartition == partition) {
				close_partition(i, IO_WRITE);
				SIT[i].activePartition = -1;
				SIT[i].recentLPN = -1;
			}
		}
	}
	

	// remove partition in block
	for (i = 0; i < PVB[partition].blocknum; i++) {
		if (PVB[partition].block[i] != -2) {
			remove_block_in_partition[block_num] = PVB[partition].block[i];
			block_num++;
		}
	}

	

	// remove blocks in partition(PVB)
	remove_partition_in_block(partition, remove_block_in_partition, block_num);
	// remove in cluster
	
	remove_partition_in_cluster(partition, cluster, IO_WRITE);	
	CLUSTER[cluster].num_partition--;

	allocate2free_partition_pool(partition);
	free_partition++;
	// init partition
	init_Partition(partition);
}

void allocate2free_partition_pool(int victim_partition) {	
	_partition *temp = PVB[victim_partition].allocate_free;

	if (PVB[victim_partition].allocate_free->partition_num == allocated_partition_pool->partition_num) {
		_partition * prev = temp->prev;
		_partition * next = temp->next;
		prev->next = next;
		next->prev = prev;
		allocated_partition_pool = next;
	}
	else {
		_partition * prev = temp->prev;
		_partition * next = temp->next;
		prev->next = next;
		next->prev = prev;
		//allocated_partition_pool = next;
	}

	_partition *prev2 = free_partition_pool->prev;
	_partition *next2 = free_partition_pool->next;
	prev2->next = temp;
	temp->prev = prev2;
	temp->next = free_partition_pool;
	free_partition_pool->prev = temp;

}

void remove_partition_in_block(int partition, int *block, int block_num) {
	int i = 0;
	while (i < block_num) {
		// remove partition from block
		//BIT[block[i]].num_partition--;
		int block_num = block[i];
		int j;
		for (j = 0; j < BIT[block_num].num_partition; j++) {
			if (BIT[block_num].partition[j] == partition) {
				int k;
				for (k = j; k < BIT[block_num].num_partition - 1; k++) {
					BIT[block_num].partition[k] = BIT[block_num].partition[k + 1];
				}
				BIT[block_num].num_partition--;
				break;
			}
		}
		i++;
	}
		/*_partition_in_block *temp = BIT[block_num].partition;
		_partition_in_block *temp_b = NULL;

		while (temp != NULL) {
			if (temp->partition_num == partition) {
				if (temp->partition_num == BIT[block_num].partition->partition_num) {
					BIT[block_num].partition = BIT[block_num].partition->next;
					BIT[block_num].num_partition--;
					break;
				}
				else {
					temp_b->next = temp->next;
					free(temp);
					BIT[block_num].num_partition--;
					break;
				}				
			}
			temp_b = temp;
			temp = temp->next;

		}
		i++;*/
	//}

}

void remove_partition_in_cluster(int partition, int cluster, int IO_type) {


	//error_cluster_vicitm_valid(cluster);
//	error_cluster_vicitm_valid_increase(cluster);
//	error_victim_partition_flag(cluster);

	//cluster_count(cluster);
	//cluster_vicitm_valid(cluster);
	//victim_partition_flag_check(cluster);
	//check_PVB(cluster);
	//cluster_vicitm_valid_increase(cluster);
	//if(IO_type == IO_WRITE)
		//victim_cluster_pool_cluster(cluster);

	// remove from normal_partition_list
	_partition * partition_prev = PVB[partition].cluster->prev;
	_partition * partition_next = PVB[partition].cluster->next;

	partition_prev->next = partition_next;
	partition_next->prev = partition_prev;

	if (partition == CLUSTER[cluster].partition_list->partition_num) {
		CLUSTER[cluster].partition_list = partition_next;
	}

	// remove from victim_partition_list
	int i; int count = 0;
	_partition *temp = CLUSTER[cluster].victim_partition_list;
	for (i = 0; i < CLUSTER[cluster].victim_partition_num; i++) {
		if (temp->partition_num == partition) {
			break;
		}
		count++;
		temp = temp->next;
	}
	
	// remove from victim_partition_list
	_partition *victim_prev = PVB[partition].victim_partition_list->prev;
	_partition *victim_next = PVB[partition].victim_partition_list->next;

	if (CLUSTER[cluster].victim_partition_num == 1) {
		CLUSTER[cluster].victim_partition_list = NULL;
	}
	else {

		victim_prev->next = victim_next;
		victim_next->prev = victim_prev;

		if (partition == CLUSTER[cluster].victim_partition_list->partition_num) {
			CLUSTER[cluster].victim_partition_list = victim_next;
		}
	}
	

	CLUSTER[cluster].victim_partition_num--;

	// update the valid count
	if (count < PARTITION_PER_CLUSTER + 2) {

		//remove from victim_partition_pool
		if (CLUSTER[cluster].victim_valid < PAGE_PER_CLUSTER) {

			_cluster *cluster_next = CLUSTER[cluster].cluster->next;
			_cluster *cluster_prev = CLUSTER[cluster].cluster->prev;

			if (cluster_next != NULL || cluster_prev != NULL) {

				if (cluster_next->cluster_num == cluster ) {
					victim_cluster_pool[CLUSTER[cluster].victim_valid] = NULL;
				}
				else {
					cluster_next->prev = cluster_prev;
					cluster_prev->next = cluster_next;
				}
				victim_cluster_pool_num[CLUSTER[cluster].victim_valid]--;

			}
			
			if (victim_cluster_pool[CLUSTER[cluster].victim_valid] != NULL) {
				if (CLUSTER[cluster].cluster->cluster_num == victim_cluster_pool[CLUSTER[cluster].victim_valid]->cluster_num) {
					victim_cluster_pool[CLUSTER[cluster].victim_valid] = cluster_next;
				}
			}
		}
		//cluster_remove_check(cluster, CLUSTER[cluster].victim_valid);

		// remove
		CLUSTER[cluster].victim_valid = CLUSTER[cluster].victim_valid - PVB[partition].valid;

		if (CLUSTER[cluster].victim_partition_num < PARTITION_PER_CLUSTER + 2) {
		}
		else {
			int j = 0;
			_partition *t = CLUSTER[cluster].victim_partition_list;
			while (j < PARTITION_PER_CLUSTER + 1) {
				t = t->next;
				j++;
			}
			CLUSTER[cluster].victim_valid = CLUSTER[cluster].victim_valid + PVB[t->partition_num].valid;
			PVB[t->partition_num].victim_partition_flag = 1;
		}


		// insert
		if (CLUSTER[cluster].victim_valid < PAGE_PER_CLUSTER) {
			if (victim_cluster_pool[CLUSTER[cluster].victim_valid] == NULL) {
				victim_cluster_pool[CLUSTER[cluster].victim_valid] = CLUSTER[cluster].cluster;
				CLUSTER[cluster].cluster->prev = CLUSTER[cluster].cluster;
				CLUSTER[cluster].cluster->next = CLUSTER[cluster].cluster;
			}
			else {
				_cluster *new_cluster_prev = victim_cluster_pool[CLUSTER[cluster].victim_valid]->prev;
				new_cluster_prev->next = CLUSTER[cluster].cluster;
				CLUSTER[cluster].cluster->next = victim_cluster_pool[CLUSTER[cluster].victim_valid];
				victim_cluster_pool[CLUSTER[cluster].victim_valid]->prev = CLUSTER[cluster].cluster;
				CLUSTER[cluster].cluster->prev = new_cluster_prev;

			}
			victim_cluster_pool_num[CLUSTER[cluster].victim_valid]++;

		}

	}


	//error_cluster_vicitm_valid(cluster);
	//error_cluster_vicitm_valid_increase(cluster);
	//error_victim_partition_flag(cluster);


	//cluster_count(cluster);
	//cluster_vicitm_valid(cluster);
	//if(IO_type == IO_WRITE)
	//	victim_partition_flag_check(cluster);
	//check_PVB(cluster);
	//cluster_vicitm_valid_increase(cluster);
	//if (IO_type == IO_WRITE)
		//victim_cluster_pool_cluster(cluster);

}

#if 0 
void remove_partition_in_cluster(int partition, int cluster) {

	_partition * partition_prev = PVB[partition].cluster->prev;
	_partition * partition_next = PVB[partition].cluster->next;

	partition_prev->next = partition_next;
	partition_next->prev = partition_prev;

	if (partition == CLUSTER[cluster].partition_list->partition_num) {
		CLUSTER[cluster].partition_list = CLUSTER[cluster].partition_list->next;
	}

	// if partition is victim_partition, update the victim_valid and victim_partition_num
	// 지우고자 하는 partition이 victim partition에 속해있다. 
	if (PVB[partition].victim_partition_flag == 1) {

		if (CLUSTER[cluster].victim_valid < PAGE_PER_CLUSTER) {
			// remove from victim_cluster_pool
			_cluster *cluster_prev = CLUSTER[cluster].cluster->prev;
			_cluster *cluster_next = CLUSTER[cluster].cluster->next;

			cluster_prev->next = cluster_next;
			cluster_next->prev = cluster_prev;

			if (cluster == victim_cluster_pool[CLUSTER[cluster].victim_valid]->cluster_num) {
				victim_cluster_pool[CLUSTER[cluster].victim_valid] = victim_cluster_pool[CLUSTER[cluster].victim_valid]->next;
			}
		}
		

		int count = 0;
		_partition *temp = CLUSTER[cluster].partition_list;
		CLUSTER[cluster].victim_partition_num--;
		if (CLUSTER[cluster].victim_partition_num > PARTITION_PER_CLUSTER + 1) {
			while (count < PARTITION_PER_CLUSTER + 2) {
				if (PVB[temp->partition_num].active_flag == 0) {
					count++;
				}
				temp = temp->next;
			}
			CLUSTER[cluster].victim_valid = CLUSTER[cluster].victim_valid - PVB[partition].valid + PVB[temp->partition_num].valid;
		}
		else {
			CLUSTER[cluster].victim_valid = CLUSTER[cluster].victim_valid - PVB[partition].valid;
		}

		if (CLUSTER[cluster].victim_valid < PAGE_PER_CLUSTER) {

			// insert into new victim_cluster_pool
			int victim_valid = CLUSTER[cluster].victim_valid;

			_cluster *new_cluster_prev = victim_cluster_pool[victim_valid]->prev;

			new_cluster_prev->next = CLUSTER[cluster].cluster;
			CLUSTER[cluster].cluster->prev = new_cluster_prev;
			CLUSTER[cluster].cluster->next = victim_cluster_pool[victim_valid];
			victim_cluster_pool[victim_valid]->prev = CLUSTER[cluster].cluster;
		}
	}

}
#endif

void insert_block_to_PVB(int partition, int block) {
	PVB[partition].block[PVB[partition].blocknum] = block;
	PVB[partition].blocknum++;
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

	check_free_block();
	//error_free_block_pool();

	_block *alloc = free_block_pool;
	assert(free_block_pool->free_flag == 1);

	alloc->free_flag = 0;
	_block *prev_ = free_block_pool->prev;
	_block *next_ = free_block_pool->next;
	prev_->next = next_;
	next_->prev = prev_;
	free_block_pool = next_;
	assert(free_block_pool->free_flag == 1);

	if (allocated_block_pool == NULL) {
		allocated_block_pool = alloc;
		alloc->next = alloc;
		alloc->prev = alloc;
		assert(free_block_pool->free_flag == 1);
	}
	else {
		_block *prev = allocated_block_pool->prev;
		_block *next = allocated_block_pool->next;
		alloc->next = allocated_block_pool;
		alloc->prev = prev;
		prev->next = alloc;
		allocated_block_pool->prev = alloc;
		assert(free_block_pool->free_flag == 1);
	}
	
	free_block--;
	
	return alloc->block_num;
}




void swap(int* a, int* b) {
	int tmp = *a;
	*a = *b;
	*b = tmp;
}

void quick_sort(int * array_lpn, int * array_ppn, int start, int end) {

	if (start >= end) return;

	int mid = (start + end) / 2;
	int pivot = array_lpn[mid];

	swap(&array_lpn[start], &array_lpn[mid]);
	swap(&array_ppn[start], &array_ppn[mid]);

	int p = start + 1, q = end;

	while (1) {
		while (array_lpn[p] <= pivot && p <= end) {
			p++;
		}
		while (array_lpn[q]>pivot && q >= 0) {
			q--;
		}

		if (p>q) break;

		swap(&array_lpn[p], &array_lpn[q]);
		swap(&array_ppn[p], &array_ppn[q]);
	}

	swap(&array_lpn[start], &array_lpn[q]);
	swap(&array_ppn[start], &array_ppn[q]);

	quick_sort(array_lpn, array_ppn, start, q - 1);
	quick_sort(array_lpn, array_ppn, q + 1, end);

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

int LPN2Partition_limit(int LPN, int victim, int IO_type) {
	// variables
	int partition_return = -1;
	int i;

	//_partition_in_block *temp = BIT[victim].partition;

	//while (temp != NULL) {
	for(i=0;i<BIT[victim].num_partition;i++){
		//int partition = temp->partition_num;
		int partition = BIT[victim].partition[i];
		int startLPN = PVB[partition].startLPN;
		int offset = LPN - startLPN;
		
		if (offset >= 0 && offset < (PAGE_PER_PARTITION)) {
			if (find_bitmap(partition, offset, IO_type) == 1) {
				if (partition_return != -1) {
					if (partition_order_compare(partition, partition_return, IO_type) && (PVB[partition].valid != 0)) {
						partition_return = partition;
					}
				}
				else {
					if (PVB[partition].valid != 0) {
						partition_return = partition;
					}
				}
			}
		}
		//temp = temp->next;
	}

	return partition_return;
}

int partition_order_compare(int a, int b, int IO_type) {
	// return 1 if a가 b보다 최신일때 
	
	_partition *temp = allocated_partition_pool;

	if (temp->partition_num == a) {
		MEM_COUNT_IO(IO_type);
		return 0;
	}
	else if (temp->partition_num == b) {
		MEM_COUNT_IO(IO_type);
		return 1;
	}
	else {
		//temp = temp->next;
		temp = temp->prev;
		while (temp->partition_num != allocated_partition_pool->partition_num) {
			MEM_COUNT_IO(IO_type);
			if (temp->partition_num == a) {
				return 1;
			}
			else if (temp->partition_num == b) {
				return 0;
			}
			temp = temp->prev;
		}
	}
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

	if (COUNT.write == 7601962 && COUNT.partition.gc_write == 23136435) {
		int a = 1;
	}

	//error_total_cluster();
	// close partition
	int target_partition = SIT[stream].activePartition;
	PVB[target_partition].endPPN = SIT[stream].recentPPN;
	PVB[target_partition].active_flag = 0;
	int cluster = PVB[target_partition].startLPN / PAGE_PER_CLUSTER;

	//error_cluster_vicitm_valid(cluster);
	//error_cluster_vicitm_valid_increase(cluster);
	//error_victim_partition_flag(cluster);

	int debug_flag = 0;
	int prev_victim_valid = CLUSTER[cluster].victim_valid;
	int count = 0;
	int move_flag = 0;
	//printf("%d ->", target_partition);
	/*cluster_count(cluster);
	cluster_vicitm_valid(cluster);
	victim_partition_flag_check(cluster);
	check_PVB(cluster);
	cluster_vicitm_valid_increase(cluster);*/
	//if(IO_type == IO_WRITE)
	//victim_cluster_pool_cluster(cluster);
	
	// 0. change the cluster's victim valid pool
	// remove from the victim_cluster_pool
	if (CLUSTER[cluster].victim_valid < PAGE_PER_CLUSTER) {

		_cluster *cluster_next = CLUSTER[cluster].cluster->next;
		_cluster *cluster_prev = CLUSTER[cluster].cluster->prev;

		if (cluster_next != NULL || cluster_prev != NULL) {

			if (cluster_next->cluster_num == cluster) {
				victim_cluster_pool[CLUSTER[cluster].victim_valid] = NULL;
			}
			else {
				cluster_next->prev = cluster_prev;
				cluster_prev->next = cluster_next;
			}
			victim_cluster_pool_num[CLUSTER[cluster].victim_valid]--;

		}

		if (victim_cluster_pool[CLUSTER[cluster].victim_valid] != NULL) {
			if (CLUSTER[cluster].cluster->cluster_num == victim_cluster_pool[CLUSTER[cluster].victim_valid]->cluster_num) {
				victim_cluster_pool[CLUSTER[cluster].victim_valid] = cluster_next;
			}
		}
	}
	//error_total_cluster();
	//cluster_remove_check(cluster, CLUSTER[cluster].victim_valid);

	// insert partitioin to victim_partition_list
	if (CLUSTER[cluster].victim_partition_list == NULL) {
		// case in NULL
		CLUSTER[cluster].victim_partition_list = PVB[target_partition].victim_partition_list;
		PVB[target_partition].victim_partition_list->prev = PVB[target_partition].victim_partition_list;
		PVB[target_partition].victim_partition_list->next = PVB[target_partition].victim_partition_list;

		PVB[target_partition].victim_partition_flag = 1;

		CLUSTER[cluster].victim_valid = CLUSTER[cluster].victim_valid + PVB[target_partition].valid;
		CLUSTER[cluster].victim_partition_num++;

		assert(CLUSTER[cluster].victim_valid == PVB[target_partition].valid);
		//error_total_cluster();

	}
	else {
		// insert into victim valid partition
		_partition *temp_partition = CLUSTER[cluster].victim_partition_list;
		int i;

		// search the position for target_partition
		for (i = 0; i < CLUSTER[cluster].victim_partition_num; i++) {
			if (PVB[temp_partition->partition_num].valid > PVB[target_partition].valid) {
				break;
			}
			temp_partition = temp_partition->next;
			count++;
		}

		// temp_partition >= valid
		_partition *temp_prev = temp_partition->prev;
		PVB[target_partition].victim_partition_list->next = temp_partition;
		temp_partition->prev = PVB[target_partition].victim_partition_list;
		PVB[target_partition].victim_partition_list->prev = temp_prev;
		temp_prev->next = PVB[target_partition].victim_partition_list;

		//if (temp_partition->partition_num == CLUSTER[cluster].victim_partition_list->partition_num) {
		if(count == 0){
			move_flag = 1;
			CLUSTER[cluster].victim_partition_list = CLUSTER[cluster].victim_partition_list->prev;

			PVB[target_partition].victim_partition_flag = 1;

			if (CLUSTER[cluster].victim_partition_num > PARTITION_PER_CLUSTER + 2) {
				int j = 0;
				_partition *t = CLUSTER[cluster].victim_partition_list;
				while (j < PARTITION_PER_CLUSTER + 2) {
					j++;
					t = t->next;
				}
				PVB[t->partition_num].victim_partition_flag = 0;
			}

		}

		CLUSTER[cluster].victim_partition_num++;
		//cluster_vicitm_valid(cluster);


		// update victim valid
		if (CLUSTER[cluster].victim_partition_num <= PARTITION_PER_CLUSTER + 2) {
			PVB[target_partition].victim_partition_flag = 1;
			CLUSTER[cluster].victim_valid = CLUSTER[cluster].victim_valid + PVB[target_partition].valid;

		}
		else {
			// count  == temp의 위치. 즉 
			if (count <= PARTITION_PER_CLUSTER + 2) {

				debug_flag = 1;
				PVB[target_partition].victim_partition_flag = 1;
				
				int j = 0;
				_partition *t = CLUSTER[cluster].victim_partition_list;
				while (j < PARTITION_PER_CLUSTER + 2) {
					j++;
					t = t->next;
				}
				PVB[target_partition].victim_partition_flag = 1;
				CLUSTER[cluster].victim_valid = CLUSTER[cluster].victim_valid + PVB[target_partition].valid - PVB[t->partition_num].valid;
				PVB[t->partition_num].victim_partition_flag = 0;
			}
		}
		//error_total_cluster();
	}

	// insert the partition victim_partition_pool into last
	if (CLUSTER[cluster].victim_valid < PAGE_PER_CLUSTER) {
		if (victim_cluster_pool[CLUSTER[cluster].victim_valid] == NULL) {
			victim_cluster_pool[CLUSTER[cluster].victim_valid] = CLUSTER[cluster].cluster;
			CLUSTER[cluster].cluster->prev = CLUSTER[cluster].cluster;
			CLUSTER[cluster].cluster->next = CLUSTER[cluster].cluster;
		}
		else {
			_cluster *new_cluster_prev = victim_cluster_pool[CLUSTER[cluster].victim_valid]->prev;
			new_cluster_prev->next = CLUSTER[cluster].cluster;
			CLUSTER[cluster].cluster->next = victim_cluster_pool[CLUSTER[cluster].victim_valid];
			victim_cluster_pool[CLUSTER[cluster].victim_valid]->prev = CLUSTER[cluster].cluster;
			CLUSTER[cluster].cluster->prev = new_cluster_prev;

		}
		victim_cluster_pool_num[CLUSTER[cluster].victim_valid]++;

	}
	error_cluster_vicitm_valid(cluster);
	error_cluster_vicitm_valid_increase(cluster);
	error_victim_partition_flag(cluster);

	//error_total_cluster();

	//cluster_count(cluster);
	//cluster_vicitm_valid(cluster);
	//victim_partition_flag_check(cluster);
	//check_PVB(cluster);
	//cluster_vicitm_valid_increase(cluster);
	//if (IO_type == IO_WRITE)
	//	victim_cluster_pool_cluster(cluster);
	//cluster_vicitm_valid(cluster);
}

#if 0
void close_partition(int stream) {
	// close partition

	int target_partition = SIT[stream].activePartition;
	PVB[target_partition].endPPN = SIT[stream].recentPPN;
	PVB[target_partition].active_flag = 0;
	int cluster = PVB[target_partition].startLPN / PAGE_PER_CLUSTER;

	// CLUSTER
	CLUSTER[cluster].victim_partition_num++;

	// update the cluster with # of valid page
	_partition *temp_partition = CLUSTER[cluster].partition_list;
	int i = 0;
	int count = 0;
	for (i = 0; i < CLUSTER[cluster].num_partition; i++) {
		if (PVB[temp_partition->partition_num].valid > PVB[target_partition].valid) {
			break;
		}
		if (PVB[temp_partition->partition_num].active_flag == 0 && temp_partition->partition_num != target_partition) {
			count++;
		}
		temp_partition = temp_partition->next;
	}


	// remove
	_partition *target_prev = PVB[target_partition].cluster->prev;
	_partition *target_next = PVB[target_partition].cluster->next;
	target_prev->next = target_next;
	target_next->prev = target_prev;

	// insert
	_partition *temp_prev = temp_partition->prev;
	temp_prev->next = PVB[target_partition].cluster;
	PVB[target_partition].cluster->prev = temp_prev;
	PVB[target_partition].cluster->next = temp_partition;
	temp_partition->prev = PVB[target_partition].cluster;

	if (temp_partition == CLUSTER[cluster].partition_list) {
		if (i == 0) {
			CLUSTER[cluster].partition_list = PVB[target_partition].cluster;
		}
	}

	// valid 개수
	int prev_victim_valid = CLUSTER[cluster].victim_valid;
	if (CLUSTER[cluster].victim_partition_num <= PARTITION_PER_CLUSTER + 2) {
		CLUSTER[cluster].victim_valid = CLUSTER[cluster].victim_valid + PVB[target_partition].valid;
		PVB[target_partition].victim_partition_flag = 1;
	}
	else {
		if (count < PARTITION_PER_CLUSTER + 2) {
			// change valid
			int j = 0;
			_partition *t = CLUSTER[cluster].partition_list;
			PVB[target_partition].victim_partition_flag = 1;
			while (j < PARTITION_PER_CLUSTER + 2) {
				j++;
				t = t->next;
			}
			CLUSTER[cluster].victim_valid = CLUSTER[cluster].victim_valid + PVB[target_partition].valid - PVB[t->next->partition_num].valid;
		}
	}


	if (prev_victim_valid != CLUSTER[cluster].victim_valid) {
		// change the cluster list
		_cluster *cluster_next = CLUSTER[cluster].cluster->next;
		_cluster *cluster_prev = CLUSTER[cluster].cluster->prev;

		if (cluster_next != NULL || cluster_prev != NULL) {
			cluster_next->prev = cluster_prev;
			cluster_prev->next = cluster_next;
		}

		// insert into last
		if (CLUSTER[cluster].victim_valid < PAGE_PER_CLUSTER) {
			if (victim_cluster_pool[CLUSTER[cluster].victim_valid] == NULL) {
				victim_cluster_pool[CLUSTER[cluster].victim_valid] = CLUSTER[cluster].cluster;
				CLUSTER[cluster].cluster->prev = CLUSTER[cluster].cluster;
				CLUSTER[cluster].cluster->next = CLUSTER[cluster].cluster;
			}
			else {
				_cluster *new_cluster_prev = victim_cluster_pool[CLUSTER[cluster].victim_valid]->prev;
				new_cluster_prev->next = CLUSTER[cluster].cluster;
				CLUSTER[cluster].cluster->next = victim_cluster_pool[CLUSTER[cluster].victim_valid];
				victim_cluster_pool[CLUSTER[cluster].victim_valid]->prev = CLUSTER[cluster].cluster;
				CLUSTER[cluster].cluster->prev = new_cluster_prev;

			}
		}
	}
}
#endif
// Invalid 시 부르는 함수.
void invalid_page_cluster(int old_LPN, int old_partition) {
	
	//error_total_cluster();
	int cluster = PVB[old_partition].startLPN / PAGE_PER_CLUSTER;
	int count = 0;
	int close_flag = 0;
	int prev_vic;
	int move_flag = 0;
	int flag1 = 0;
	int flag2 = 0;
	int flag3 = 0;

	//error_victim_partition_flag(cluster);
	//error_cluster_vicitm_valid_increase_with_p(cluster, old_partition);

	if (PVB[old_partition].active_flag == 1) {
		int i;
		for (i = 0; i < NUMBER_STREAM; i++) {
			if (SIT[i].activePartition == old_partition) {
				close_partition(i, IO_WRITE);
				close_flag = 1;
				SIT[i].activePartition = -1;
				SIT[i].recentLPN = -1;
			}
		}
	}
	if (close_flag == 0) {
		prev_vic = CLUSTER[cluster].victim_valid;

		// 해당 partition의 위치 변경. 

		int i;
		_partition *temp = CLUSTER[cluster].victim_partition_list;
		//int count = 0;
		for (i = 0; i < CLUSTER[cluster].victim_partition_num; i++) {
			if (PVB[temp->partition_num].valid > PVB[old_partition].valid) {
				break;
			}
			count++;
			temp = temp->next;
		}

		// 위치를 바꾸는게 아니라 밀리게 하자. 
		// temp가 큰것.
		if (temp->partition_num != PVB[old_partition].victim_partition_list->next->partition_num) {
			flag1 = 1;
			// change
			// remove the old partition
			_partition * old_prev = PVB[old_partition].victim_partition_list->prev;
			_partition * old_next = PVB[old_partition].victim_partition_list->next;
			_partition * prev = temp->prev;
			//_partition * next = temp->next;
			if (old_partition == CLUSTER[cluster].victim_partition_list->partition_num) {
				CLUSTER[cluster].victim_partition_list = CLUSTER[cluster].victim_partition_list->next;
			}

			prev->next = PVB[old_partition].victim_partition_list;
			temp->prev = PVB[old_partition].victim_partition_list;
			PVB[old_partition].victim_partition_list->next = temp;
			PVB[old_partition].victim_partition_list->prev = prev;

			old_prev->next = old_next;
			old_next->prev = old_prev;

			//cluster_count(cluster);
			//cluster_vicitm_valid_increase(cluster);

			//if (CLUSTER[cluster].victim_partition_list->partition_num == temp->partition_num) {
			//	if (PVB[temp->partition_num].valid > PVB[old_partition].valid) {
			if (count == 0) {
				move_flag = 1;
				CLUSTER[cluster].victim_partition_list = CLUSTER[cluster].victim_partition_list->prev;
				PVB[old_partition].victim_partition_flag = 1;

				if (CLUSTER[cluster].victim_partition_num > PARTITION_PER_CLUSTER + 2) {
					int j = 0;
					_partition *t = CLUSTER[cluster].victim_partition_list;
					while (j < PARTITION_PER_CLUSTER + 2) {
						j++;
						t = t->next;
					}
					PVB[t->partition_num].victim_partition_flag = 0;
				}		
			}
					//cluster_vicitm_valid_increase(cluster);
			//	}

			//}

			if (PVB[old_partition].victim_partition_flag == 1) {
				if (count < PARTITION_PER_CLUSTER + 2) {
					CLUSTER[cluster].victim_valid--;
				}
			}
			else {
				// old partition이 위치를 바꿔서 temp를 민 경우. 혹은 temp이후의 것을 민 경우. 
				if (count < PARTITION_PER_CLUSTER + 2) {
					flag2 = 1;
					int j = 0;
					_partition *t = CLUSTER[cluster].victim_partition_list;
					while (j < PARTITION_PER_CLUSTER + 2) {
						j++;
						t = t->next;
					}
					PVB[old_partition].victim_partition_flag = 1;
					CLUSTER[cluster].victim_valid = CLUSTER[cluster].victim_valid + PVB[old_partition].valid - PVB[t->partition_num].valid;
					if(CLUSTER[cluster].victim_partition_num > PARTITION_PER_CLUSTER +2)
						PVB[t->partition_num].victim_partition_flag = 0;
				}

			}

		}
		else {
			// No need to move

			if (PVB[old_partition].valid < PVB[CLUSTER[cluster].victim_partition_list->partition_num].valid) {
				CLUSTER[cluster].victim_partition_list = CLUSTER[cluster].victim_partition_list->prev;
				// flag
				flag3 = 1;
				PVB[old_partition].victim_partition_flag = 1;
				//PVB[CLUSTER[cluster].victim_partition_list->partition_num].victim_partition_flag = 0;
				int j = 0;
				_partition *t = CLUSTER[cluster].victim_partition_list;
				while (j < PARTITION_PER_CLUSTER + 2) {
					j++;
					t = t->next;
				}
				PVB[old_partition].victim_partition_flag = 1;
				//CLUSTER[cluster].victim_valid = CLUSTER[cluster].victim_valid + PVB[old_partition].valid - PVB[t->partition_num].valid;
				if (CLUSTER[cluster].victim_partition_num > PARTITION_PER_CLUSTER + 2)
					PVB[t->partition_num].victim_partition_flag = 0;
			}

			// check the the victim valid
			count = 0;
			_partition *temp_partition = CLUSTER[cluster].victim_partition_list;
			while (temp_partition->partition_num != old_partition) {
				count++;
				temp_partition = temp_partition->next;
			}

			// change the victim valid;
			if (count < PARTITION_PER_CLUSTER + 2) {
				CLUSTER[cluster].victim_valid--;
			}
		}
		//error_cluster_vicitm_valid(cluster);

		if (prev_vic != CLUSTER[cluster].victim_valid) {
			//remove
			
			if (prev_vic < PAGE_PER_CLUSTER) {
				_cluster *cluster_next = CLUSTER[cluster].cluster->next;
				_cluster *cluster_prev = CLUSTER[cluster].cluster->prev;

				if (cluster_next != NULL || cluster_prev != NULL) {

					if (cluster_next->cluster_num == cluster) {
						victim_cluster_pool[prev_vic] = NULL;
					}
					else {
						cluster_next->prev = cluster_prev;
						cluster_prev->next = cluster_next;
					}
					victim_cluster_pool_num[prev_vic]--;

				}

				if (victim_cluster_pool[prev_vic] != NULL) {
					if (CLUSTER[cluster].cluster->cluster_num == victim_cluster_pool[prev_vic]->cluster_num) {
						victim_cluster_pool[prev_vic] = cluster_next;
					}
				}

			}
			//cluster_remove_check(cluster, CLUSTER[cluster].victim_valid);
			//error_cluster_move_check(cluster);

			//insert
			if (CLUSTER[cluster].victim_valid < PAGE_PER_CLUSTER) {
				if (victim_cluster_pool[CLUSTER[cluster].victim_valid] == NULL) {
					victim_cluster_pool[CLUSTER[cluster].victim_valid] = CLUSTER[cluster].cluster;
					CLUSTER[cluster].cluster->prev = CLUSTER[cluster].cluster;
					CLUSTER[cluster].cluster->next = CLUSTER[cluster].cluster;

				}
				else {
					_cluster *new_cluster_prev = victim_cluster_pool[CLUSTER[cluster].victim_valid]->prev;
					new_cluster_prev->next = CLUSTER[cluster].cluster;
					CLUSTER[cluster].cluster->prev = new_cluster_prev;
					CLUSTER[cluster].cluster->next = victim_cluster_pool[CLUSTER[cluster].victim_valid];
					victim_cluster_pool[CLUSTER[cluster].victim_valid]->prev = CLUSTER[cluster].cluster;
				}
				victim_cluster_pool_num[CLUSTER[cluster].victim_valid]++;
			}
			//error_cluster_move_check(cluster);

		}
	}
	
	//error_cluster_vicitm_valid_increase(cluster);
	//error_victim_partition_flag(cluster);

	//error_total_cluster();
	//cluster_count(cluster);
	//cluster_vicitm_valid(cluster);
	// victim_partition_flag_check(cluster);
	//check_PVB(cluster);
	//cluster_vicitm_valid_increase(cluster);
	//victim_cluster_pool_cluster(cluster);

	//cluster_vicitm_valid(cluster);

}

#if 0
int check_cluster(int cluster, int partition) {
	_partition *temp = CLUSTER[cluster].partition_list;
	int count = 0;

	while (count < PARTITION_PER_CLUSTER + 1) {
		int partition_temp = temp->partition_num;

		if (partition_temp == partition) {


			//return ;
		}

		if (PVB[partition_temp].active_flag == 0) {
			count++;
		}
	}
}
#endif

void remove_page(int partition, int PPN, int flag) {
	// if flag = 0 : blockGC. no need to decrease the valid page in block
	int cluster_num = PVB[partition].startLPN / PAGE_PER_CLUSTER;

	//error_cluster_move_check(cluster_num);

	// PVB
	PVB[partition].valid--;
	// Cluster
	CLUSTER[cluster_num].valid--;

	//int prev_victim = CLUSTER[cluster_num].victim_valid;

	// update cluster
	invalid_page_cluster(0, partition);

	//int block = PPN / PAGE_PER_BLOCK;
	//BIT[block].invalid++;
	//PB[block].valid--;
	//PB[block].PPN2LPN[PPN%PAGE_PER_BLOCK] = 0;

}

void allocate2free_block_pool(int victim_block, int flag) {

	/*int count = 1;
	if (free_block != 0) {
		_block *temp = free_block_pool->next;
		while (free_block_pool->block_num != temp->block_num) {
			count++;
			temp = temp->next;
		}
	}

	if (count != free_block) {
		int debug = 1;
	}*/

	/*if (BIT[victim_block].alloc_free->free_flag == 1) {
		int debug = 1;
	}*/

	//error_free_block_pool();

		BIT[victim_block].alloc_free->free_flag = 1;

	/*if (flag == 0) {
	
		_block *prev = free_block_pool->prev;
		BIT[victim_block].alloc_free->next = free_block_pool;
		BIT[victim_block].alloc_free->prev = prev;
		prev->next = BIT[victim_block].alloc_free;
		free_block_pool->prev = BIT[victim_block].alloc_free;
	}*/
	//else {
		// allocated to 
		_block *prev = BIT[victim_block].alloc_free->prev;
		_block *next = BIT[victim_block].alloc_free->next;
		prev->next = next;
		next->prev = prev;

		if (victim_block == allocated_block_pool->block_num)
			allocated_block_pool = allocated_block_pool->next;

		_block *prev_2 = free_block_pool->prev;
		BIT[victim_block].alloc_free->next = free_block_pool;
		BIT[victim_block].alloc_free->prev = prev_2;
		prev_2->next = BIT[victim_block].alloc_free;
		free_block_pool->prev = BIT[victim_block].alloc_free;
	//}
		free_block++;


		//error_free_block_pool();

	/*count = 1;
	if (free_block != 0) {
		_block *temp = free_block_pool->next;
		while (free_block_pool->block_num != temp->block_num) {
			count++;
			temp = temp->next;
		}
	}

	if (count != free_block+1) {
		int debug = 1;
	}*/
	
	
	
}



void invalid_block_in_partition(int victim_block) {

	int i,j;
	//_partition_in_block *temp = BIT[victim_block].partition;
	for (i = 0; i < BIT[victim_block].num_partition; i++) {
		//if(BIT[vic])
		//int partition = temp->partition_num;
		int partition = BIT[victim_block].partition[i];

		for (j = 0; j < PVB[partition].blocknum; j++) {
			if (PVB[partition].block[j] == victim_block) {
				PVB[partition].block[j] = -2;

			}
		}
		if (PVB[partition].valid == 0) {
			free_full_invalid_partition(partition);
		}

		//temp = temp->next;
	}
}

#if 0
int select_victim_block(int *flag) {
	int victim_block = 0;

	if (full_invalid_block_num != 0) {

		if (full_invalid_block_num == 1) {
			victim_block = full_invalid_block_pool->block_num;
			full_invalid_block_pool = NULL;
			*flag = 0;
		}
		else {

			if (full_invalid_block_num == 2) {
				int debug = 3;
			}
			_block *temp = full_invalid_block_pool;
			victim_block = full_invalid_block_pool->block_num;
			_block *prev = temp->prev;
			_block *next = temp->next;

			prev->next = next;
			next->prev = prev;

			full_invalid_block_pool = full_invalid_block_pool->next;
			*flag = 0;
		}
		
		full_invalid_block_num--;
		//free(*(&temp));
	}

	else {

		_block *alloc_block_temp = allocated_block_pool;
		*flag = 1;

		// find the victim block in allocated_block_pool;
		while (alloc_block_temp->block_num != allocated_block_pool->prev->block_num) {
			if (BIT[alloc_block_temp->block_num].invalid == PAGE_PER_BLOCK - 1) {
				victim_block = alloc_block_temp->block_num;
				break;
			}
			if (BIT[alloc_block_temp->block_num].invalid > BIT[victim_block].invalid) {
				victim_block = alloc_block_temp->block_num;
			}
			alloc_block_temp = alloc_block_temp->next;
		}
	}
	
	return victim_block;
}
#endif


int select_victim_block() {
	int victim_block = 0;

	int i;
	for (i = 0; i < FREE_BLOCK; i++) {
		if (BIT[victim_block].invalid < BIT[i].invalid) {
			victim_block = i;

		}
	}
	//error_block_invalid(victim_block);
	return victim_block;
}

int select_victim_cluster(int *predict) {
	
	int i;
	// select victim cluster
	for (i = 0; i < PAGE_PER_CLUSTER; i++) {
		_cluster *temp_cluster = victim_cluster_pool[i];
		if (temp_cluster != NULL) {
			if (temp_cluster->cluster_num != -1 && CLUSTER[temp_cluster->cluster_num].victim_partition_num > PARTITION_PER_CLUSTER +1) {
				// exist
				//_cluster *temp_next = temp_cluster->next;
				//_cluster *temp_prev = temp_cluster->prev;

				//temp_prev->next = temp_next;
				//temp_next->prev = temp_prev;

				//victim_cluster_pool[i] = victim_cluster_pool[i]->next;
				*predict = i;
				return temp_cluster->cluster_num;
			}
		}
	}

	// not exist 
	printf("victim not exist\n");
	getchar();
	return 0;

}




#if 0
int select_victim_cluster() {
	
	int i;
	int victim_cluster = -1;
	int valid_in_vicim_cluster = FREE_BLOCK * PAGE_PER_BLOCK;


	for (i = 0; i < NUMBER_STREAM; i++) {
		int partition = SIT[i].activePartition;
		if (partition != -1) {
			int cluster = PVB[partition].startLPN / PAGE_PER_CLUSTER;
			CLUSTER[cluster].valid = CLUSTER[cluster].valid - PVB[partition].valid;
			CLUSTER[cluster].num_partition--;
		}
	}

	int * arr_valid = (int *)malloc(sizeof(int)*(int)(NUMBER_PARTITION));
	int * arr_partition = (int *)malloc(sizeof(int)*NUMBER_PARTITION);
	int num = 0;
	int j;
	int valid_num = 0;
	// select victim cluster;
	for (i = 0; i < NUMBER_CLUSTER; i++) {
		
		// get the smallest things.
		_CLUSTER_PARTITION * temp = CLUSTER[i].cluster;
		num = 0;
		while (temp != NULL) {
			if (PVB[temp->partition].active_flag == 0) {
				arr_partition[num] = temp->partition;
				arr_valid[num] = PVB[temp->partition].valid;
				num++;
			}
			temp = temp->next;
		}

		quickSort(arr_valid, arr_partition, 0, num - 1);
		valid_num = 0;
		for (j = 0; j < PARTITION_PER_CLUSTER + 2; j++) {
			valid_num = valid_num + arr_valid[j];
		}


		if (CLUSTER[i].num_partition != 0) {
			if (valid_num < valid_in_vicim_cluster && (CLUSTER[i].num_partition > PARTITION_PER_CLUSTER + 1)) {
				//if ((CLUSTER[i].valid < valid_in_vicim_cluster) && (CLUSTER[i].num_partition > PARTITION_PER_CLUSTER + 1)) {
				victim_cluster = i;
				valid_in_vicim_cluster = valid_num;
			}
		}
	}
	
	
	
	for (i = 0; i < NUMBER_STREAM; i++) {
		int partition = SIT[i].activePartition;
		if (partition != -1) {
			int cluster = PVB[partition].startLPN / PAGE_PER_CLUSTER;
			CLUSTER[cluster].valid = CLUSTER[cluster].valid + PVB[partition].valid;
			CLUSTER[cluster].num_partition++;
		}
	}
	free(arr_valid);
	free(arr_partition);
	return victim_cluster;
}
#endif


#if 0
int select_victim_cluster() {
	int i;
	int victim_cluster = -1;
	int valid_in_vicim_cluster = FREE_BLOCK * PAGE_PER_BLOCK;

	
	for (i = 0; i < NUMBER_STREAM; i++) {
		int partition = SIT[i].activePartition;
		if (partition != -1) {
			int cluster = PVB[partition].startLPN / PAGE_PER_CLUSTER;
			CLUSTER[cluster].valid = CLUSTER[cluster].valid - PVB[partition].valid;
			CLUSTER[cluster].num_partition--;
		}	
	}

	// select victim cluster;
	for (i = 0; i < NUMBER_CLUSTER; i++) {
		if (CLUSTER[i].num_partition != 0) {
			if (((double)((double)CLUSTER[i].valid / (double)CLUSTER[i].num_partition)< valid_in_vicim_cluster) && (CLUSTER[i].num_partition > PARTITION_PER_CLUSTER + 1)) {
			//if ((CLUSTER[i].valid < valid_in_vicim_cluster) && (CLUSTER[i].num_partition > PARTITION_PER_CLUSTER + 1)) {
				victim_cluster = i;
				valid_in_vicim_cluster = CLUSTER[i].valid / (double)CLUSTER[i].num_partition;
			}
		}	
	}
	for (i = 0; i < NUMBER_STREAM; i++) {
		int partition = SIT[i].activePartition;
		if (partition != -1) {
			int cluster = PVB[partition].startLPN / PAGE_PER_CLUSTER;
			CLUSTER[cluster].valid = CLUSTER[cluster].valid + PVB[partition].valid;
			CLUSTER[cluster].num_partition++;
		}
	}
	return victim_cluster;
}
#endif
#if 0
int select_victim_cluster() {
	int i;
	int victim_cluster = -1;
	//double valid_in_vicim_cluster = FREE_BLOCK * PAGE_PER_BLOCK;
	int valid_in_vicim_cluster = FREE_BLOCK * PAGE_PER_BLOCK;

	//
	//
	for (i = 0; i < NUMBER_STREAM; i++) {
		int partition = SIT[i].activePartition;
		if (partition != -1) {
			int cluster = PVB[partition].startLPN / PAGE_PER_CLUSTER;
			CLUSTER[cluster].valid = CLUSTER[cluster].valid - PVB[partition].valid;
			CLUSTER[cluster].num_partition--;
		}
	}

	// select victim cluster;
	for (i = 0; i < NUMBER_CLUSTER; i++) {
		if (CLUSTER[i].num_partition != 0) {
			if (((double)((double)CLUSTER[i].valid / (double)CLUSTER[i].num_partition)< valid_in_vicim_cluster) && (CLUSTER[i].num_partition > PARTITION_PER_CLUSTER + 1)) {
				//if ((CLUSTER[i].valid < valid_in_vicim_cluster) && (CLUSTER[i].num_partition > PARTITION_PER_CLUSTER + 1)) {
				victim_cluster = i;
				valid_in_vicim_cluster = CLUSTER[i].valid / (double)CLUSTER[i].num_partition;
			}
		}
	}
	for (i = 0; i < NUMBER_STREAM; i++) {
		int partition = SIT[i].activePartition;
		if (partition != -1) {
			int cluster = PVB[partition].startLPN / PAGE_PER_CLUSTER;
			CLUSTER[cluster].valid = CLUSTER[cluster].valid + PVB[partition].valid;
			CLUSTER[cluster].num_partition++;
		}
	}
	return victim_cluster;
}
#endif
