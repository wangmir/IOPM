#ifndef _MAIN_H
#define _MAIN_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>
#include <assert.h>

#include "nand.h"

#define BLOCK_FROM_PPN(PPN) ((PPN) / PAGE_PER_BLOCK)
#define OFFSET_FROM_PPN(PPN) ((PPN) % PAGE_PER_BLOCK)
#define PPN_FROM_PBN_N_OFFSET(PBN, offset) (offset + (PBN * PAGE_PER_BLOCK))

#define IS_BLOCK_FULL(PPN) ((PPN + 1) % PAGE_PER_BLOCK == 0)	// also if 0 when the block is not allocated (-1)


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
long long int OVERPROVISION;
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
int DO_INIT_FLAG;

int break_GC;
int close_blockGC;
int close_streamGC;

/*
*	Structures
*/

typedef struct _NAND_CNT {

	int read;
	int write;
	int erase;

}_NAND_CNT;

typedef struct _IO_CNT {

	_NAND_CNT nand;
	int null_read;
	int overwrite;
	int write_req;
	int read_req;

}_IO_CNT;

typedef struct _PGC_CNT {

	_NAND_CNT nand;
	int pgc_cnt;
	int num_victim_partition;
	int partition_free_cnt;

}_PGC_CNT;

typedef struct _BGC_CNT {

	_NAND_CNT nand;
	int bgc_cnt;
	int ugc_cnt;

}_BGC_CNT;

typedef struct _STAT {

	_NAND_CNT nand;
	_IO_CNT io;
	_PGC_CNT pgc;
	_BGC_CNT bgc;

}_STAT;

enum {
	
	prof_NAND_write = 0,
	prof_NAND_read,
	prof_NAND_erase,

	prof_IO_write,
	prof_IO_read,
	prof_IO_overwrite,
	prof_IO_nullread,
	prof_IO_write_req,
	prof_IO_read_req,

	prof_BGC_write,
	prof_BGC_read,
	prof_BGC_erase,
	prof_BGC_cnt,
	prof_UGC_cnt,

	prof_PGC_write,
	prof_PGC_read,
	prof_PGC_erase,
	prof_PGC_free,
	prof_PGC_cnt,
	prof_PGC_victim
};

_STAT stat;

char * INPUT_FILENAME;

// function
int trace_parsing_filebench(FILE *fp, int* start_LPN, int *count);
int trace_parsing_msr(FILE *fp, int *start_LPN, int *count);
void command_setting(int argc, char *argv[]);
int parsing_size(char * str);
void print_count(char * file, int trace_total_write);
void count_init();
void do_count(int flag, int cnt);
int get_count(int flag);

#endif