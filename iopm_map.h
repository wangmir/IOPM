#include <stdio.h>


int select_stream(int LPN, int IO_type);
void check_partition_gc(int flag);
void remove_partition_from_cluster(int victim_partition, int victim_cluster, int IO_type);
void invalid_page_cluster(int old_LPN, int old_partition);

void free_full_invalid_partition(int partition);
void allocate2free_partition_pool(int victim_partition);

void remove_partition_in_order(int partition);
void remove_partition_in_block(int partition, int *block, int block_num);
void insert_block_to_PVB(int partition, int block);

int find_bitmap(int partition, int offset, int read);
void insert_bitmap(int partition, int offset);
int bitCount_u32_HammingWeight(unsigned int n, int IO_type);
int get_offset_fast(unsigned int *a, int n, int IO_type);

int allocate_block();
int allocate_partition(int flag);
int allocate_partition_to_cluster(int LPN, int partition);
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
void allocate2free_block_pool(int victim_block, int flag);
void invalid_block_in_partition(int victim_block);
int select_victim_block();
int select_victim_cluster(int *predict);
void remove_partition_in_cluster(int partition, int cluster, int IO_type);
int get_partitions_in_cluster(int victim_cluster);