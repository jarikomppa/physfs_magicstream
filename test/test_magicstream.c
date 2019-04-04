#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "physfs.h"

#ifdef _WIN32
#include <Windows.h>

#define TIMETYPE LARGE_INTEGER

long diff_milli(TIMETYPE *start, TIMETYPE *end)
{
	TIMETYPE Frequency, elapsed;

	QueryPerformanceFrequency(&Frequency);
	elapsed.QuadPart = end->QuadPart - start->QuadPart;

	elapsed.QuadPart *= 1000;
	elapsed.QuadPart /= Frequency.QuadPart;

	return (long)elapsed.QuadPart;
}

#define GETTIMESTAMP(x) QueryPerformanceCounter(&x)
#else
#include <time.h>
#define TIMETYPE struct timespec
long diff_milli(TIMETYPE *start, TIMETYPE *end)
{
	/* ms */
	return ((end->tv_sec * 1000) + (end->tv_nsec / 1000000)) -
		((start->tv_sec * 1000) + (start->tv_nsec / 1000000));
}

#define GETTIMESTAMP(x) clock_gettime(CLOCK_MONOTONIC, &x)
#endif

/* Algorithm "xor" from p. 4 of Marsaglia, "Xorshift RNGs" */
PHYSFS_uint32 randstate = 0xc0cac01a;
PHYSFS_uint32 xorshift32()
{	
	PHYSFS_uint32 x = randstate;
	x ^= x << 13;
	x ^= x >> 17;
	x ^= x << 5;
	randstate = x;
	return x;
}

void make_testdata()
{
	int i;
	PHYSFS_File *f = PHYSFS_openWrite("test.dat");
	for (i = 0; i < 1024 * 1024; i++)
	{
		double a, b;
		a = xorshift32();
		b = xorshift32();
		b *= a;
		PHYSFS_writeBytes(f, &b, sizeof(double));
	}
	PHYSFS_close(f);
}

#define MAX_BYTES (1024)
#define MAX_FILES (1024*10)

int total_open = 0;
int total_bytes = 0;
int total_ops = 0;
unsigned int checksum = 0;

char scratch[MAX_BYTES];

void testfunc()
{
	int i, bytes;
	PHYSFS_File *f = PHYSFS_openRead("test.dat");
	PHYSFS_seek(f, xorshift32() % (1024 * 1024));
	total_open++;
	total_ops++;

	int action = 0;
	int nextaction = 0;

	do
	{
		switch (action)
		{
		default:
			bytes = xorshift32() % MAX_BYTES;
			nextaction = 0;
			PHYSFS_readBytes(f, scratch, bytes);
			for (i = 0; i < bytes; i++)
				nextaction += scratch[i];
			total_bytes += bytes;
			total_ops++;
			//printf("\r%d bytes, %d files   ", total_bytes, total_open);
			break;
		case 1:
			nextaction = (int)PHYSFS_tell(f);
			total_ops++;
			break;
		case 2:
			nextaction = 0;
			PHYSFS_seek(f, PHYSFS_tell(f) + (xorshift32() % 100) - 50);
			total_ops++;
			total_ops++;
			break;
		case 3:
			nextaction = 0;
			testfunc();
			break;
		case 4:
			nextaction = 0;
			PHYSFS_flush(f);
			total_ops++;
			break;
		case 5:
			nextaction = (int)(PHYSFS_fileLength(f) - PHYSFS_tell(f));
			total_ops++;
			total_ops++;
			break;
		case 6:
			nextaction = 0;
			PHYSFS_eof(f);
			total_ops++;
			break;
		}
		checksum += nextaction;
		nextaction %= 7;
		if (nextaction == action)
			nextaction = 0;
		action = nextaction;

	} while (action != 4 && total_open < MAX_FILES);

	PHYSFS_close(f);
	total_ops++;
}

void run_test()
{
	randstate = 0xc0cac01a;
	total_open = 0;
	total_bytes = 0;
	total_ops = 0;
	checksum = 0;
	while (total_open < MAX_FILES)
		testfunc();
}

int main(int parc, char ** pars)
{
	TIMETYPE time_a, time_b;
	int i;
	PHYSFS_init(pars[0]);
	PHYSFS_setWriteDir(".");
	PHYSFS_mount(".", "", 0);

	printf("Generating test data file..\t");
	GETTIMESTAMP(time_a);
	make_testdata();
	GETTIMESTAMP(time_b);
	printf("Generation took %d milliseconds.\n", diff_milli(&time_a, &time_b));
	printf("Generating magic stream..\t");
	GETTIMESTAMP(time_a);
	PHYSFS_File *sf = PHYSFS_openWrite("test.magicstream");
	PHYSFS_createMagicStream(sf);
	run_test();
	PHYSFS_closeMagicStream();
	GETTIMESTAMP(time_b);
	printf("Generation took %d milliseconds.\n", diff_milli(&time_a, &time_b));
	printf("\nTesting..\n\n");
	int total_with = 0;
	int total_without = 0;
	for (i = 0; i < 8; i++)
	{
		printf("Running test without magic stream..\t");
		GETTIMESTAMP(time_a);
		run_test();
		GETTIMESTAMP(time_b);
		total_without += diff_milli(&time_a, &time_b);
		printf("[%08x] Test took %5d milliseconds. (%d file operations)\n", checksum, diff_milli(&time_a, &time_b), total_ops);

		printf("Running test with magic stream..\t");
		GETTIMESTAMP(time_a);
		sf = PHYSFS_openRead("test.magicstream");
		PHYSFS_openMagicStream(sf);
		run_test();
		PHYSFS_closeMagicStream();
		GETTIMESTAMP(time_b);
		total_with += diff_milli(&time_a, &time_b);
		printf("[%08x] Test took %5d milliseconds. (%d file operations)\n", checksum, diff_milli(&time_a, &time_b), total_ops);
	}
	printf("total without: \t%d\n", total_without);
	printf("total with: \t%d\n", total_with);

	PHYSFS_deinit();
	return 0;
}