#ifndef _IOPM_H
#define _IOPM_H

#include "list.h"

#define IO_WRITE 0
#define IO_READ 1
#define PGC 2
#define BGC 3

#define CLUSTER_FROM_LPN(LPN) ((LPN) / PAGE_PER_CLUSTER)
#define BLOCK_FROM_PPN(PPN) ((PPN) / PAGE_PER_BLOCK)
#define OFFSET_FROM_PPN(PPN) ((PPN) % PAGE_PER_BLOCK)
#define PPN_FROM_PBN_N_OFFSET(PBN, offset) (offset + (PBN * PAGE_PER_BLOCK))

#define IS_BLOCK_FULL(PPN) ((PPN + 1) % PAGE_PER_BLOCK == 0)	// also if 0 when the block is not allocated (-1)

/*
* Mapping Structure
*/

/* PVB : map for bitmap. */
typedef struct _PVB {
	unsigned int *bitmap;					// bitmap for logical page written.
	int startLPN;							// start logical page number
	int startPPN;							// start physical page number
	int endPPN;								// end physical page number
	int valid;								// number of valid page in partition
	int *block;								// blocks allocated in partition
	int blocknum;							// # of blocks allocated in this partition
	int active_flag;						// active partition or not.
	int victim_partition_flag;				// set if this partition is victim for GC

	struct list_head p_list;				// list for partition
	int free_flag;
	int partition_num;

}_PVB;

_PVB *PVB;

struct list_head free_partition_pool;

/*
	CLUSTER : map for cluster management. 
*/
typedef struct _CLUSTER {
	int valid;						// # of valid page in cluster
	int num_partition;				// # of partition in cluster

	int cluster_num;

	struct list_head p_list;		// partition list head to queue allocated partition
									// p_list is LRU list for the partition, active partition will be MRU partition
	struct list_head c_list;		// list for cluster
}_CLUSTER;

_CLUSTER *CLUSTER;

/* SIT : Stream Information Table. */
typedef struct _SIT {
	int activePartition;					// Partition recently written
	int recentLPN;							// LPN recently written
	int recentPPN;							// PPN recently written	

	int stream_num;
	struct list_head SIT_list;
}_SIT;

_SIT *SIT;

struct list_head active_stream_pool;		//managed as LRU
struct list_head free_stream_pool;

/* BIT : Block Information Table. */
typedef struct _BIT {
	int invalid;							// # of invalid page in block
	int num_partition;						// number of partition in block
	int is_active;				// if 1, then the block is in the stream

	int block_num;
	int free_flag;
	struct list_head b_list;
}_BIT;

_BIT *BIT;

struct list_head allocated_block_pool;
struct list_head free_block_pool;

int free_partition;
int free_block;
int current_order;

int * remove_block_in_partition;


// data structure used for GC
// cuz IOPM GC needs sorting to reduce the # of partition
// we also need to examine the actual benefit from sorting
int * GC_temp_LPN;
int * GC_temp_PPN;
int GC_temp_cnt;

int * victim_partition;

// function
void read(int start_LPN, int count);
void IOPM_read(int LPN);
void write(int start_LPN, int count);
void IOPM_write(int LPN, int flag);
void BlockGC();
void PartitionGC();

#endif