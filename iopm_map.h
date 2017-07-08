#ifndef _IOPM_MAP_H
#define _IOPM_MAP_H

#include <stdio.h>

#define PVB_BLOCK_RECLAIMED (-2)

int select_stream(int LPN, int IO_type);
void do_gc_if_needed(int flag);
void remove_partition_from_cluster(int partition, int IO_type);

void free_full_invalid_partition(int partition);

void link_partition_to_BIT(int partition, int block);
void unlink_partition_from_BIT(int partition);

int find_bitmap(int partition, int offset, int read);
void insert_bitmap(int partition, int offset);
int bitCount_u32_HammingWeight(unsigned int n, int IO_type);
int get_offset_fast(unsigned int *a, int n, int IO_type);

int allocate_block();
int allocate_partition(int flag);
void insert_partition_into_cluster(int cluster, int partition);
void swap(int* a, int* b);

int LPN2PPN(int LPN, int *partition, int IO_type);

int LPN2Partition(int LPN, int read);
int Partition2PPN(int LPN, int partition, int read);
int physical_offset(int partition, int log_offset, int read);
int logical_offset(int partition, int LP, int read);
void two_bitmap(unsigned int n);
void quickSort(int arr[], int b[], int left, int right);
void close_stream(int stream, int IO_type);
void close_partition(int partition);

void put_block(int victim_block, int flag);
void unlink_block_from_PVB(int victim_block);

void insert_cluster_to_victim_list(int cluster);
int select_victim_block();
int select_victim_cluster();
void select_victim_partition(int cluster, int *p_array);

#endif