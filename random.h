#ifndef __RANDOM_H__
#define __RANDOM_H__

typedef unsigned int uint32;
typedef unsigned long long uint64;

uint32 BRandom();
void RandomInit(uint32 seed);
double Random();
int IRandom(int min, int max);

#endif
