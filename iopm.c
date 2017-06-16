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

	int overwrite_partition = -1;
	int overwrite_PPN = -1;

	int partition = -1;
	_PVB *ppvb = NULL;

	int block = -1;
	_BIT *pbit = NULL;

	int cluster = CLUSTER_FROM_LPN(LPN);
	_CLUSTER *pcluster = &CLUSTER[cluster];
	
	// Select the Stream
	int stream = select_stream(LPN, IO_type);
	_SIT *psit = &SIT[stream];

	// Check Overwrite
	if (IO_type == IO_WRITE) {
		overwrite_partition = LPN2Partition(LPN, IO_type);
		if (overwrite_partition != -1) {
			//partition_valid_check(overwrite_partition);
			overwrite_PPN = Partition2PPN(LPN, overwrite_partition, IO_type);		
		}
	}

	int new_block_flag = 0;

	// Check the stream status for a write
	// 1. do we need to allocate a new physical block?
	if (IS_BLOCK_FULL(psit->recentPPN)) {
		block = allocate_block();
		pbit = &BIT[block];
		new_block_flag = 1;

		psit->recentPPN = block * PAGE_PER_BLOCK;
	}
	else {
		block = BLOCK_FROM_PPN(psit->recentPPN);
		pbit = &BIT[block];

		psit->recentPPN++;
	}

	// 2. do we need to allocate a new partition?
	if (psit->recentLPN == -1 || LPN <= psit->recentLPN) {

		// close previous partition if exist
		if (psit->activePartition != -1)
			close_partition(stream, IO_type);

		partition = allocate_partition(IO_type);
		ppvb = &PVB[partition];

		ppvb->block[ppvb->blocknum] = block;
		ppvb->blocknum++;

		ppvb->startPPN = psit->recentPPN;

		// for aligned partition alloc
		// for future, startLPN can be deleted
		ppvb->startLPN = cluster * PAGE_PER_CLUSTER;
		insert_partition_into_cluster(cluster, partition);

		psit->activePartition = partition;

		BIT[block].num_partition++;
	}
	else {
		partition = psit->activePartition;
		ppvb = &PVB[partition];

		if (new_block_flag) {

			ppvb->block[ppvb->blocknum] = block;
			ppvb->blocknum++;

			pbit->num_partition++;
		}
	}

	// do write
	NAND_write(psit->recentPPN, LPN);

	// Set SIT
	psit->recentLPN = LPN;

	// Set PVB
	insert_bitmap(partition, LPN - ppvb->startLPN);
	ppvb->valid++;

	// Set CLUSTER
	pcluster->valid++;

	// invalid old data
	if (overwrite_PPN != -1 && IO_type == IO_WRITE) {

		COUNT.overwrite++;
		// error check
		error_LPN_PPN(LPN, overwrite_PPN);

		// unset page valid bit
		NAND_invalidate(overwrite_PPN);

		// invalid PVB
		if(!(--ppvb->valid))
            free_full_invalid_partition(overwrite_partition);

		// BIT
		if((++pbit->invalid) == PAGE_PER_BLOCK)
            put_block(pbit->block_num, IO_type);

		// invalid cluster
		pcluster->valid--;
	}

	// do partition gc
	do_gc_if_needed(IO_type);
	//SIT_debug();
}

void BlockGC() {
	
	/*******COUNTING******/
	BLOCKGC_count();
	/*********************/

	int victim_block = 0;
	
	/* Select victim block */
	victim_block = select_victim_block();

	_BIT *pbit = &BIT[victim_block];

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
			if (SIT[i].activePartition != -1) {
				close_partition(i, BGC);
			}
			SIT[i].activePartition = -1;
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
			int partition = LPN2Partition_limit(victim_page_LPN[i], victim_block, BGC);
			assert(partition != -1);
			IOPM_write(victim_page_LPN[i], BGC);
			remove_page(partition, victim_page_PPN[i], 0);
		}
	}

	/* invalid victim block in partition */
	if(BIT[victim_block].num_partition != 0)
		invalid_block_in_partition(victim_block);

	/* remove block from allocate to free list*/
	put_block(victim_block, flag);
    NAND_erase(victim_block);
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

		// one block in partition
		if (PVB[victim_partition_num].blocknum == 1) {
			int block = PVB[victim_partition_num].block[0];
			if (block != PVB_BLOCK_RECLAIMED) {
				int start_off = PVB[victim_partition_num].startPPN % PAGE_PER_BLOCK;
				int end_off = PVB[victim_partition_num].endPPN % PAGE_PER_BLOCK;
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
			int start_off = PVB[victim_partition_num].startPPN % PAGE_PER_BLOCK;
			int end_off = PVB[victim_partition_num].endPPN % PAGE_PER_BLOCK;
			int i;
			if (block != PVB_BLOCK_RECLAIMED) {

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
				if (block != PVB_BLOCK_RECLAIMED) {
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
			if (block != PVB_BLOCK_RECLAIMED) {
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

		// remove partition in block
		for (int j = 0; j < PVB[temp_victim_partition].blocknum; j++) {
			if (PVB[temp_victim_partition].block[j] != PVB_BLOCK_RECLAIMED) {
				remove_block_in_partition[block_num] = PVB[temp_victim_partition].block[j];
				block_num++;
			}
		}
		// remove blocks in partition (PVB)
		unlink_partition_from_BIT(temp_victim_partition, remove_block_in_partition, block_num);

		// cluster 재조정
		remove_partition_from_cluster(temp_victim_partition, victim_cluster, PGC);

		// init PVB
		init_Partition(temp_victim_partition);
		free_partition++;
	
	}
	//victim_partition_flag_check(victim_cluster);
	assert(free_partition_prev < free_partition);
}



