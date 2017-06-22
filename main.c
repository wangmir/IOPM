#include "main.h"
#include "iopm.h"
#include "init.h"
#include "random.h"

int main(int argc, char *argv[]) {

	struct tm *t;
	time_t timer;
	timer = time(NULL);
	t = localtime(&timer);

	printf("%d½Ã %dºÐ %dÃÊ\n", t->tm_hour, t->tm_min, t->tm_sec);

	command_setting(argc, argv);

	char * file = argv[1];
	FILE *fp;	
	FILE *fp2;

	int start_LPN = -1;
	int count = -1;
	int trace_total_write = 0;
	int i = 0;

	init();
	AGING_IO = 0;
	break_GC = 0;
	close_partitionGC = 0;
	close_blockGC = 0;

	/* Aging */
	if (AGINGFILE == NULL){
		// seq
		printf("[main] SEQ WRITE\n");
		for (i = 0; i < LOGICAL_FLASH_SIZE / PAGE_SIZE * SEQ_RATE; i++) {
			write(i, 1);
			AGING_IO++;
			if (AGING_IO % 10000 == 1)
				printf("-");
		}
		
		printf("\n[main] RAND WRITE\n");
		RandomInit(1);
		i = 0;
		int j_c = 0;
		//count_init();

		//	random_increase

		if (RANDOM_INCREASE == 1) {
			int last_page = 0;
			if (RANDOM_SIZE == 0) {
				while (i <= (int)(LOGICAL_FLASH_SIZE / PAGE_SIZE * RANDOM_MOUNT)) {
					int arr[4] = { 8, 16, 32, 64 };
					int page = IRandom(last_page, (int)(LOGICAL_FLASH_SIZE / PAGE_SIZE * RANDOM_RATE));
					int count = IRandom(0, 3);
					write(page, arr[count]);
					last_page = page;
					if (last_page >= (int)(LOGICAL_FLASH_SIZE / PAGE_SIZE * RANDOM_RATE)) {
						last_page = 0;
					}
					i = i + arr[count];
					AGING_IO = AGING_IO + arr[count];
					if (j_c % 10000 == 0) {
						printf("-");
					}
					j_c++;
				}
			}
			else {
				while (i <= (int)(LOGICAL_FLASH_SIZE / PAGE_SIZE * RANDOM_MOUNT)) {
					int page = IRandom(last_page, (int)(LOGICAL_FLASH_SIZE / PAGE_SIZE * RANDOM_RATE));
					write(page, RANDOM_SIZE);
					last_page = page;
					if (last_page >= (int)(LOGICAL_FLASH_SIZE / PAGE_SIZE * RANDOM_RATE)) {
						last_page = 0;
					}
					i = i + RANDOM_SIZE;
					AGING_IO = AGING_IO + RANDOM_SIZE;
					if (j_c % 10000 == 0) {
						printf("-");
					}
					j_c++;
				}
			}
		}
		else {
			if (RANDOM_SIZE == 0) {
				while (i <= (int)(LOGICAL_FLASH_SIZE / PAGE_SIZE * RANDOM_MOUNT)) {
					int arr[4] = { 8, 16, 32, 64 };
					int page = IRandom(0,(int)(LOGICAL_FLASH_SIZE / PAGE_SIZE * RANDOM_RATE));
					int count = IRandom(0, 3);
					write(page, arr[count]);
					i = i + arr[count];
					AGING_IO = AGING_IO + arr[count];
					if (j_c % 10000 == 0) {
						printf("-");
					}
					j_c++;

				}
			}
			else {
				while (i <= (int)(LOGICAL_FLASH_SIZE / PAGE_SIZE * RANDOM_MOUNT)) {
					int page = IRandom(0, (int)(LOGICAL_FLASH_SIZE / PAGE_SIZE * RANDOM_RATE));
					write(page, RANDOM_SIZE);
					i = i + RANDOM_SIZE;
					AGING_IO = AGING_IO + RANDOM_SIZE;
					if (j_c % 10000 == 0) {
						printf("-");
					}
					j_c++;
				}
			}
		}
	}
	else {
		//fopen_s(&fp2, AGINGFILE, "r");
		fp2 = fopen(AGINGFILE, "r");

		while (!feof(fp2)) {
			//trace_total_write++;
			int result;
			//if (MSR == 1) {
			//	result = trace_parsing_msr(fp2, &start_LPN, &count);
			//}
			//else {
			result = trace_parsing_filebench(fp2, &start_LPN, &count);
			//}

			if (result == 1) {
				if (start_LPN + count < (int)(LOGICAL_FLASH_SIZE / PAGE_SIZE)) {
					trace_total_write = trace_total_write + count;
					AGING_IO = AGING_IO + count;
					write(start_LPN, count);
				}
			}
			else if (result == 0) {

				if (start_LPN + count < (int)(LOGICAL_FLASH_SIZE / PAGE_SIZE)) {
					//read(start_LPN, count);
				}
			}
			else if (result == 3) {

			}
			else {
				break;
			}

			if (trace_total_write % 5000 == 1)
				printf("-");

			if (CUTTER == 1) {
				if (trace_total_write > (10 * G) / PAGE_SIZE) {
					break;
				}
			}
		}
	}
	printf("\n[main] AGING FINN\n");	

	//fopen_s(&fp, file, "r");
	fp = fopen(file, "r");
	trace_total_write = 0;
	count_init();
	break_GC = 0;
	close_blockGC = 0;
	close_partitionGC = 0;
	i = 0;
	while (!feof(fp)) {
		trace_total_write++;
		int result;
		if (MSR == 1) {
			result = trace_parsing_msr(fp, &start_LPN, &count);
		}
		else {
			result = trace_parsing_filebench(fp, &start_LPN, &count);
		}
	
		if (result == 1) {
			if (start_LPN + count < (LOGICAL_FLASH_SIZE /PAGE_SIZE)) {
				//trace_total_write = trace_total_write + count;
				write(start_LPN, count);
			}
		}
		else if (result == 0) {

			if (start_LPN + count < (LOGICAL_FLASH_SIZE / PAGE_SIZE)) {
				read(start_LPN, count);
			}
		}
		else if (result == 3) {

		}
		else {
			break;
		}

		if (trace_total_write % 5000 == 1)
			printf("-");
	
		if(CUTTER == 1){
			if (trace_total_write > (10 * G) / PAGE_SIZE) {
				break;
			}
		}
	}

	print_count(file, trace_total_write);
	printf("fin\n");
	exit(1);
}

int trace_parsing_msr(FILE *fp, int *start_LPN, int *count) {
	char str[1024];
	char str2[1024];
	unsigned long long int timestamp;
	char *hostname;
	int disknum;
	char *type;
	unsigned long long offset;
	int size;
	int responsetime;

	char *tok = NULL;

	//fscanf_s(fp, "%s", str, 1024);
	fscanf(fp, "%s", str);
	//strcpy_s(str2, 1024, str);
	//printf("%s\n", str);

	if (str == NULL)
		return 2;

	//timestamp = atoll(strtok_s(str, ",", &tok));
	timestamp = atoll(strtok(str, ","));
	if (timestamp == NULL)
		return 2;
	hostname = strtok(NULL, ",");
	if (hostname == NULL)
		return 2;
	disknum = atoi(strtok(NULL, ","));
	//type = strtok_s(NULL, ",", &tok);
	type = strtok(NULL, ",");
	if (type == NULL)
		return 2;
	offset = atoll(strtok(NULL, ","));
	if (offset == NULL)
		return 2;
	size = atoi(strtok(NULL, ","));
	if (size == NULL)
		return 2;
	responsetime = atoi(strtok(NULL, ","));
	if (responsetime == NULL)
		return 2;


	if (strstr(type, "W")) {
		*start_LPN = offset / PAGE_SIZE;

		if ((offset + size) % PAGE_SIZE == 0)
			*count = (offset + size) / PAGE_SIZE - offset / PAGE_SIZE;
		else
			*count = (offset + size) / PAGE_SIZE + 1 - offset / PAGE_SIZE;

		return 1;
	}else if (strstr(type, "R")) {
		*start_LPN = offset / PAGE_SIZE;

		if ((offset + size) % PAGE_SIZE == 0)
			*count = (offset + size) / PAGE_SIZE - offset / PAGE_SIZE;
		else
			*count = (offset + size) / PAGE_SIZE + 1 - offset / PAGE_SIZE;

		return 0;
	}
	else {
		return 2;
	}
}


int trace_parsing_filebench(FILE *fp, int* start_LPN, int *count) {
	char *opType = NULL;
	int start_addr = -1;
	int count_sec = -1;

	char str[1024];
	fgets(str, sizeof(str), fp);
	char *tok;
	//timestamp = atoll(strtok_s(str, ",", &tok));

	if (strstr(str, "W") != NULL) {

		char * ori = strstr(str, "W");
		//char * new = strtok_s(ori, " ", &tok);
		char * new = strtok(ori, " ");
		int strtok_count = 0;

		while (new != NULL) {
			if (strtok_count == 1) {
				start_addr = atoi(new);
			}
			else if (strtok_count == 3) {
				count_sec = atoi(new);
			}
			//new = strtok_s(NULL, " ", &tok);
			new = strtok(NULL, " ");
			strtok_count++;
		}

		*start_LPN = start_addr / SECTOR_PER_PAGE;

		if ((start_addr + count_sec) % SECTOR_PER_PAGE == 0)
			*count = (start_addr + count_sec) / SECTOR_PER_PAGE - start_addr / SECTOR_PER_PAGE;
		else
			*count = (start_addr + count_sec) / SECTOR_PER_PAGE + 1 - start_addr / SECTOR_PER_PAGE;
		
		return 1;
	}
	else if (strstr(str, "R") != NULL) {
		char * ori = strstr(str, "R");
		//char * new = strtok_s(ori, " ", &tok);
		char * new = strtok(ori, " ");
		int strtok_count = 0;

		while (new != NULL) {
			if (strtok_count == 1) {
				start_addr = atoi(new);
			}
			else if (strtok_count == 3) {
				count_sec = atoi(new);
			}
			//new = strtok_s(NULL, " ", &tok);
			new = strtok(NULL, " ");
			strtok_count++;
		}

		*start_LPN = start_addr / SECTOR_PER_PAGE;

		if ((start_addr + count_sec) % SECTOR_PER_PAGE == 0)
			*count = (start_addr + count_sec) / SECTOR_PER_PAGE - start_addr / SECTOR_PER_PAGE;
		else
			*count = (start_addr + count_sec) / SECTOR_PER_PAGE + 1 - start_addr / SECTOR_PER_PAGE;

		return 0;
	}
	else {
		return 3;
	}
	printf("\nEND of file\n");
	return 2;

}


void command_setting(int argc, char *argv[]) {

	int i, j = 0;

	KB = 1024;
	MB = 1024 * KB;                                                                                                                                                                                                                                                                                                                                                   
	GB = 1024 * MB;

	K = 1024;
	M = 1024 * K;
	G = 1024 * M;

	/*
		Default Setting
	*/
	// partition

	PARTITION_SIZE = 256 * KB, CLUSTER_SIZE = 256 * KB, NUMBER_PARTITION = 256 * K;
	// stream
	NUMBER_STREAM = 256;

	// cluster
	//CLUSTER_SIZE = 2 * MB;
	
	//-stc 4 -msr 1 -seqr 0.5 -randm 0.6 -randr 0.3 -rands 1

	SECTOR_SIZE = 512;
	PAGE_SIZE = 4 * KB;
	BLOCK_SIZE = 512 * KB;

	LOGICAL_FLASH_SIZE = 32 * GB;
	OVERPROVISION = 3*GB;
	//LOGICAL_FLASH_SIZE = 16 * GB;
	//OVERPROVISION = 1*GB;
	//5047296

	PARTITION_LIMIT = 20;
	BLOCK_LIMIT = 20;

	//def
	RANDOM_RATE = 0.3;
	SEQ_RATE = 0.5;
	RANDOM_MOUNT = 0.7;
	RANDOM_INCREASE = 1;
	RANDOM_SIZE = 1;
	MSR = 1;
	CUTTER = 0;
	AGINGFILE = NULL;
	//test_setting();
	MTB = 0;

	for (i = 0; i<argc; i++) {

		if (strcmp(argv[i], "-mtb") == 0) {
			i++;
			MTB = atoi(argv[i]) * KB;
		}

		if (strcmp(argv[i], "-aging") == 0) {
			i++;
			AGINGFILE = argv[i];
		}

		if (strcmp(argv[i], "-cut") == 0) {
			i++;
			CUTTER = atoi(argv[i]) * GB;
		}

		if (strcmp(argv[i], "-msr") == 0) {
			i++;
			MSR = atoi(argv[i]);
		}

		if (strcmp(argv[i], "-randr") == 0) {
			i++;
			RANDOM_RATE = atof(argv[i]);
		}

		if (strcmp(argv[i], "-seqr") == 0) {
			i++;
			SEQ_RATE = atof(argv[i]);
		}

		if (strcmp(argv[i], "-randm") == 0) {
			i++;
			RANDOM_MOUNT = atof(argv[i]);
		}

		if (strcmp(argv[i], "-randa") == 0) {
			i++;
			RANDOM_ARRANGE = atof(argv[i]);
		}

		if (strcmp(argv[i], "-randi") == 0) {
			i++;
			RANDOM_INCREASE = atoi(argv[i]);
		}

		if (strcmp(argv[i], "-rands") == 0) {
			i++;
			RANDOM_SIZE = atoi(argv[i]);
		}

		if (strcmp(argv[i], "-stc") == 0) {
			i++;
			NUMBER_STREAM = atoi(argv[i]);
		}

		if (strcmp(argv[i], "-ses") == 0) {
			i++;
			SECTOR_SIZE = atoi(argv[i]);
		}

		if (strcmp(argv[i], "-pas") == 0) {
			i++;
			PARTITION_SIZE = atoi(argv[i])*KB;
		}

		if (strcmp(argv[i], "-cs") == 0) {
			i++;
			CLUSTER_SIZE = atoi(argv[i])*KB;
		}


		if (strcmp(argv[i], "-pac") == 0) {
			i++;
			NUMBER_PARTITION = atoi(argv[i])*K;
		}

		if (strcmp(argv[i], "-ps") == 0) {
			i++;
			PAGE_SIZE = atoi(argv[i]);
		}

		if (strcmp(argv[i], "-bs") == 0) {
			i++;
			BLOCK_SIZE = atoi(argv[i]);
		}

		if (strcmp(argv[i], "-ss") == 0) {
			i++;
			LOGICAL_FLASH_SIZE = atoi(argv[i]) * GB;
		}

		if (strcmp(argv[i], "-op") == 0) {
			i++;
			OVERPROVISION = atoi(argv[i]) * GB;
		}
	}

	

	NUMBER_CLUSTER = LOGICAL_FLASH_SIZE / CLUSTER_SIZE;

	FLASH_SIZE = LOGICAL_FLASH_SIZE + OVERPROVISION;
	FREE_BLOCK = (int)(FLASH_SIZE / (long long int) BLOCK_SIZE);
	

	PAGE_PER_BLOCK = BLOCK_SIZE / PAGE_SIZE;
	SECTOR_PER_PAGE = PAGE_SIZE / SECTOR_SIZE;
	PAGE_PER_PARTITION = PARTITION_SIZE / PAGE_SIZE;
	PAGE_PER_PARTITION_32 = PAGE_PER_PARTITION / 32;
	BLOCK_PER_PARTITION = PARTITION_SIZE / BLOCK_SIZE;
	PARTITION_PER_CLUSTER = CLUSTER_SIZE / PARTITION_SIZE;
	PAGE_PER_CLUSTER = CLUSTER_SIZE / PAGE_SIZE;

	if (PARTITION_SIZE > BLOCK_SIZE) {
		BLOCK_PER_PARTITION = PARTITION_SIZE / BLOCK_SIZE;
		PARTITION_PER_BLOCK = BLOCK_PER_PARTITION;
	}
	else {
		BLOCK_PER_PARTITION = BLOCK_SIZE / PARTITION_SIZE;
		PARTITION_PER_BLOCK = BLOCK_PER_PARTITION;
	}
	

	// DEBUG
	/*printf("[DEBUG] NUMBER_PARTITION : %d K\n", NUMBER_PARTITION/K);
	printf("[DEBUG] PARTITION_SIZE : %d KB\n", PARTITION_SIZE/KB);
	printf("[DEBUG] NUMBER_STREAM : %d\n", NUMBER_STREAM);
	printf("[DEBUG] SECTOR_SIZE : %d Byte\n", SECTOR_SIZE);
	printf("[DEBUG] PAGE_SIZE : %d KB\n", PAGE_SIZE/KB);
	printf("[DEBUG] BLOCK_SIZE : %d KB\n", BLOCK_SIZE/KB);
	printf("[DEBUG] LOGICAL_FLASH_SIZE : %d GB\n", LOGICAL_FLASH_SIZE/GB);
	printf("[DEBUG] OVERPROVISION : %f\n", OVERPROVISION);
	printf("[DEBUG] FLASH_SIZE : %d GB\n", FLASH_SIZE/GB);
	printf("[DEBUG] FREE_BLOCK : %d\n", FREE_BLOCK);
	printf("[DEBUG] PAGE_PER_BLOCK : %d\n", PAGE_PER_BLOCK);
	printf("[DEBUG] CLUSTER_SIZE : %d MB\n", CLUSTER_SIZE/MB);
	printf("[DEBUG] NUMBER_CLUSTER : %d\n", NUMBER_CLUSTER);
	printf("[DEBUG] PARTITION_CLUSTER : %d\n", PARTITION_PER_CLUSTER);
	printf("[DEBUG] SEQ RATE: %f\n", SEQ_RATE);
	printf("[DEBUG] RANDOM RATE : %f\n", RANDOM_RATE);
	printf("[DEBUG] RANDOM MOUNT : %f\n", RANDOM_MOUNT);
	printf("[DEBUG] RANDOM_INCREASE : %d\n", RANDOM_INCREASE);
	printf("[DEBUG] RANDOM_SIZE : %d\n", RANDOM_SIZE);*/

	NAND_init();
}

int parsing_size(char * str) {
	if (strstr(str, "K") != NULL) {
		return KB;
	}
	else if (strstr(str, "M") != NULL) {
		return MB;
	}
	else if (strstr(str, "G") != NULL) {
		return GB;
	}
	else {
		printf("ERROR in parsing_size\n");
		exit(1);
	}
}

void count_init() {
	// init count information
	COUNT.read = 0;
	COUNT.write = 0;
	COUNT.partition.gc = 0;
	COUNT.partition.gc_read = 0;
	COUNT.partition.gc_write = 0;
	COUNT.block.gc = 0;
	COUNT.block.gc_read = 0;
	COUNT.null_partition = 0;
	COUNT.block.gc_write = 0;
}

#if 0
void print_count(char * file, int trace_total_write) {
	printf("printf_count\n");
	FILE *fp;
	char file_name[1024];
	char *tok;
	char * extension = ".txt";
	char * underbar = "_";
	char part_size[1024];
	char clus_size[1024];
	char st_size[1024];
	char num_part[1024];

	sprintf_s(part_size,1024, "%dMB", PARTITION_SIZE/MB);
	sprintf_s(clus_size,1024, "%dMB", CLUSTER_SIZE/MB);
	sprintf_s(st_size, 1024, "%dGB", LOGICAL_FLASH_SIZE / GB);
	sprintf_s(num_part, 1024, "#PA.%d", NUMBER_STREAM);

	strcpy_s(file_name, sizeof(file_name), file);
	strtok_s(file_name, ".", &tok);
	strcat_s(file_name, sizeof(file_name), underbar);
	strcat_s(file_name, 1024, part_size);
	strcat_s(file_name, sizeof(file_name), underbar);
	strcat_s(file_name, 1024, clus_size);
	strcat_s(file_name, sizeof(file_name), underbar);
	strcat_s(file_name, 1024, st_size);
	strcat_s(file_name, sizeof(file_name), underbar);
	strcat_s(file_name, 1024, num_part);
	strcat_s(file_name, 1024, extension);

	// trace_partition_cluster_st.txt
	fopen_s(&fp, file_name, "w");

	fprintf(fp, "NUMBER_PARTITION : %d K\n", NUMBER_PARTITION / K);
	fprintf(fp, "PARTITION_SIZE : %d MB\n", PARTITION_SIZE / MB);
	fprintf(fp, "NUMBER_STREAM : %d\n", NUMBER_STREAM);
	fprintf(fp, "SECTOR_SIZE : %d Byte\n", SECTOR_SIZE);
	fprintf(fp, "PAGE_SIZE : %d KB\n", PAGE_SIZE / KB);
	fprintf(fp, "BLOCK_SIZE : %d KB\n", BLOCK_SIZE / KB);
	fprintf(fp, "LOGICAL_FLASH_SIZE : %d GB\n", LOGICAL_FLASH_SIZE / GB);
	fprintf(fp, "OVERPROVISION : %f\n", OVERPROVISION);
	fprintf(fp, "FLASH_SIZE : %d GB\n", FLASH_SIZE / GB);
	fprintf(fp, "FREE_BLOCK : %d\n", FREE_BLOCK);
	fprintf(fp, "PAGE_PER_BLOCK : %d\n", PAGE_PER_BLOCK);
	fprintf(fp, "CLUSTER_SIZE : %d MB\n", CLUSTER_SIZE / MB);
	fprintf(fp, "NUMBER_CLUSTER : %d\n", NUMBER_CLUSTER);
	fprintf(fp, "PARTITION_CLUSTER : %d\n\n", PARTITION_PER_CLUSTER);
	fprintf(fp, "STREAM : %d\n\n", NUMBER_STREAM);
	fprintf(fp, "[DEBUG] SEQ RATE: %f\n", SEQ_RATE);
	fprintf(fp, "[DEBUG] RANDOM RATE : %f\n", RANDOM_RATE);
	fprintf(fp, "[DEBUG] RANDOM MOUNT : %f\n", RANDOM_MOUNT);
	fprintf(fp, "[DEBUG] RANDOM_INCREASE : %d\n", RANDOM_INCREASE);
	fprintf(fp, "[DEBUG] RANDOM_SIZE : %d\n", RANDOM_SIZE);


	/**/
	int bit;
	if (PAGE_PER_PARTITION % 8 != 0) {
		bit = PAGE_PER_PARTITION / 8 + 1;
	}
	else {
		bit = PAGE_PER_PARTITION / 8;
	}
	int startLPN = 4;
	int startPPN = 4;
	int endPPN = 4;
	double valid_d = log(PAGE_PER_PARTITION) / log(2);
	int valid = ceil(valid_d);
	
	int blockvalidation;
	int BLOCK_PER_PARTITION = PARTITION_SIZE / BLOCK_SIZE;
	if ((BLOCK_PER_PARTITION+1) % 8 != 0) {
		blockvalidation = (BLOCK_PER_PARTITION + 1) % 8 + 1;
	}
	else {
		blockvalidation = (BLOCK_PER_PARTITION + 1) % 8;
	}
	int blockorder = (BLOCK_PER_PARTITION-1)*4;


	int map_size = NUMBER_PARTITION *(bit+startLPN+startPPN+endPPN+valid+blockvalidation+blockorder);
	map_size = map_size + 8 * NUMBER_PARTITION;
	map_size = map_size + 12 * NUMBER_STREAM;
	map_size = map_size + NUMBER_CLUSTER * 4 + 8 * NUMBER_PARTITION;


	//fprintf(fp, "FTL, STREAM, WRITE, READ, ERASE, IO_WRITE, IO_READ, B.GC.write, B.GC.read, B.GC.erase, P.GC.write, P.GC.read, MAP_SIZE, #.PA\n");
	fprintf(fp, "FTL, STREAM, WRITE, READ, ERASE, MAP_SIZE, #.PA\n"); 
	fprintf(fp, "IOPM, ");
	fprintf(fp, "%d, ",NUMBER_STREAM);
	fprintf(fp, "%d, ", COUNT.write + COUNT.partition.gc_write + COUNT.block.gc_write);
	fprintf(fp, "%d, ", COUNT.read + COUNT.partition.gc_read + COUNT.block.gc_read);
	/*fprintf(fp, "%d, ", COUNT.block.gc);
	fprintf(fp, "%d, ", COUNT.write);
	fprintf(fp, "%d, ", COUNT.read);
	fprintf(fp, "%d, ", COUNT.block.gc_write);
	fprintf(fp, "%d, ", COUNT.block.gc_read);
	fprintf(fp, "%d, ", COUNT.block.gc);
	fprintf(fp, "%d, ", COUNT.partition.gc_write);
	fprintf(fp, "%d, ", COUNT.partition.gc_read);*/
	fprintf(fp, "%d, ", map_size);
	fprintf(fp, "%d, ", NUMBER_PARTITION);
	fprintf(fp, "\n");
	fprintf(fp, "OP, IO, MAP, GC, P.GC\n");
	fprintf(fp, "W, ");
	fprintf(fp, "%d, ", COUNT.write);
	fprintf(fp, "0, ");
	fprintf(fp, "%d, ", COUNT.block.gc_write);
	fprintf(fp, "%d, ", COUNT.partition.gc_write);
	fprintf(fp, "\n");
	fprintf(fp, "R, ");
	fprintf(fp, "%d, ", COUNT.read);
	fprintf(fp, "0, ");
	fprintf(fp, "%d, ", COUNT.block.gc_read);
	fprintf(fp, "%d, ", COUNT.partition.gc_read);
	fprintf(fp, "\n");

	fprintf(fp, "\n\n");
	fprintf(fp, "IO_WRITE, IO_READ, B.GC.write, B.GC.read, B.GC.erase, P.GC.write, P.GC.read, P.GC, P.nullGC, break_GC, closeP, closeB\n");
	fprintf(fp, "%d, ", COUNT.write);
	fprintf(fp, "%d, ", COUNT.read);
	fprintf(fp, "%d, ", COUNT.block.gc_write);
	fprintf(fp, "%d, ", COUNT.block.gc_read);
	fprintf(fp, "%d, ", COUNT.block.gc);
	fprintf(fp, "%d, ", COUNT.partition.gc_write);
	fprintf(fp, "%d, ", COUNT.partition.gc_read);
	fprintf(fp, "%d, ", COUNT.partition.gc);
	fprintf(fp, "%d, ", COUNT.null_partition);
	fprintf(fp, "%d, ", break_GC);
	fprintf(fp, "%d, ", close_partitionGC);
	fprintf(fp, "%d, ", close_blockGC);

}
#endif
void print_count(char * file, int trace_total_write) {
	printf("printf_count\n");
	FILE *fp;
	char file_name[1024];
	char *tok;
	char * extension = ".txt";
	char * underbar = "_result";



	/*strcpy_s(file_name, sizeof(file_name), file);
	strtok_s(file_name, ".", &tok);
	strcat_s(file_name, sizeof(file_name), underbar);
	strcat_s(file_name, 1024, extension);*/

	strcpy(file_name,  file);
	strtok(file_name, ".");
	strcat(file_name,  underbar);
	strcat(file_name,  extension);

	// trace_partition_cluster_st.txt
	//fopen_s(&fp, file_name, "r");
	fp = fopen(file_name, "r");

	if (fp == NULL) {
		fp = fopen(file_name, "a+");
		//fopen_s(&fp, file_name, "a+");
		fprintf(fp, "Aging, NUM_STREAM, PA_Size, Cluster_Size, PA#, MAP_SIZE, WRITE, READ, ERASE, IO_WRITE, IO_READ, B.GC.write, B.GC.read, B.GC.erase, P.GC.write, P.GC.read, P.GC, P.nullGC, MEM_IO, MEM_B, MEM_P\n");
	}
	else {
		fclose(fp);
		fp = fopen(file_name, "a+");
		//fopen_s(&fp, file_name, "a+");
	}


	/*fprintf(fp, "NUMBER_PARTITION : %d K\n", NUMBER_PARTITION / K);
	fprintf(fp, "PARTITION_SIZE : %d MB\n", PARTITION_SIZE / MB);
	fprintf(fp, "NUMBER_STREAM : %d\n", NUMBER_STREAM);
	fprintf(fp, "SECTOR_SIZE : %d Byte\n", SECTOR_SIZE);
	fprintf(fp, "PAGE_SIZE : %d KB\n", PAGE_SIZE / KB);
	fprintf(fp, "BLOCK_SIZE : %d KB\n", BLOCK_SIZE / KB);
	fprintf(fp, "LOGICAL_FLASH_SIZE : %d GB\n", LOGICAL_FLASH_SIZE / GB);
	fprintf(fp, "OVERPROVISION : %f\n", OVERPROVISION);
	fprintf(fp, "FLASH_SIZE : %d GB\n", FLASH_SIZE / GB);
	fprintf(fp, "FREE_BLOCK : %d\n", FREE_BLOCK);
	fprintf(fp, "PAGE_PER_BLOCK : %d\n", PAGE_PER_BLOCK);
	fprintf(fp, "CLUSTER_SIZE : %d MB\n", CLUSTER_SIZE / MB);
	fprintf(fp, "NUMBER_CLUSTER : %d\n", NUMBER_CLUSTER);
	fprintf(fp, "PARTITION_CLUSTER : %d\n\n", PARTITION_PER_CLUSTER);
	fprintf(fp, "STREAM : %d\n\n", NUMBER_STREAM);
	fprintf(fp, "[DEBUG] SEQ RATE: %f\n", SEQ_RATE);
	fprintf(fp, "[DEBUG] RANDOM RATE : %f\n", RANDOM_RATE);
	fprintf(fp, "[DEBUG] RANDOM MOUNT : %f\n", RANDOM_MOUNT);
	fprintf(fp, "[DEBUG] RANDOM_INCREASE : %d\n", RANDOM_INCREASE);
	fprintf(fp, "[DEBUG] RANDOM_SIZE : %d\n", RANDOM_SIZE);*/


	/**/
	/*int bit;
	if (PAGE_PER_PARTITION % 8 != 0) {
		bit = PAGE_PER_PARTITION / 8 + 1;
	}
	else {
		bit = PAGE_PER_PARTITION / 8;
	}
	// Bitmap
	
	int startLPN = 4;
	int startPPN = 4;
	int endPPN = 4;
	
	// valid page
	double valid_d = log(PAGE_PER_PARTITION) / log(2);
	int valid = ceil(valid_d);

	if (valid % 8 != 0) {
		valid = valid / 8 + 1;
	}
	else {
		valid = valid / 8;
	}
	*/
	// block validation
	/*int blockvalidation;
	int BLOCK_PER_PARTITION = PARTITION_SIZE / BLOCK_SIZE;
	if ((BLOCK_PER_PARTITION + 1) % 8 != 0) {
		blockvalidation = (BLOCK_PER_PARTITION + 1) / 8 + 1;
	}
	else {
		blockvalidation = (BLOCK_PER_PARTITION + 1) / 8;
	}
	// blocks in partition
	int blockorder = (BLOCK_PER_PARTITION - 1) * 4;

	if (BLOCK_PER_PARTITION == 0) {
		blockorder = 0;
	}
	blockvalidation = 0;

	double partition_page = log(NUMBER_PARTITION) / log(2);
	int partition_p = ceil(partition_page);
	if (partition_p % 8 != 0) {
		partition_p = partition_p / 8 + 1;
	}
	else {
		partition_p = partition_p / 8;
	}

	double cluster_page = log(PAGE_PER_CLUSTER) / log(2);
	int cluster_p = ceil(cluster_page);
	if (cluster_p % 8 != 0) {
		cluster_p = cluster_p / 8 + 1;
	}
	else {
		cluster_p = cluster_p / 8;
	}

	int map_size = NUMBER_PARTITION *(bit + startLPN + startPPN + endPPN + valid + blockorder + partition_p + partition_p); //seq pointer
	//map_size = map_size + (partition_p) * NUMBER_PARTITION;
	map_size = map_size + (8 + partition_p) * NUMBER_STREAM;
	map_size = map_size + NUMBER_CLUSTER * (cluster_p + partition_p);
	*/
	int map_size = 0;
	// Map size with bit
	if (PARTITION_SIZE < 1 * MB) {
		int startLPN = 23;
		int startPPN = 23;
		int endPBN = 16;
		int bitmap = PAGE_PER_PARTITION;
		double valid_page_d = log(PAGE_PER_PARTITION) / log(2);
		int valid_page = ceil(valid_page_d);
		double seq_d = log(NUMBER_PARTITION) / log(2);
		int seq = ceil(seq_d);		// pointer
		int num_partition = 8;

		map_size = (startLPN + startPPN + endPBN + bitmap + valid_page + seq + seq)*NUMBER_PARTITION + (seq + num_partition)*NUMBER_CLUSTER;
		if (map_size % 8 != 0) {
			map_size = map_size / 8 + 1;
		}
		else {
			map_size = map_size / 8;
		}
	}

	if (AGINGFILE != NULL) {
		fprintf(fp, "%s, ", AGINGFILE);
	}
	else {
		fprintf(fp, "s(%f)r(%f m:%f i:%d s:%d), ", SEQ_RATE, RANDOM_RATE, RANDOM_MOUNT, RANDOM_INCREASE, RANDOM_SIZE);

	}

	//fprintf(fp, "FTL, STREAM, WRITE, READ, ERASE, IO_WRITE, IO_READ, B.GC.write, B.GC.read, B.GC.erase, P.GC.write, P.GC.read, MAP_SIZE, #.PA\n");
	fprintf(fp, "%d, ", NUMBER_STREAM);
	fprintf(fp, "%d, ", PARTITION_SIZE);
	fprintf(fp, "%d, ", CLUSTER_SIZE);
	fprintf(fp, "%d, ", NUMBER_PARTITION);
	fprintf(fp, "%d, ", map_size);
	fprintf(fp, "%d, ", COUNT.write + COUNT.partition.gc_write + COUNT.block.gc_write);
	fprintf(fp, "%d, ", COUNT.read + COUNT.partition.gc_read + COUNT.block.gc_read);
	fprintf(fp, "%d, ", COUNT.block.gc);
	fprintf(fp, "%d, ", COUNT.write);
	fprintf(fp, "%d, ", COUNT.read);
	fprintf(fp, "%d, ", COUNT.block.gc_write);
	fprintf(fp, "%d, ", COUNT.block.gc_read);
	fprintf(fp, "%d, ", COUNT.block.gc);
	fprintf(fp, "%d, ", COUNT.partition.gc_write);
	fprintf(fp, "%d, ", COUNT.partition.gc_read);
	fprintf(fp, "%d, ", COUNT.partition.gc);
	fprintf(fp, "%d, ", COUNT.null_partition);
	fprintf(fp, "%d, ", COUNT.IO_mem);
	fprintf(fp, "%d, ", COUNT.IO_mem_M);
	fprintf(fp, "%d, ", COUNT.block.mem);
	fprintf(fp, "%d\n", COUNT.partition.mem);

}



void READ_count(int flag) {
	COUNT.read++;
}

void normal_WRITE_count() {
	COUNT.write++;
}

void WRITE_count(int flag) {
	if (flag == IO_WRITE) {
		normal_WRITE_count();
	}
	else if (flag == PGC) {
		PARTITIONGC_WRITE_count();
	}
	else if (flag == BGC) {
		BLOCKGC_WRITE_count();
	}
}

void PARTITIONGC_count() {
	COUNT.partition.gc++;
}

void BLOCKGC_count() {
	COUNT.block.gc++;
}

void PARTITIONGC_READ_count() {
	COUNT.partition.gc_read++;
}

void PARTITIONGC_WRITE_count() {
	COUNT.partition.gc_write++;
}

void BLOCKGC_READ_count() {
	COUNT.block.gc_read++;
}

void BLOCKGC_WRITE_count() {
	COUNT.block.gc_write++;
}


void MEM_COUNT_IO(int IO_type) {
	if (IO_READ == 1)
		COUNT.IO_mem++;
}

