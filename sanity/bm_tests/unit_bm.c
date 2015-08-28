#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>

#include "dft_ioctl.h"
#include "../CuTest/CuTest.h"

#define LNVM_DEV_MAX_LEN 11
#define PAGE_SIZE 4096

static CuSuite *per_test_suite = NULL;
static int fd;

static struct dft_block_info vblock_info;
static struct dft_block vblock;

static int lnvm_open_device(const char *lnvm_dev)
{
	char dev_loc[20] = "/dev/";
	int ret;

	strcat(dev_loc, lnvm_dev);
	ret = open(dev_loc, O_RDWR);
	if (ret < 0) {
		printf("Failed to open LightNVM device %s. Error: %d\n",
								dev_loc, ret);
		return -1;
	}

	fd = ret;

	return 0;
}

static int lnvm_get_features()
{
	int ret;

	ret = ioctl(fd, LNVM_GET_NPAGES, &vblock_info.n_pages);
	if (ret)
		perror("Could not obtain number of pages from LightNVM");

	return ret;
}

/**
 * Test1:
 *	- Get block from LightNVM BM
 *	- Write 1 page
 *	- Read 1 page
 *	- Put block to BM
 */
static void test_rw_1(CuTest *ct)
{
	char input_payload[PAGE_SIZE];
	char read_payload[PAGE_SIZE];
	unsigned long i;
	int ret;

	/* get block from lun 0*/
	ret = ioctl(fd, LNVM_GET_BLOCK, &vblock);
	if (ret) {
		perror("Could not get new block from LightNVM BM");
		exit(-1);
	}

	for (i = 0; i < PAGE_SIZE; i++)
		input_payload[i] = i;

	ret = pwrite(fd, input_payload, PAGE_SIZE, vblock.bppa);
	if (ret != PAGE_SIZE) {
		perror("Could not write data to vblock\n");
		exit(-1);
	}

retry:
	ret = pread(fd, read_payload, PAGE_SIZE, vblock.bppa);
	if (ret != PAGE_SIZE) {
		if (errno == EINTR)
			goto retry;

		perror("Could not write data to vblock\n");
		exit(-1);
	}

	CuAssertByteArrayEquals(ct, input_payload, read_payload,
							PAGE_SIZE, PAGE_SIZE);

}

CuSuite* bm_GetSuite()
{
	per_test_suite = CuSuiteNew();

	SUITE_ADD_TEST(per_test_suite, test_rw_1);

	return per_test_suite;
}

void run_all_test(void)
{
	CuString *output = CuStringNew();
	CuSuite *suite = CuSuiteNew();

	CuSuiteAddSuite(suite, (CuSuite*) bm_GetSuite());

	CuSuiteRun(suite);
	CuSuiteSummary(suite, output);
	CuSuiteDetails(suite, output);
	printf("%s\n", output->buffer);

	CuStringDelete(output);
	CuSuiteDelete(suite);
}

static void clean_bm_Suite()
{
	free(per_test_suite);
}

int main(int argc, char **argv)
{
	char lnvm_dev[LNVM_DEV_MAX_LEN];
	int ret;

	if (argc != 2) {
		printf("Usage: %s lnvm_dev / lnvm_dev: LightNVM device\n",
									argv[0]);
		return -1;
	}

	if (strlen(argv[1]) > LNVM_DEV_MAX_LEN) {
		printf("Argument lnvm_dev can be maximum %d characters\n",
							LNVM_DEV_MAX_LEN - 1);
	}

	strcpy(lnvm_dev, argv[1]);

	if (ret = lnvm_open_device(lnvm_dev))
		return ret;

	if (ret = lnvm_get_features())
		return ret;

	run_all_test();
	clean_bm_Suite();

	return 0;
}

