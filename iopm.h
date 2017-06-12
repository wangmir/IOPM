
#define IO_WRITE 0
#define IO_READ 1
#define PGC 2
#define BGC 3




int free_partition;
int free_block;
int current_order;




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
	int blocknum;							// 
	int active_flag;						// active partition or not.
	//int order;
	struct _partition *allocate_free;		// list component for partition free/allocation list
	struct _partition *cluster;					// list component for cluster allocation
	struct _partition *victim_partition_list;	// list component for cluster allocation
	int victim_partition_flag;

}_PVB;

_PVB *PVB;

// list component for partition free/allocation list
typedef struct _partition {
	int free_flag;
	int partition_num;						// partition 번호
	struct _partition *next;				// next
	struct _partition *prev;				// prev
}_partition;

_partition *allocated_partition_pool;
_partition *free_partition_pool;

/* CLUSTER : map for cluster management. */
typedef struct _CLUSTER {
	int valid;								// # of valid page in cluster
	int num_partition;						// # of partition in cluster
	int victim_valid;						// # of valid pages in the front n.
	int victim_partition_num;
	struct _partition *partition_list;		// list for partition allocated in cluster
	struct _partition *victim_partition_list;		// list for partition allocated in cluster
	struct _cluster *cluster;				// list component for cluster list (for P.GC)
	
}_CLUSTER;

_CLUSTER *CLUSTER;

// list component for cluster list
typedef struct _cluster {
	int cluster_num;					// cluster number 번호
	struct _cluster *next;				// next
	struct _cluster *prev;				// prev
}_cluster;

_cluster **victim_cluster_pool;
int *victim_cluster_pool_num;

/* SIT : Stream Information Table. */
typedef struct _SIT {
	int recentPartition;					// Partition recently written
	int recentLPN;							// LPN recently written
	int recentPPN;							// PPN recently written	
}_SIT;

_SIT *SIT;

/* BIT : Block Information Table. */
typedef struct _BIT {
	int invalid;							// # of invalid page in block
	int *partition;	// partition mapping to block 
	int num_partition;						// number of partition in block
	struct _block *alloc_free;				// list component for block allocate/free
}_BIT;

_BIT *BIT;

typedef struct _block {
	int block_num;
	struct _block *next;
	struct _block *prev;
	int free_flag;
}_block;

_block *allocated_block_pool;
_block *free_block_pool;
_block *full_invalid_block_pool;


typedef struct _partition_in_block {
	int partition_num;	// partition 번호
	struct _partition_in_block *next; // next
}_partition_in_block;


typedef struct _victim_partition {
	int partition;
	int valid;
}_victim_partition;


int full_invalid_block_num;

/*int * copy_pages_LPN_P;
int * copy_pages_PPN_P;
int * copy_pages_LPN_B;
int * copy_pages_PPN_B;
int * victim_partition;
int * valid_sort;
int * victim_partition_candidates;*/
int * remove_block_in_partition;

int * victim_page_LPN;
int * victim_page_PPN;
int victim_page_num;

int * victim_partition;


// function
void read(int start_LPN, int count);
void IOPM_read(int LPN);
void write(int start_LPN, int count);
void IOPM_write(int LPN, int flag);
void BlockGC();
void PartitionGC();
