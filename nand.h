#ifndef _NAND_H
#define _NAND_H

// physical block
typedef struct _PB {
	int *PPN2LPN;		// store LPN to PPN
	int *valid;		// page's validity
}_PB;

_PB *PB;

inline void NAND_init() {
	// create the PB map
	PB = (_PB *)malloc(sizeof(_PB) * FREE_BLOCK);

	for (int i = 0; i<FREE_BLOCK; i++) {

		PB[i].valid = (int *)malloc(sizeof(int) * PAGE_PER_BLOCK);
		PB[i].PPN2LPN = (int *)malloc(sizeof(int) * PAGE_PER_BLOCK);

		for (int j = 0; j<PAGE_PER_BLOCK; j++) {
			PB[i].valid[j] = -1;
			PB[i].PPN2LPN[j] = -1;
		}
	}
}

inline void NAND_write(int PPN, int LPN) {

	int block = BLOCK_FROM_PPN(PPN);
	int offset = OFFSET_FROM_PPN(PPN);

	// this page must be free page
	assert(PB[block].PPN2LPN[offset] == -1);

	PB[block].valid[offset] = 1;
	PB[block].PPN2LPN[offset] = LPN;
}

inline void NAND_invalidate(int PPN) {

	PB[BLOCK_FROM_PPN(PPN)].valid[OFFSET_FROM_PPN(PPN)] = 0;
}

inline int NAND_read(int PPN) {

	return PB[BLOCK_FROM_PPN(PPN)].PPN2LPN[OFFSET_FROM_PPN(PPN)];
}

inline int NAND_read(int PBN, int offset){

	return PB[PBN].PPN2LPN[offset];
}

inline int NAND_is_valid(int PPN) {

	return PB[BLOCK_FROM_PPN(PPN)].valid[OFFSET_FROM_PPN(PPN)];
}

inline int NAND_is_valid(int PBN, int offset) {

	return PB[PBN].valid[offset];
}

inline void NAND_erase(int block) {
   	/* Init PB structure*/
	for (int i = 0; i < PAGE_PER_BLOCK; i++) {
		PB[block].PPN2LPN[i] = -1;
		PB[block].valid[i] = -1;
	}
}

#endif
