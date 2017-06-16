#ifndef _IOPM_MAP_H
#define _IOPM_MAP_H

#include <stdio.h>

#define PVB_BLOCK_RECLAIMED (-2)

int select_stream(int LPN, int IO_type);
void do_gc_if_needed(int flag);
void remove_partition_from_cluster(int victim_partition, int victim_cluster, int IO_type);

void free_full_invalid_partition(int partition);

void unlink_partition_from_BIT(int partition);

int find_bitmap(int partition, int offset, int read);
void insert_bitmap(int partition, int offset);
int bitCount_u32_HammingWeight(unsigned int n, int IO_type);
int get_offset_fast(unsigned int *a, int n, int IO_type);

int allocate_block();
int allocate_partition(int flag);
void insert_partition_into_cluster(int cluster, int partition);
void swap(int* a, int* b);
void quick_sort(int * array_lpn, int * array_ppn, int start, int end);

int LPN2Partition(int LPN, int read);
int LPN2Partition_limit(int LPN, int victim, int IO_type);
int partition_order_compare(int a, int b, int read);
int Partition2PPN(int LPN, int partition, int read);
int physical_offset(int partition, int log_offset, int read);
int logical_offset(int partition, int LP, int read);
void two_bitmap(unsigned int n);
void quickSort(int arr[], int b[], int left, int right);
void close_partition(int stream, int IO_type);


void remove_page(int partition, int PPN, int flag);
void put_block(int victim_block, int flag);
void invalid_block_in_partition(int victim_block);
int select_victim_block();
int select_victim_cluster(int *predict);

#endif