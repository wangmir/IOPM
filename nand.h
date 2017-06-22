#ifndef _NAND_H
#define _NAND_H

#include "main.h"

// physical block
typedef struct _PB {
	int *PPN2LPN;		// store LPN to PPN
	int *valid;		// page's validity
}_PB;

_PB *PB;

void NAND_init();
void NAND_write(int PPN, int LPN);
void NAND_invalidate(int PPN);
int NAND_read(int PBN, int offset);
int NAND_is_valid(int PBN, int offset);
void NAND_erase(int block);

#endif
