#include "iopm.h"
#include "error.h"
#include "main.h"
#include "init.h"
#include "iopm_map.h"

void check_LPN_MAP(int LPN, int PPN)
{
	if (ERROR_CHECKING_ON) {
		// check MAP
		int partition, test_PPN;

		test_PPN = LPN2PPN(LPN, &partition, TEST);

		if (partition == -1)
			assert(0);
		else
			assert(test_PPN == PPN);
	}
}

void check_OOB_MAP(int LPN, int PPN) {

	if (ERROR_CHECKING_ON) {
		// check RMAP
		assert(PB[PPN / PAGE_PER_BLOCK].PPN2LPN[PPN % PAGE_PER_BLOCK] == LPN);
	}
}

void check_total_MAP() {

	if (ERROR_CHECKING_ON) {

		// check LPN MAP
		for (int i = 0; i < LOGICAL_FLASH_SIZE / PAGE_SIZE; i++) {

			int partition, PPN;

			PPN = LPN2PPN(i, &partition, TEST);

			if (partition == -1)
				continue;

			check_OOB_MAP(i, PPN);

		}
		
		// check OOB MAP
		for (int i = 0; i < FLASH_SIZE / PAGE_SIZE; i++) {

			int valid, LPN;

			if (NAND_is_valid(BLOCK_FROM_PPN(i), OFFSET_FROM_PPN(i))) {

				LPN = PB[BLOCK_FROM_PPN(i)].PPN2LPN[OFFSET_FROM_PPN(i)];

				if(LPN != -1)
					check_LPN_MAP(LPN, i);
			}
			else
				continue;
		}
	}
}


// check validity of partition
void __check_partition_validity(int partition) {

	_PVB *ppvb = &PVB[partition];

	int start_offset, end_offset;
	int valid_count = 0;

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

				valid_count++;

				// this page can be accessible through cluster?
				int OOB_LPN = PB[block].PPN2LPN[j];
				int PPN = IOPM_read(OOB_LPN, TEST);

				if (PPN != PPN_FROM_PBN_N_OFFSET(block, j)) {
					printf("ERROR: reverse map and IOPM map is not same\n");
					assert(0);
				}
			}

		}
	}

	if (valid_count != ppvb->valid) {
		printf("ERROR: block valid bitmap and IOPM valid count is not same\n");
		assert(0);
	}
}

void check_partition_validity() {

	for (int i = 0; i < NUMBER_PARTITION; i++) {

		_PVB *ppvb = &PVB[i];

		if (!ppvb->free_flag)
			__check_partition_validity(i);
	}
}

void __check_block_validity(int block) {

	_BIT *pbit = &BIT[block];

	int valid_count = 0;

	for (int i = 0; i < PAGE_PER_BLOCK; i++) {

		if (NAND_is_valid(block, i))
			valid_count++;
	}

	if (pbit->invalid > (PAGE_PER_BLOCK - valid_count)) {

		printf("ERROR: block valid bitmap and block invalid count is not matched\n");
		assert(0);
	}

	if(!pbit->is_active && (pbit->invalid != PAGE_PER_BLOCK - valid_count)) {

		printf("ERROR: block valid bitmap and block invalid count is not matched\n");
		assert(0);
	}

}

void check_block_validity() {

	for (int i = 0; i < FREE_BLOCK; i++) {

		_BIT *pbit = &BIT[i];
		
		if (!pbit->free_flag)
			__check_block_validity(i);
	}
}

void check_validity() {

	if (ERROR_CHECKING_ON) {
		check_partition_validity();
		check_block_validity();
	}

}

