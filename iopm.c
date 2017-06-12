#include "iopm.h"
#include "main.h"
#include "error.h"
#include "iopm_map.h"
#include "init.h"

/* read pages started with 'start_LPN' */
void read(int start_LPN, int count) {
	int i;
	for (i = 0; i<count; i++) {
		IOPM_read(start_LPN + i);
	}
}

/* read LPN */
void IOPM_read(int LPN) {
	/* count the read operation */
	READ_count();			
	
	int partition = LPN2Partition(LPN, IO_READ);
	int PPN;

	if (partition == -1) {
		// NULL read
	}
	else {
		// calculate the physical address
		PPN = Partition2PPN(LPN, partition, IO_READ);

		//error check : LPN match with PPN?
		error_LPN_PPN(LPN, PPN);
	}	
}

/* write 'count' pages started with 'start_LPN' */
void write(int start_LPN, int count) {
	int i;
	for (i = 0; i<count; i++) {
		IOPM_write(start_LPN + i, 0);
	}
}

/* write LPN 
   flag = 0 : I/O write
   flag = 1 : I/O read
   flag = 2 : write during PartitonGC
   flag = 3 : write during BlockGC */

void IOPM_write(int LPN, int IO_type) {

	/****** COUNTING ******/
	WRITE_count(IO_type);
	/*********************/

	assert(free_partition_pool->free_flag == 1);


	// variables
	int overwrite_partition = -1;
	int overwrite_PPN = -1;
	int partition = -1;
	
	/* Check Overwrite */
	if (IO_type == IO_WRITE) {
		overwrite_partition = LPN2Partition(LPN, IO_type);
		if (overwrite_partition != -1) {
			//partition_valid_check(overwrite_partition);
			overwrite_PPN = Partition2PPN(LPN, overwrite_partition, IO_type);		
		}
	}

	/* Select the Stream*/
	int stream = select_stream(LPN, IO_type);

	int new_partition_flag = 0;
	int new_block_flag = 0;

	// need to allocate new partition
	if (LPN <= SIT[stream].recentLPN || SIT[stream].recentLPN == -1 || (LPN - PVB[SIT[stream].recentPartition].startLPN) >= PAGE_PER_PARTITION) {
		new_partition_flag = 1;	
	}
	
	// need to allocate new block
	if ((SIT[stream].recentPPN + 1) % PAGE_PER_BLOCK == 0) {
		new_block_flag = 1;
	}

	if (new_block_flag == 1 && new_partition_flag == 1) {
		// close stream
		if (SIT[stream].recentPartition != -1) {
			close_partition(stream, IO_type);
			SIT[stream].recentPartition = -1;
			SIT[stream].recentLPN = -1;
		}
		// allocate new partition
		partition = allocate_partition(IO_type);
		free_partition--;

		int block = allocate_block();
		PVB[partition].block[PVB[partition].blocknum] = block;
		PVB[partition].startPPN = block * PAGE_PER_BLOCK;
		PVB[partition].blocknum++;
		SIT[stream].recentPPN = block * PAGE_PER_BLOCK;

		PVB[partition].startLPN = LPN;
		PVB[partition].startPPN = SIT[stream].recentPPN;

		int cluster = allocate_partition_to_cluster(LPN, partition);
		CLUSTER[cluster].num_partition++;

		SIT[stream].recentPartition = partition;
		//partition_valid_check(partition);
		BIT[block].partition[BIT[block].num_partition] = partition;
		BIT[block].num_partition++;

	}
	else if (new_block_flag == 0 && new_partition_flag == 1) {
		assert(free_partition_pool->free_flag == 1);

		// close stream
		if (SIT[stream].recentPartition != -1) {
			close_partition(stream, IO_type);
			SIT[stream].recentPartition = -1;
			SIT[stream].recentLPN = -1;
		}
		SIT[stream].recentPPN = SIT[stream].recentPPN + 1;

		assert(free_partition_pool->free_flag == 1);

		// allocate new partition
		partition = allocate_partition(IO_type);
		free_partition--;

		int block = SIT[stream].recentPPN / PAGE_PER_BLOCK;
		PVB[partition].startLPN = LPN;
		PVB[partition].startPPN = SIT[stream].recentPPN;
		PVB[partition].block[PVB[partition].blocknum] = SIT[stream].recentPPN/PAGE_PER_BLOCK;
		PVB[partition].blocknum++;

		assert(free_partition_pool->free_flag == 1);

		int cluster = allocate_partition_to_cluster(LPN, partition);
		CLUSTER[cluster].num_partition++;
		//victim_partition_flag_check(cluster);

		SIT[stream].recentPartition = partition;
		//partition_valid_check(partition);

		BIT[block].partition[BIT[block].num_partition] = partition;
		BIT[block].num_partition++;


	}
	else if (new_block_flag == 1 && new_partition_flag == 0){
		partition = SIT[stream].recentPartition;
		
		int block = allocate_block();
		PVB[partition].block[PVB[partition].blocknum] = block;
		PVB[partition].blocknum++;
		SIT[stream].recentPPN = block * PAGE_PER_BLOCK;
		//partition_valid_check(partition);

		BIT[block].partition[BIT[block].num_partition] = partition;
		BIT[block].num_partition++;

	}
	else {

		partition = SIT[stream].recentPartition;
		SIT[stream].recentPPN++;
		//partition_valid_check(partition);

	}

	// Set SIT
	SIT[stream].recentLPN = LPN;

	// Set PVB
	insert_bitmap(partition, LPN-PVB[partition].startLPN);
	PVB[partition].valid++;

	// Set CLUSTER
	CLUSTER[PVB[partition].startLPN / PAGE_PER_CLUSTER].valid++;
	// Set PB
	int block = SIT[stream].recentPPN / PAGE_PER_BLOCK;
	int offset = SIT[stream].recentPPN % PAGE_PER_BLOCK;


	assert(PB[block].PPN2LPN[offset] == -1);
	PB[block].valid[offset] = 1;
	PB[block].PPN2LPN[offset] = LPN;
	
	//if(IO_type == IO_WRITE)
	//	victim_partition_flag_check(PVB[partition].startLPN / PAGE_PER_CLUSTER);

	assert(free_partition_pool->free_flag == 1);

	// invalid old data
	if (overwrite_PPN != -1 && IO_type == IO_WRITE) {
		COUNT.overwrite++;
		// error check
		error_LPN_PPN(LPN, overwrite_PPN);	

		// invalid PB
		PB[overwrite_PPN/PAGE_PER_BLOCK].valid[overwrite_PPN % PAGE_PER_BLOCK] = 0;
		PB[overwrite_PPN / PAGE_PER_BLOCK].PPN2LPN[overwrite_PPN % PAGE_PER_BLOCK] = -1;

		// invalid PVB
		PVB[overwrite_partition].valid--;

		// BIT
		BIT[overwrite_PPN / PAGE_PER_BLOCK].invalid++;

		// invalid cluster
		int cluster = PVB[overwrite_partition].startLPN / PAGE_PER_CLUSTER;
		CLUSTER[cluster].valid--;	
		assert(free_partition_pool->free_flag == 1);
	//	victim_partition_flag_check(cluster);

		//cluster_vicitm_valid(cluster);
		if (PVB[overwrite_partition].valid == 0) {
			invalid_page_cluster(LPN, overwrite_partition);
			free_full_invalid_partition(overwrite_partition);
			assert(free_partition_pool->free_flag == 1);

		}
		else {
			// remove the cluster - VICTIM VALID인 경우 CHECK
			invalid_page_cluster(LPN, overwrite_partition);
			assert(free_partition_pool->free_flag == 1);

		}


	}
	//partition_valid_check(partition);
	// do partition gc
	check_partition_gc(IO_type);
	//SIT_debug();
}

#if 0
void BlockGC() {

	/*******COUNTING******/	
		BLOCKGC_count();
	/*********************/
	
	/* Select victim block */
	int victim_block = select_victim_block();
	int i, j;

	// get the startPPN in victim block's partition
	_partition *temp = BIT[victim_block].partition;
	//int *partition_startPPN = (int *)malloc(sizeof(int)*BIT[victim_block].num_partition);
	int *partition_startOff = (int *)malloc(sizeof(int)*BIT[victim_block].num_partition);
	int *partition_endOff = (int *)malloc(sizeof(int)*BIT[victim_block].num_partition);

	int *copy_pages = (int *)malloc(sizeof(int *)* PAGE_PER_BLOCK);
	int *index = (int *)malloc(sizeof(int)*BIT[victim_block].num_partition);
	for (i = 0; i < BIT[victim_block].num_partition; i++) {
		index[i] = -1;
	}
	int copy_pages_num = 0;

	int partition_num = 0;
	while (temp != NULL) {
		if (PVB[temp->partition_num].startPPN / PAGE_PER_BLOCK != victim_block) {
			partition_startOff[partition_num] = 0;
		}
		else {
			partition_startOff[partition_num] = PVB[temp->partition_num].startPPN%PAGE_PER_BLOCK;
		}
		
		if (PVB[temp->partition_num].endPPN / PAGE_PER_BLOCK != victim_block) {
			partition_endOff[partition_num] = PAGE_PER_BLOCK-1;
		}
		else {
			partition_endOff[partition_num] = PVB[temp->partition_num].endPPN%PAGE_PER_BLOCK;
		}
		temp = temp->next;
		partition_num++;
	}

	// copy the pages in each partition
	for (i = 0; i < partition_num; i++) {
		for (j = partition_startOff[i]; j <= partition_endOff[i]; j++) {
			if (PB[victim_block].valid[j] == 1) {
				if (index[i] == -1) {
					index[i] = j;
				}
				copy_pages[copy_pages_num] = PB[victim_block].PPN2LPN[j];
			}
		}		
	}

	for(i)

}
#endif

void BlockGC() {
	
	/*******COUNTING******/
	BLOCKGC_count();
	/*********************/

	int victim_block = 0;
	
	int flag = 1;	//flag == 0 : get the block in full invalid 1 : get the block in allocated
	
	/* Select victim block */
	victim_block = select_victim_block(&flag);

	/* Copy the valid pages in Physical Block */
	victim_page_num = 0;
	if (BIT[victim_block].invalid != PAGE_PER_BLOCK) {
		int i;	
		for (i = 0; i < PAGE_PER_BLOCK; i++) {
			if (PB[victim_block].valid[i] == 1) {
				BLOCKGC_READ_count();
				victim_page_LPN[victim_page_num] = PB[victim_block].PPN2LPN[i];
				victim_page_PPN[victim_page_num] = i + victim_block*PAGE_PER_BLOCK;
				victim_page_num++;
			}
		}
	}

	/* Check that if block is active block*/
	int i;
	for (i = 0; i<NUMBER_STREAM; i++) {
		int active_block = SIT[i].recentPPN / PAGE_PER_BLOCK;
		if (active_block == victim_block) {
			// close partition
			if (SIT[i].recentPartition != -1) {
				close_partition(i, BGC);
			}
			SIT[i].recentPartition = -1;
			SIT[i].recentLPN = -1;
			SIT[i].recentPPN = -1;
		}
	}
	
	/* Copy the pages in victim block */
	if (BIT[victim_block].invalid != PAGE_PER_BLOCK) {
		// reorder the valid pages 
		quickSort(victim_page_LPN, victim_page_PPN, 0, victim_page_num - 1);
		// write&remove the pages
		for (i = 0; i < victim_page_num; i++) {
			// write page
			// 느리다면 여길 바꾸자.
			int partition = LPN2Partition_limit(victim_page_LPN[i], victim_block, BGC);
			assert(partition != -1);
			IOPM_write(victim_page_LPN[i], BGC);
			remove_page(partition, victim_page_PPN[i], 0);
		}
	}


	/* invalid victim block in partition */
	if(BIT[victim_block].num_partition != 0)
		invalid_block_in_partition(victim_block);

	/* Init BIT map */
	BIT[victim_block].invalid = 0;
	//BIT[victim_block].partition;
	memset(BIT[victim_block].partition, -1, PAGE_PER_BLOCK);
	BIT[victim_block].num_partition = 0;

	/* Init PB structure*/
	for (i = 0; i < PAGE_PER_BLOCK; i++) {
		PB[victim_block].PPN2LPN[i] = -1;
		PB[victim_block].valid[i] = -1;
	}
	
	/* remove block from allocate to free list*/
	allocate2free_block_pool(victim_block, flag);

}


void PartitionGC() {

	/*******COUNTING******/
	PARTITIONGC_count();
	/*********************/


	int predict_copy = -1;

	/* Select the victim cluster */
	int victim_cluster = select_victim_cluster(&predict_copy);

	/* Get the victim partitions/pages from cluster */
	int first_page = -1;
	int last_page = -1;
	int num_victim_partition = 0;
	_partition *temp_victim_partition = CLUSTER[victim_cluster].victim_partition_list;
	victim_page_num = 0;
	int copy_index = 0;

	int free_partition_prev = free_partition;

	//while (num_victim_partition < PARTITION_PER_CLUSTER + 2) {
	while(1){

		int victim_partition_num = temp_victim_partition->partition_num;

		if (PVB[victim_partition_num].active_flag == 1) {
			continue;
		}
		
		if (num_victim_partition >= PARTITION_PER_CLUSTER + 2) {
			if (victim_page_num + PVB[victim_partition_num].valid > PAGE_PER_PARTITION / 4) {
				break;
			}
		}


		victim_partition[num_victim_partition] = victim_partition_num;
		num_victim_partition++;
		copy_index = victim_page_num;

		//victim_partition[

		// one block in partition
		if (PVB[victim_partition_num].blocknum == 1) {
			int block = PVB[victim_partition_num].block[0];
			if (block != -2) {
				int start_off = PVB[victim_partition_num].startPPN%PAGE_PER_BLOCK;
				int end_off = PVB[victim_partition_num].endPPN%PAGE_PER_BLOCK;
				int i;

				for (i = start_off; i <= end_off; i++) {
					if (PB[block].valid[i] == 1) {
						victim_page_LPN[victim_page_num] = PB[block].PPN2LPN[i];
						victim_page_PPN[victim_page_num] = block*PAGE_PER_BLOCK + i;
						victim_page_num++;
						PARTITIONGC_READ_count();
					}
				}
			}
		}
		else {
			int block = PVB[victim_partition_num].block[0];
			int start_off = PVB[victim_partition_num].startPPN%PAGE_PER_BLOCK;
			int end_off = PVB[victim_partition_num].endPPN%PAGE_PER_BLOCK;
			int i;
			if (block != -2) {

				// first block
				for (i = start_off; i < PAGE_PER_BLOCK; i++) {
					if (PB[block].valid[i] == 1) {
						victim_page_LPN[victim_page_num] = PB[block].PPN2LPN[i];
						victim_page_PPN[victim_page_num] = block*PAGE_PER_BLOCK + i;
						victim_page_num++;
						PARTITIONGC_READ_count();
					}
				}
			}
			
			// middle
			int j;
			for (j = 1; j < PVB[victim_partition_num].blocknum - 1; j++) {
				block = PVB[victim_partition_num].block[j];
				if (block != -2) {
					for (i = 0; i < PAGE_PER_BLOCK; i++) {
						if (PB[block].valid[i] == 1) {
							victim_page_LPN[victim_page_num] = PB[block].PPN2LPN[i];
							victim_page_PPN[victim_page_num] = block*PAGE_PER_BLOCK + i;
							victim_page_num++;
							PARTITIONGC_READ_count();
						}
					}
				}
			}
			
			// last block
			block = PVB[victim_partition_num].block[PVB[victim_partition_num].blocknum-1];
			if (block != -2) {
				for (i = 0; i <= end_off; i++) {
					if (PB[block].valid[i] == 1) {
						victim_page_LPN[victim_page_num] = PB[block].PPN2LPN[i];
						victim_page_PPN[victim_page_num] = block*PAGE_PER_BLOCK + i;
						victim_page_num++;
						PARTITIONGC_READ_count();

					}
				}
			}
		}

		// Get the first page and last page
		int temp_first = victim_page_LPN[copy_index];
		int temp_last = victim_page_LPN[victim_page_num-1];

		if (first_page == -1) {
			first_page = temp_first;
		}
		else {
			if (temp_first < first_page) {
				first_page = temp_first;
			}
		}

		if (last_page == -1) {
			last_page = temp_last;
		}
		else {
			if (last_page < temp_last) {
				last_page = temp_last;
			}
		}

		//first_page = (temp_first < first_page) ? temp_first : first_page;
		//last_page = (temp_last < last_page) ? temp_last : last_page;
		int need_partition;
		if ((last_page - first_page + 1) % PAGE_PER_PARTITION == 0) {
			need_partition = (last_page - first_page + 1) / PAGE_PER_PARTITION;
		}
		else {
			need_partition = (last_page - first_page + 1) / PAGE_PER_PARTITION + 1;
		}
		if (need_partition < num_victim_partition) {
			break;
		}

		temp_victim_partition = temp_victim_partition->next;		
	}
	
	assert(victim_page_num <= predict_copy);


	/* Write the copy pages */
	quickSort(victim_page_LPN, victim_page_PPN, 0, victim_page_num - 1);
	//victim_cluster_pool_cluster(victim_cluster);

	int i;
	for (i = 0; i < victim_page_num; i++) {
		IOPM_write(victim_page_LPN[i], PGC);

		// Invalid the page
		PB[victim_page_PPN[i] / PAGE_PER_BLOCK].PPN2LPN[victim_page_PPN[i] % PAGE_PER_BLOCK] = -1;
		PB[victim_page_PPN[i] / PAGE_PER_BLOCK].valid[victim_page_PPN[i] % PAGE_PER_BLOCK] = 0;

	}

	/* Remove the partition information */
	for (i = 0; i < num_victim_partition; i++) {
		int temp_victim_partition = victim_partition[i];
		//PVB[temp_victim_partition].valid--;
		int block_num = 0;
		int j;
		// remove partition in block
		for (j = 0; j < PVB[temp_victim_partition].blocknum; j++) {
			if (PVB[temp_victim_partition].block[j] != -2) {
				remove_block_in_partition[block_num] = PVB[temp_victim_partition].block[j];
				block_num++;
			}
		}
		// remove blocks in partition(PVB)
		remove_partition_in_block(temp_victim_partition, remove_block_in_partition, block_num);

		// cluster 재조정
		remove_partition_from_cluster(temp_victim_partition, victim_cluster, PGC);

		// init PVB
		init_Partition(temp_victim_partition);
		free_partition++;
	
	}
	//victim_partition_flag_check(victim_cluster);
	assert(free_partition_prev < free_partition);
}



