#ifndef _ERROR_H
#define _ERROR_H

#include <assert.h>

#define ERROR_CHECKING_ON (1)

void check_LPN_MAP(int LPN, int PPN);
void check_OOB_MAP(int LPN, int PPN);

void check_total_MAP();

void __check_partition_validity(int partition);
void check_partition_validity();
void __check_block_validity(int block);
void check_block_validity();

void check_validity();

#endif