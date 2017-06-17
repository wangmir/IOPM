#ifndef _MAIN_H
#define _MAIN_H

#pragma warning(disable:4996)
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>

#include "nand.h"

// Storage
long long int KB;
long long int MB;
long long int GB;

long long int K;
long long int M;
long long int G;

/*
* 	Information with default
*/

// partition
int NUMBER_PARTITION;
int PARTITION_SIZE;

// stream
int NUMBER_STREAM;

// cluster
int CLUSTER_SIZE;
int NUMBER_CLUSTER;

// default size
int SECTOR_SIZE;
int PAGE_SIZE;
int BLOCK_SIZE;
long long int LOGICAL_FLASH_SIZE;
long long int FLASH_SIZE;
double OVERPROVISION;
int FREE_BLOCK;
int ALLOC_PARTITION;

// x per y
int PAGE_PER_BLOCK;
int SECTOR_PER_PAGE;
int PAGE_PER_PARTITION;
int PAGE_PER_PARTITION_32;	// page per partition / 32
int BLOCK_PER_PARTITION;
int PARTITION_PER_CLUSTER;	//partition_size/cluster_size
int PAGE_PER_CLUSTER;	//CLUSERT_SIZE/PAGE_SIZE
int PARTITION_PER_BLOCK;
int BLOCK_PER_PARTITION;

// GC point
int PARTITION_LIMIT;
int BLOCK_LIMIT;

// aging variables
double AGING_LOGICAL_ADDRESS_SPACE_PERCENTAGE;
double AGING_SEQ_PERCENTAGE;
double AGING_RANDOM_PERCENTAGE;

// trace
double TRACE_LOGICAL_ADDRESS_SPACE_PERCENTAGE;


// aging
double RANDOM_RATE;
double SEQ_RATE;
double RANDOM_MOUNT;
double RANDOM_ARRANGE;
int RANDOM_INCREASE;
int RANDOM_SIZE;
char * AGINGFILE;
// 0 : init, else size

int MSR;
int CUTTER;
int AGING_IO;

int break_GC;
int close_blockGC;
int close_partitionGC;

int MTB;

/*
*	Structures
*/

// count information
typedef struct _GC {
	int gc_read;
	int gc_write;
	int gc;				// gc È£Ãâ È½¼ö
	int mem;
}_GC;

// count structure
typedef struct _COUNT {
	int read;
	int write;
	struct _GC partition;
	struct _GC block;
	long long int IO_mem;
	int IO_mem_M;
	int null_partition;
	int overwrite;
	//int nand;
}_COUNT;

_COUNT COUNT;

char * INPUT_FILENAME;

// function
int trace_parsing_filebench(FILE *fp, int* start_LPN, int *count);
int trace_parsing_msr(FILE *fp, int *start_LPN, int *count);
void command_setting(int argc, char *argv[]);
int parsing_size(char * str);
void print_count(char * file, int trace_total_write);
void count_init();
void READ_count();
void WRITE_count(int flag);
void PARTITIONGC_count();
void BLOCKGC_count();
void PARTITIONGC_READ_count();
void PARTITIONGC_WRITE_count();
void BLOCKGC_READ_count();
void BLOCKGC_WRITE_count();
//void MEM_count();
//void PARTITIONGC_MEM_count();
//void BLOCKGC_MEM_count();
void MEM_COUNT_IO(int host);
//void MEM_COUNT_IO(int MEM_HOST);

#endif