#include "random.h"


//typedef unsigned int uint32;
//typedef unsigned long long uint64;

uint32 x[5];

uint32 BRandom() {
	uint64 sum;
	sum = (uint64)2111111111 * (uint64)x[3] +
		(uint64)1492 * (uint64)(x[2]) +
		(uint64)1776 * (uint64)(x[1]) +
		(uint64)5115 * (uint64)(x[0]) +
		(uint64)x[4];
	x[3] = x[2];  x[2] = x[1];  x[1] = x[0];
	x[4] = (uint32)(sum >> 32);            // Carry
	x[0] = (uint32)(sum);                  // Low 32 bits of sum
	return x[0];
}


void RandomInit(uint32 seed) {
	int i;
	uint32 s = seed;
	// make random numbers and put them into the buffer
	for (i = 0; i < 5; i++) {
		s = s * 29943829 - 1;
		x[i] = s;
	}
	// randomize some more
	for (i = 0; i<19; i++) BRandom();
}


// returns a random number between 0 and 1:
double Random() {
	return (double)BRandom() * (1. / (65536.*65536.));
}


int IRandom(int min, int max) {
	int r;
	// Output random integer in the interval min <= x <= max
	// Relative error on frequencies < 2^-32
	if (max <= min) {
		if (max == min) return min; else return 0x80000000;
	}
	// Multiply interval with random and truncate
	r = (int)((max - min + 1) * Random()) + min;
	if (r > max) r = max;
	return r;
}

#if 0
int main()
{
	if ((result = fopen("result.txt", "w")) == NULL) {
		printf("file was not opened\n"); exit(1);
	}

	RandomInit(1);

	int j = 1;
	for (int i = 0; i < 1000; i++) {
		int rand = IRandom(0, 100);

		if (rand < 1000)	/*cout << "_index: " << j++ << " rand num: " << rand << endl;*/
		{
			fprintf(result, "%d\t%d\n", j++, rand);
		}
	}

	return 0;
}
#endif



