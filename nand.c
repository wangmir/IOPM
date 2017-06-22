#include "nand.h"
#include "main.h"


void NAND_init() {
	// create the PB map
	PB = (_PB *)malloc(sizeof(_PB) * FREE_BLOCK);

	for (int i = 0; i < FREE_BLOCK; i++) {

		PB[i].valid = (int *)malloc(sizeof(int) * PAGE_PER_BLOCK);
		PB[i].PPN2LPN = (int *)malloc(sizeof(int) * PAGE_PER_BLOCK);

		for (int j = 0; j<PAGE_PER_BLOCK; j++) {
			PB[i].valid[j] = -1;
			PB[i].PPN2LPN[j] = -1;
		}
	}
}

void NAND_write(int PPN, int LPN) {

	int block = BLOCK_FROM_PPN(PPN);
	int offset = OFFSET_FROM_PPN(PPN);

	// this page must be free page
	assert(PB[block].PPN2LPN[offset] == -1);

	PB[block].valid[offset] = 1;
	PB[block].PPN2LPN[offset] = LPN;
}

void NAND_invalidate(int PPN) {

	PB[BLOCK_FROM_PPN(PPN)].valid[OFFSET_FROM_PPN(PPN)] = 0;
}

int NAND_read(int PBN, int offset) {

	return PB[PBN].PPN2LPN[offset];
}

int NAND_is_valid(int PBN, int offset) {

	return PB[PBN].valid[offset];
}

void NAND_erase(int block) {
	/* Init PB structure*/
	for (int i = 0; i < PAGE_PER_BLOCK; i++) {
		PB[block].PPN2LPN[i] = -1;
		PB[block].valid[i] = -1;
	}
}