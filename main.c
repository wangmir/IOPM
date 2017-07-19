#include "main.h"
#include "iopm.h"
#include "init.h"
#include "random.h"
#include "error.h"

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
	close_streamGC = 0;
	close_blockGC = 0;

	/* Aging */
	if (AGINGFILE == NULL){
		// seq
		printf("[main] SEQ WRITE\n");
		for (i = 0; i < LOGICAL_FLASH_SIZE / PAGE_SIZE * SEQ_RATE; i++) {

			write(i, 1);
			AGING_IO++; 

			if (AGING_IO % 10000 == 1) {
				printf("-");
			}
		}
		
		// debug
		check_validity();
		check_total_MAP();

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

	// debug
	check_validity();
	check_total_MAP();

	//fopen_s(&fp, file, "r");
	fp = fopen(file, "r");
	trace_total_write = 0;
	count_init();
	break_GC = 0;
	close_blockGC = 0;
	close_streamGC = 0;
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
	NUMBER_STREAM = 32;

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

	for (i = 0; i<argc; i++) {

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
	printf("[DEBUG] NUMBER_PARTITION : %d K\n", NUMBER_PARTITION/K);
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
	printf("[DEBUG] RANDOM_SIZE : %d\n", RANDOM_SIZE);

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
	memset(&stat, 0x00, sizeof(_STAT));
}

void print_count(char * file, int trace_total_write) {
	printf("printf_count\n");
	FILE *fp;
	char file_name[1024];
	char * extension = ".txt";
	char * underbar = "_result";

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
		fprintf(fp, "Aging, NUM_STREAM, PA_Size, Cluster_Size, PA#, MAP_SIZE, ");
		fprintf(fp, "NAND_WRITE, ");
		fprintf(fp, "NAND_READ, ");
		fprintf(fp, "NAND_ERASE, ");

		fprintf(fp, "IO_WRITE, ");
		fprintf(fp, "IO_READ, ");
		fprintf(fp, "IO_OVERWRITE, ");
		fprintf(fp, "IO_NULLREAD, ");
		fprintf(fp, "IO_WRITE_REQ, ");
		fprintf(fp, "IO_READ_REQ, ");

		fprintf(fp, "BGC_WRITE, ");
		fprintf(fp, "BGC_READ, ");
		fprintf(fp, "BGC_ERASE, ");
		fprintf(fp, "BGC_CNT, ");

		fprintf(fp, "PGC_WRITE, ");
		fprintf(fp, "PGC_READ, ");
		fprintf(fp, "PGC_ERASE, ");
		fprintf(fp, "PGC_CNT, ");
		fprintf(fp, "PGC_VICTIM\n");
	}
	else {
		fclose(fp);
		fp = fopen(file_name, "a+");
		//fopen_s(&fp, file_name, "a+");
	}

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

	fprintf(fp, "%d, ", NUMBER_STREAM);
	fprintf(fp, "%d, ", PARTITION_SIZE);
	fprintf(fp, "%d, ", CLUSTER_SIZE);
	fprintf(fp, "%d, ", NUMBER_PARTITION);
	fprintf(fp, "%d, ", map_size);
	fprintf(fp, "%d, ", get_count(prof_NAND_write));
	fprintf(fp, "%d, ", get_count(prof_NAND_read));
	fprintf(fp, "%d, ", get_count(prof_NAND_erase));

	fprintf(fp, "%d, ", get_count(prof_IO_write));
	fprintf(fp, "%d, ", get_count(prof_IO_read));
	fprintf(fp, "%d, ", get_count(prof_IO_overwrite));
	fprintf(fp, "%d, ", get_count(prof_IO_nullread));
	fprintf(fp, "%d, ", get_count(prof_IO_write_req));
	fprintf(fp, "%d, ", get_count(prof_IO_read_req));

	fprintf(fp, "%d, ", get_count(prof_BGC_write));
	fprintf(fp, "%d, ", get_count(prof_BGC_read));
	fprintf(fp, "%d, ", get_count(prof_BGC_erase));
	fprintf(fp, "%d, ", get_count(prof_BGC_cnt));

	fprintf(fp, "%d, ", get_count(prof_PGC_write));
	fprintf(fp, "%d, ", get_count(prof_PGC_read));
	fprintf(fp, "%d, ", get_count(prof_PGC_erase));
	fprintf(fp, "%d, ", get_count(prof_PGC_cnt));
	fprintf(fp, "%d, ", get_count(prof_PGC_victim));

}

static int* return_cnt_ptr(int flag) {

	int *pcnt = NULL;

	switch (flag) {
	// NAND
	case prof_NAND_write:
		pcnt = &stat.nand.write;
		break;

	case prof_NAND_read:
		pcnt = &stat.nand.read;
		break;

	case prof_NAND_erase:
		pcnt = &stat.nand.erase;
		break;

	// IO
	case prof_IO_write:
		pcnt = &stat.io.nand.write;
		break;

	case prof_IO_read:
		pcnt = &stat.io.nand.read;
		break;

	case prof_IO_overwrite:
		pcnt = &stat.io.overwrite;
		break;

	case prof_IO_nullread:
		pcnt = &stat.io.null_read;
		break;

	case prof_IO_write_req:
		pcnt = &stat.io.write_req;
		break;

	case prof_IO_read_req:
		pcnt = &stat.io.read_req;
		break;
	
	// PGC
	case prof_PGC_cnt:
		pcnt = &stat.pgc.pgc_cnt;
		break;

	case prof_PGC_victim:
		pcnt = &stat.pgc.num_victim_partition;
		break;

	case prof_PGC_free:
		pcnt = &stat.pgc.partition_free_cnt;
		break;

	case prof_PGC_write:
		pcnt = &stat.pgc.nand.write;
		break;

	case prof_PGC_read:
		pcnt = &stat.pgc.nand.read;
		break;

	case prof_PGC_erase:
		pcnt = &stat.pgc.nand.erase;
		break;

	// BGC
	case prof_BGC_cnt:
		pcnt = &stat.bgc.bgc_cnt;
		break;

	case prof_BGC_write:
		pcnt = &stat.bgc.nand.write;
		break;

	case prof_BGC_read:
		pcnt = &stat.bgc.nand.read;
		break;

	case prof_BGC_erase:
		pcnt = &stat.bgc.nand.erase;
		break;

	default:
		assert(0);
		break;
	}

	return pcnt;
}

void do_count(int flag, int cnt)
{
	int *pcnt;

	pcnt = return_cnt_ptr(flag);

	*pcnt += cnt;
}

int get_count(int flag) {

	int *pcnt;

	pcnt = return_cnt_ptr(flag);

	return *pcnt;
}