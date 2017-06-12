#include <assert.h>


void error_free_block_pool();
void error_LPN_PPN(int LPN, int PPN);
void error_PPN(int PPN, int partition, int flag);
void error_endPPN_blockorder(int partition);
void error_victim_valid(int victim_block);
void error_PVB_valid(int partition);
void error_PartitionGC_increase(int num, int partition);
int error_partition_valid(int partition);
void check_free_block();
void error_block_invalid(int block);
void error_cluster_move_check(int cluster);
void error_cluster_vicitm_valid(int cluster);
void error_cluster_vicitm_valid_increase_with_p(int cluster, int old_p);
void error_cluster_vicitm_valid_increase(int cluster);
void error_victim_partition_flag(int cluster);


//void cluster_count(int cluster);
//void cluster_vicitm_valid(int cluster);
//void COUNT_write(int write);
//void cluster_vicitm_valid_increase(int cluster);
//void victim_partition_flag_check(int cluster);
//void check_PVB(int cluster);
//void SIT_debug();
//void partition_valid_check(int partition);
//void victim_cluster_pool_cluster(int cluster);
//void cluster_remove_check(int cluster, int valid);
//void error_total_cluster();