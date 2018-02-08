#include "iopm.h"
#include "main.h"
#include "error.h"
#include "iopm_map.h"
#include "init.h"

/* read pages started with 'start_LPN' */
void read(int start_LPN, int count) {
	int i;

	do_count(prof_IO_read_req, 1);

	for (i = 0; i<count; i++) 
		IOPM_read(start_LPN + i, IO);
}

/* read LPN */
int IOPM_read(int LPN, int IO_type) {
	/* count the read operation */

	int partition, PPN;

	/****** COUNTING ******/
	if (IO_type == IO)
		do_count(prof_IO_read, 1);
	else if (IO_type == PGC)
		do_count(prof_PGC_read, 1);
	else if (IO_type == BGC)
		do_count(prof_BGC_read, 1);
	/*********************/

	PPN = LPN2PPN(LPN, &partition, IO);

	// NAND read count
	do_count(prof_NAND_read, 1);

	if (PPN == -1 && IO_type != TEST)
		do_count(prof_IO_nullread, 1);

	return PPN;
}

/* write 'count' pages started with 'start_LPN' */
void write(int start_LPN, int count) {

	int i;

	do_count(prof_IO_write_req, 1);

	for (i = 0; i<count; i++) 
		IOPM_write(start_LPN + i, 0);
}

/* write LPN 
   flag = 0 : I/O write
   flag = 1 : I/O read
   flag = 2 : write during PartitonGC
   flag = 3 : write during BlockGC */
void IOPM_write(int LPN, int IO_type) {

	/****** COUNTING ******/
	if (IO_type == IO)
		do_count(prof_IO_write, 1);
	else if (IO_type == PGC)
		do_count(prof_PGC_write, 1);
	else
		do_count(prof_BGC_write, 1);
	/*********************/

	int overwrite_partition = -1;
	int overwrite_PPN = -1;

	//partition -> stream
	int partition = -1;
	_PVB *ppvb = NULL;

	int block = -1;
	_BIT *pbit = NULL;

	// custer --> partition
	int cluster = CLUSTER_FROM_LPN(LPN);
	_CLUSTER *pcluster = &CLUSTER[cluster];

#if SORTED_CLUSTER_LIST
	int pre_num_partition = pcluster->num_partition;
#endif
	// Select the Stream
	int stream = select_stream(LPN, IO_type);
	_SIT *psit = &SIT[stream];

	// Check Overwrite
	overwrite_PPN = LPN2PPN(LPN, &overwrite_partition, IO_type);

	int new_block_flag = 0;

	// Check the stream status for a write
	// 1. do we need to allocate a new physical block?
	if (IS_BLOCK_FULL(psit->recentPPN)) {

		// unset previous block as inactive
		if (psit->recentPPN != -1) {
			block = BLOCK_FROM_PPN(psit->recentPPN);
			if (!BIT[block].is_active) {
				printf("ERROR:: the block should be active\n");
				// getchar();
			}
			BIT[block].is_active = 0;
		}

		block = allocate_block();
		pbit = &BIT[block];
		new_block_flag = 1;

		psit->recentPPN = block * PAGE_PER_BLOCK;
		pbit->is_active = 1;
	}
	else {
		block = BLOCK_FROM_PPN(psit->recentPPN);
		pbit = &BIT[block];

		psit->recentPPN++;
	}

	// 2. do we need to allocate a new partition?
	if (new_block_flag || psit->recentLPN == -1 || LPN <= psit->recentLPN 
		|| CLUSTER_FROM_LPN(LPN) != CLUSTER_FROM_LPN(psit->recentLPN)) {

		// close previous partition if exist
		if (psit->activePartition != -1)
			close_stream(stream, IO_type);

		partition = allocate_partition(IO_type);

		ppvb = &PVB[partition];

		ppvb->block[ppvb->blocknum] = block;
		ppvb->blocknum++;

		assert(ppvb->blocknum == 1);

		ppvb->startPPN = psit->recentPPN;

		// for aligned partition alloc
		// for future, startLPN can be deleted
		ppvb->startLPN = cluster * PAGE_PER_CLUSTER;
		insert_partition_into_cluster(cluster, partition);

		psit->activePartition = partition;

		link_partition_to_BIT(cluster, partition, block);
	}
	else {
		partition = psit->activePartition;
		ppvb = &PVB[partition];
	}

	// do write
	NAND_write(psit->recentPPN, LPN);

	// Set SIT
	psit->recentLPN = LPN;

	// Set PVB
	insert_bitmap(partition, LPN - ppvb->startLPN);
	ppvb->valid++;

	ppvb->endPPN = psit->recentPPN; //temporal endPPN, will be changed if the partition is not closed 

	// 3. if LPN has old data: invalid old data
	if (overwrite_PPN != -1) {

		if(IO_type == IO)
			do_count(prof_IO_overwrite, 1);

		pbit = &BIT[BLOCK_FROM_PPN(overwrite_PPN)];
		ppvb = &PVB[overwrite_partition];

		// error check
		check_OOB_MAP(LPN, overwrite_PPN);

		// unset page valid bit
		NAND_invalidate(overwrite_PPN);

		// invalid PVB
		if(!(--ppvb->valid))
            free_full_invalid_partition(overwrite_partition);

		// BIT
		if ((++pbit->invalid) == PAGE_PER_BLOCK) {
			unlink_block_from_PVB(pbit->block_num);
			put_block(pbit->block_num, IO_type);
			NAND_erase(pbit->block_num);

			if (IO_type == PGC)
				do_count(prof_PGC_erase, 1);
			else if (IO_type == BGC)
				do_count(prof_BGC_erase, 1);
		}
		else
			sort_block(pbit->block_num);

		if (!ppvb->active_flag)
			pcluster->inactive_valid--;
	}
	else
		pcluster->valid++;

	// test 
	check_LPN_MAP(LPN, psit->recentPPN);
	check_OOB_MAP(LPN, psit->recentPPN);

#if SORTED_CLUSTER_LIST
	// if mean valid pages of cluster are changed, then re-sort the victim list
	if(pre_num_partition != pcluster->num_partition)
		insert_cluster_to_victim_list(cluster);
#endif

	// do partition gc
	do_gc_if_needed(IO_type);

}

static void __do_gc(){

	int gc_count = 0;

	while(free_partition < PARTITION_LIMIT || free_block < BLOCK_LIMIT){
		
		int block = select_vicitim_block_for_unified_GC();

		if(block != -1){
			do_count(prof_UGC_cnt, 1);
			BlockGC(block);
		}
		else if(free_partition < PARTITION_LIMIT)
			PartitionGC();
		else if(free_block < BLOCK_LIMIT)
			BlockGC(-1);
	}	
}

void do_gc_if_needed(int IOtype) {
	
	if (IOtype == IO) {
		if(free_partition < PARTITION_LIMIT || free_block < BLOCK_LIMIT) {
			__do_gc();
		}
	}
}

void BlockGC(int block) {
	
	/*******COUNTING******/
	do_count(prof_BGC_cnt, 1);
	/*********************/

	int victim_block = 0;
	
	/* Select victim block */
	if(block == -1)
		victim_block = select_victim_block();
	else 
		victim_block = block;

	_BIT *pbit = &BIT[victim_block];

	/* Copy the valid pages in Physical Block */
	GC_temp_cnt = 0;

	if (pbit->invalid != PAGE_PER_BLOCK) {
		for (int i = 0; i < PAGE_PER_BLOCK; i++) {
			if (NAND_is_valid(victim_block, i)) {
				
				do_count(prof_BGC_read, 1);

				GC_temp_LPN[GC_temp_cnt] = NAND_read(victim_block, i);
				GC_temp_PPN[GC_temp_cnt] = PPN_FROM_PBN_N_OFFSET(victim_block, i);

				check_LPN_MAP(GC_temp_LPN[GC_temp_cnt], GC_temp_PPN[GC_temp_cnt]);

				GC_temp_cnt++;
			}
		}
	}
	
	/* Copy the pages in victim block */
	if (BIT[victim_block].invalid != PAGE_PER_BLOCK) {
		// reorder the valid pages 
		quickSort(GC_temp_LPN, GC_temp_PPN, 0, GC_temp_cnt - 1);

		// write&remove the pages
		for (int i = 0; i < GC_temp_cnt; i++) {
			// write page
			IOPM_write(GC_temp_LPN[i], BGC);
		}
	}
}

/*
	Partition GC:
*/
void PartitionGC() {

	/*******COUNTING******/
	do_count(prof_PGC_cnt, 1);
	/*********************/

	int victim_cluster = select_victim_cluster();
	int copied_page = 0;
	
	_CLUSTER *pcluster = &CLUSTER[victim_cluster];
	_PVB *ppvb = NULL;

	// page information for new partition
	int first_page = -1;

	GC_temp_cnt = 0;

	list_for_each_entry(_PVB, ppvb, &pcluster->p_list, p_list) {

		do_count(prof_PGC_victim, 1);

		int start_offset, end_offset;

		for (int i = 0; i < ppvb->blocknum; i++) {

			int block = ppvb->block[i];

			if (block == PVB_BLOCK_RECLAIMED)
				continue;

			start_offset = 0;
			end_offset = PAGE_PER_BLOCK - 1;
			if (i == 0)
				start_offset = OFFSET_FROM_PPN(ppvb->startPPN);
			if (i == (ppvb->blocknum - 1))
				end_offset = OFFSET_FROM_PPN(ppvb->endPPN);

			for (int j = start_offset; j <= end_offset; j++) {

				if (NAND_is_valid(block, j)) {
					
					do_count(prof_PGC_read, 1);

					GC_temp_LPN[GC_temp_cnt] = NAND_read(block, j);
					GC_temp_PPN[GC_temp_cnt] = PPN_FROM_PBN_N_OFFSET(block, j);
					GC_temp_cnt++;
				}
			}
		}
	}

	/* Write the copy pages */
	quickSort(GC_temp_LPN, GC_temp_PPN, 0, GC_temp_cnt - 1);
	//victim_cluster_pool_cluster(victim_cluster);

	for (int i = 0; i < GC_temp_cnt; i++) {
		IOPM_write(GC_temp_LPN[i], PGC);
	}
}



