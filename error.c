#include "iopm.h"
#include "error.h"
#include "main.h"
#include "init.h"


void error_victim_partition_flag(int cluster) {

	if (CLUSTER[cluster].victim_partition_num < PARTITION_PER_CLUSTER + 2) {
		int i;
		_partition *temp = CLUSTER[cluster].victim_partition_list;
		for (i = 0; i < CLUSTER[cluster].victim_partition_num; i++) {
			assert(PVB[temp->partition_num].victim_partition_flag == 1);
			temp = temp->next;
		}
	}
	else {
		int i;
		_partition *temp = CLUSTER[cluster].victim_partition_list;
		for (i = 0; i < PARTITION_PER_CLUSTER + 2; i++) {
			assert(PVB[temp->partition_num].victim_partition_flag == 1);
			temp = temp->next;

		}

		for (i = PARTITION_PER_CLUSTER + 2; i < CLUSTER[cluster].victim_partition_num; i++) {
			assert(PVB[temp->partition_num].victim_partition_flag == 0);
			temp = temp->next;
		}
	}

}

void error_cluster_vicitm_valid_increase_with_p(int cluster, int old_p) {
	if (CLUSTER[cluster].victim_partition_list != NULL) {
		_partition *t = CLUSTER[cluster].victim_partition_list;
		int prev = -1;
		int prev_p = -1;
		int count = 0;
		
		assert(prev <= PVB[t->partition_num].valid);
		t = t->next;

		while (t->partition_num != CLUSTER[cluster].victim_partition_list->partition_num) {
			if(t->partition_num == old_p)
				assert(prev <= PVB[t->partition_num].valid +1);
			else
				assert(prev <= PVB[t->partition_num].valid);

			if (prev_p != -1) {
				assert(prev_p != t->partition_num);
			}
			prev_p = t->partition_num;
			prev = PVB[t->partition_num].valid;
			t = t->next;
		}

	}
}

void error_cluster_vicitm_valid_increase(int cluster) {
	if (CLUSTER[cluster].victim_partition_list != NULL) {
		_partition *t = CLUSTER[cluster].victim_partition_list;
		int prev = -1;
		int prev_p = -1;
		int count = 0;

		assert(prev <= PVB[t->partition_num].valid);
		t = t->next;

		while (t->partition_num != CLUSTER[cluster].victim_partition_list->partition_num) {

			assert(prev <= PVB[t->partition_num].valid);

			if (prev_p != -1) {
				assert(prev_p != t->partition_num);
			}
			prev_p = t->partition_num;
			prev = PVB[t->partition_num].valid;
			t = t->next;
		}

	}
}


void error_cluster_vicitm_valid(int cluster) {
	if (CLUSTER[cluster].victim_partition_list != NULL) {
		_partition *t = CLUSTER[cluster].victim_partition_list;
		int victim_valid_count = 0;
		int i;

		if (CLUSTER[cluster].victim_partition_num <= PARTITION_PER_CLUSTER + 2) {
			for (i = 0; i<CLUSTER[cluster].victim_partition_num; i++) {
				victim_valid_count = victim_valid_count + PVB[t->partition_num].valid;
				t = t->next;
			}
		}
		else {
			for (i = 0; i< PARTITION_PER_CLUSTER + 2; i++) {
				victim_valid_count = victim_valid_count + PVB[t->partition_num].valid;
				t = t->next;
			}
		}

		assert(CLUSTER[cluster].victim_valid >= 0);
		//assert(CLUSTER[cluster].victim_valid <= CLUSTER[cluster].valid);
		assert(victim_valid_count == CLUSTER[cluster].victim_valid);

	}
}

void error_block_invalid(int block) {
	int i;
	int count = 0;
	for (i = 0; i < PAGE_PER_BLOCK; i++) {
		if (PB[block].valid[i] == 1) {
			count++;
		}
	}
	assert(count == 128 - BIT[block].invalid);
}

void error_cluster_move_check(int cluster) {
	int flag = 0;
	int valid = CLUSTER[cluster].victim_valid;
	if (valid < PAGE_PER_CLUSTER) {
		_cluster *temp = victim_cluster_pool[valid];
		if (victim_cluster_pool[valid] != NULL) {
			if (victim_cluster_pool[valid]->cluster_num == cluster) {
				flag = 1;
			}
			else {
				temp = temp->next;
				while (temp->cluster_num != victim_cluster_pool[valid]->cluster_num) {
					if (cluster == temp->cluster_num) {
						flag = 1;
					}
					temp = temp->next;
				}
			}
			assert(flag == 1);
		}
	}
}

void error_free_block_pool() {
	int count = 1;
	_block *temp = free_block_pool;
	temp = temp->next;
	while (temp->block_num != free_block_pool->block_num) {
		count++;
		temp = temp->next;
	}
	assert(count == free_block);
}


void check_free_block() {
	if (free_block == 0) {
		printf("[Error] No Free Block!\n");
		getchar();
		//exit(1);
	}
}

int error_partition_valid(int partition) {
	int count =0;
	return count;
}

void error_LPN_PPN(int LPN, int PPN) {
	//if (PB[PPN / PAGE_PER_BLOCK].PPN2LPN[PPN%PAGE_PER_BLOCK] != LPN) {
	//	printf("[ERROR] LPN doesn't match with PPN\n");
	//	getchar();
		//exit(1);
	//}
	assert(PB[PPN / PAGE_PER_BLOCK].PPN2LPN[PPN%PAGE_PER_BLOCK] == LPN);
}

void error_PPN(int PPN, int partition, int flag) {
	if (PPN == -1) {
		printf("[ERROR] wrong PPN. PPN = -1\n");
		getchar();
		//exit(1);
	}
}

void error_victim_valid(int victim_block) {
	if (BIT[victim_block].invalid <= 0) {
		printf("[ERROR] victim block has no valid pages\n");
		getchar();
		//exit(1);
	}
}


void error_PVB_valid(int partition) {
	if (PVB[partition].valid > PARTITION_SIZE / PAGE_SIZE) {
		printf("PVB[partition].valid > PARTITION_SIZE / PAGE_SIZE\n");
		getchar();
		//exit(1);
	}
}

void error_PartitionGC_increase(int num, int partition) {
	if (num != PVB[partition].valid) {
		printf("error\n wrong valid pages num\n");
		getchar();
		//exit(1);
	}
}


#if 0
void cluster_count(int cluster) {
	if(CLUSTER[cluster].victim_partition_list!= NULL){
		_partition *t = CLUSTER[cluster].victim_partition_list;
		int count = 0;
		while (t->next->partition_num != CLUSTER[cluster].victim_partition_list->partition_num) {
			count++;
			t = t->next;
		}

		assert(count + 1 == CLUSTER[cluster].victim_partition_num);
	}
	
}

void cluster_vicitm_valid_increase(int cluster) {
	if (CLUSTER[cluster].victim_partition_list != NULL) {
		_partition *t = CLUSTER[cluster].victim_partition_list;
		int prev = -1;
		int prev_p = -1;
		int count = 0;
		while (t->next->partition_num != CLUSTER[cluster].victim_partition_list->partition_num) {
			assert(prev <= PVB[t->partition_num].valid);
			
			if (prev_p != -1) {
				assert(prev_p != t->partition_num);
			}
			prev_p = t->partition_num;
			prev = PVB[t->partition_num].valid;
			t = t->next;
		}

	}
}

void cluster_vicitm_valid(int cluster) {
	if (CLUSTER[cluster].victim_partition_list != NULL) {
		_partition *t = CLUSTER[cluster].victim_partition_list;
		int victim_valid_count = 0;
		int i;

		if (CLUSTER[cluster].victim_partition_num <= PARTITION_PER_CLUSTER + 2) {
			for (i = 0; i<CLUSTER[cluster].victim_partition_num; i++) {
				victim_valid_count = victim_valid_count + PVB[t->partition_num].valid;
				t = t->next;
			}
		}
		else {
			for (i = 0; i< PARTITION_PER_CLUSTER + 2; i++) {
				victim_valid_count = victim_valid_count + PVB[t->partition_num].valid;
				t = t->next;
			}
		}

		assert(CLUSTER[cluster].victim_valid >= 0);
		assert(CLUSTER[cluster].victim_valid <= CLUSTER[cluster].valid);
		assert(victim_valid_count == CLUSTER[cluster].victim_valid);

	}
}

void partition_valid_check(int partition) {
	int j;
	int p = partition;
	int count = 0;
	if (PVB[p].endPPN != -1) {
		if (PVB[p].blocknum == 1) {
			int block = PVB[p].block[0];
			int start_off = PVB[p].startPPN % 128;
			int end_off = PVB[p].endPPN % 128;
			if (end_off != -1) {
				for (j = start_off; j <= end_off; j++) {
					if (PB[block].valid[j] == 1) {
						count++;
					}
				}
				assert(count == PVB[p].valid);
			}
		}
		else {
			int block = PVB[p].block[0];
			int start_off = PVB[p].startPPN % 128;
			int end_off = PVB[p].endPPN % 128;
			for (j = start_off; j < 128; j++) {
				if (PB[block].valid[j] == 1) {
					count++;
				}
			}

			//end
			int block_num = PVB[p].blocknum;
			block = PVB[p].block[block_num - 1];
			for (j = 0; j <= end_off; j++) {
				if (PB[block].valid[j] == 1) {
					count++;
				}
			}

			// middle
			block_num = PVB[p].blocknum;
			int k;
			for (k = 1; k < block_num - 1; k++) {
				int block = PVB[p].block[k];
				for (j = 0; j <= end_off; j++) {
					if (PB[block].valid[j] == 1) {
						count++;
					}
				}
			}
			assert(count == PVB[p].valid);
		}
	}
}

void victim_partition_flag_check(int cluster) {
	if (CLUSTER[cluster].victim_partition_list != NULL) {
		_partition *t = CLUSTER[cluster].victim_partition_list;
		int victim_valid_count = 0;
		int i,j;

		if (CLUSTER[cluster].victim_partition_num <= PARTITION_PER_CLUSTER + 2) {
			for (i = 0; i<CLUSTER[cluster].victim_partition_num; i++) {
				int p = t->partition_num;
				assert(PVB[p].victim_partition_flag == 1);
				int count = 0;
				if (PVB[p].blocknum == 1) {
					int block = PVB[p].block[0];
					int start_off = PVB[p].startPPN % 128;
					int end_off = PVB[p].endPPN % 128;
					for (j = start_off; j <= end_off; j++) {
						if (PB[block].valid[j] == 1) {
							count++;
						}
					}
					assert(count == PVB[p].valid);
				}
				else {
					int block = PVB[p].block[0];
					int start_off = PVB[p].startPPN % 128;
					int end_off = PVB[p].endPPN % 128;
					for (j = start_off; j < 128; j++) {
						if (PB[block].valid[j] == 1) {
							count++;
						}
					}

					//end
					int block_num = PVB[p].blocknum;
					block = PVB[p].block[block_num-1];
					for (j = 0; j <= end_off; j++) {
						if (PB[block].valid[j] == 1) {
							count++;
						}
					}

					// middle
					block_num = PVB[p].blocknum;
					int k;
					for (k = 1; k < block_num - 1; k++) {
						int block = PVB[p].block[k];
						for (j = 0; j <= end_off; j++) {
							if (PB[block].valid[j] == 1) {
								count++;
							}
						}
					}
					assert(count == PVB[p].valid);
				}
				t = t->next;
			}
		}
		else {
			for (i = 0; i< PARTITION_PER_CLUSTER + 2; i++) {
				int p = t->partition_num;
				assert(PVB[p].victim_partition_flag == 1);
				int count = 0;
				if (PVB[p].blocknum == 1) {
					int block = PVB[p].block[0];
					int start_off = PVB[p].startPPN % 128;
					int end_off = PVB[p].endPPN % 128;
					for (j = start_off; j <= end_off; j++) {
						if (PB[block].valid[j] == 1) {
							count++;
						}
					}
					assert(count == PVB[p].valid);
				}
				else {
					int block = PVB[p].block[0];
					int start_off = PVB[p].startPPN % 128;
					int end_off = PVB[p].endPPN % 128;
					for (j = start_off; j < 128; j++) {
						if (PB[block].valid[j] == 1) {
							count++;
						}
					}

					//end
					int block_num = PVB[p].blocknum;
					block = PVB[p].block[block_num - 1];
					for (j = 0; j <= end_off; j++) {
						if (PB[block].valid[j] == 1) {
							count++;
						}
					}

					// middle
					block_num = PVB[p].blocknum;
					int k;
					for (k = 1; k < block_num - 1; k++) {
						int block = PVB[p].block[k];
						for (j = 0; j <= end_off; j++) {
							if (PB[block].valid[j] == 1) {
								count++;
							}
						}
					}
					assert(count == PVB[p].valid);
				}
				assert(PVB[t->partition_num].victim_partition_flag == 1);
				t = t->next;
			}
		}

	}
}

void cluster_remove_check(int cluster, int valid) {
	if (valid < PAGE_PER_CLUSTER) {
		_cluster *temp = victim_cluster_pool[valid];
		if (victim_cluster_pool[valid] != NULL) {
			while (temp->next->cluster_num != victim_cluster_pool[valid]->cluster_num) {
				assert(temp->cluster_num != cluster);
				temp = temp->next;
			}
		}
	}
}


void victim_cluster_pool_cluster(int cluster) {
	int victim = CLUSTER[cluster].victim_valid;
	int count = 0;
	if (victim < PAGE_PER_CLUSTER) {
		_cluster *temp = victim_cluster_pool[victim];
		if (victim_cluster_pool[victim] != NULL) {

			if (temp->next->cluster_num == victim_cluster_pool[victim]->cluster_num) {
				count = 1;
			}
			else {
				count = 1;
				while (temp->next->cluster_num != victim_cluster_pool[victim]->cluster_num) {
					count++;
					assert(CLUSTER[temp->cluster_num].victim_valid == victim);
					temp = temp->next;
				}
			}
		}
		// check the victim_cluster_pool_num and number of real member in victim_cluster_pool
		assert(count == victim_cluster_pool_num[victim]);

	}
}


void error_total_cluster() {
	int i;
	int j;
	int flag = 1;
	for (j = 0; j < NUMBER_CLUSTER; j++) {
		int vic = CLUSTER[j].victim_valid;

		if (vic < PAGE_PER_CLUSTER && victim_cluster_pool[vic]!=NULL) {
			_cluster *temp = victim_cluster_pool[vic]->next;

			while (temp->cluster_num != victim_cluster_pool[vic]->cluster_num) {

				if (j == temp->cluster_num) {
					flag = 0;
					break;
				}
				temp = temp->next;
			}
			if (j == victim_cluster_pool[vic]->cluster_num) {
				flag = 0;
			}
			assert(flag == 0);
		}
		flag = 1;
	}
}

void check_PVB(int cluster) {
	if (CLUSTER[cluster].victim_partition_list != NULL) {
		_partition *t = CLUSTER[cluster].victim_partition_list;
		int victim_valid_count = 0;
		int i;

		for (i = 0; i<CLUSTER[cluster].victim_partition_num; i++) {
			assert(PVB[t->partition_num].victim_partition_list->next->partition_num == t->next->partition_num);
			assert(PVB[t->partition_num].victim_partition_list->prev->partition_num == t->prev->partition_num);
			t = t->next;
		}
		

	}
}

void SIT_debug() {
	int i;
	int j;
	for (i = 0; i < NUMBER_STREAM; i++) {
		for (j = 0; j < NUMBER_STREAM; j++) {
			if (j != i && SIT[i].recentPartition !=-1 && SIT[j].recentPartition != -1) {
				assert(SIT[i].recentPartition != SIT[j].recentPartition);
			}
		}

	}
}


void check_free_partition_pool() { 
	
}

void COUNT_write(int write) {
	assert(COUNT.write != write);
}
#endif
