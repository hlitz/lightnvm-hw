#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>

#include "ioctl.h"
#include "../CuTest/CuTest.h"

#define NVM_DEV_MAX_LEN 11
#define PAGE_SIZE 4096

static CuSuite *per_test_suite = NULL;
static int fd;

static struct vlun_info vlun_info;
static struct nvm_ioctl_vblock vblock;

static int lnvm_open_device(const char *lnvm_dev)
{
	char dev_loc[20] = "/dev/";
	int ret;

	strcat(dev_loc, lnvm_dev);
	ret = open(dev_loc, O_RDWR | O_DIRECT);
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

	ret = ioctl(fd, NVM_DEV_NBLOCKS_LUN, &vlun_info.n_vblocks);
	if (ret)
		perror("Cloud not obtain number of vblocks in LUN");

	ret = ioctl(fd, NVM_DEV_NPAGES_BLOCK, &vlun_info.n_pages_per_blk);
	if (ret)
		perror("Could not obtain number of pages from LightNVM");

	printf("BM UNIT TESTS: \n"
		"\t- Number of vblocks: %lu\n"
		"\t- Number of pages per block: %lu\n",
		vlun_info.n_vblocks, vlun_info.n_pages_per_blk);
	return ret;
}

/**
 * Test1:
 *	- Get block from LightNVM BM
 *	- Write first page
 *	- Read first page back
 *	- Put block to BM
 */
static void test_rw_1(CuTest *ct)
{
	char *input_payload;
	char *read_payload;
	unsigned long i;
	int ret;

	printf("Test1...");

	/* get block from lun 0*/
	vblock.vlun_id = 0;

	/* get block from lun 0*/
	ret = ioctl(fd, NVM_PR_GET_BLOCK, &vblock);
	if (ret) {
		perror("Could not get new block from LightNVM BM");
		exit(-1);
	}

	/* Allocate aligned memory to use direct IO */
	input_payload = (char*)memalign(PAGE_SIZE, PAGE_SIZE);
	if (!input_payload) {
		perror("Could not allocate alligned memory\n");
		exit(-1);
	}

	read_payload = (char*)memalign(PAGE_SIZE, PAGE_SIZE);
	if (!read_payload) {
		perror("Could not allocate alligned memory\n");
		exit(-1);
	}

	for (i = 0; i < PAGE_SIZE; i++)
		input_payload[i] = i;

	printf("Writing to block %llu - starting ppa: %llu, position: %lu\n",
						vblock.id, vblock.bppa, i);
	ret = pwrite(fd, input_payload, PAGE_SIZE, vblock.bppa * 4096);
	if (ret != PAGE_SIZE) {
		perror("Could not write data to vblock\n");
		exit(-1);
	}

retry:
	ret = pread(fd, read_payload, PAGE_SIZE, vblock.bppa * 4096);
	if (ret != PAGE_SIZE) {
		if (errno == EINTR)
			goto retry;

		perror("Could not write data to vblock\n");
		exit(-1);
	}

	CuAssertByteArrayEquals(ct, input_payload, read_payload,
							PAGE_SIZE, PAGE_SIZE);

	ret = ioctl(fd, NVM_PR_PUT_BLOCK, &vblock);
	if (ret) {
		perror("Could not put block to LightNVM BM");
		exit(-1);
	}

	free(input_payload);
	free(read_payload);

	printf("DONE\n");
}

/**
 * Test2:
 *	- Write and Read all blocks from LightNVM BM
 *	- Reads and Writes occur at PAGE_SIZE granurality
 *
 * This test assumes that all blocks are free. Other tests executed before this
 * tests must put all blocks back to the BM
 */
static void test_rw_2(CuTest *ct)
{
	char *input_payload;
	char *read_payload;
	unsigned long *block_ids;
	unsigned long n_left_blocks = 0;
	unsigned long i, j;
	int ret;

	printf("Test2...");

	ret = ioctl(fd, NVM_DEV_NFREE_BLOCKS, &n_left_blocks);
	if (ret) {
		perror("Could not obtain number of vblocks in LUN");
		exit(-1);
	}

	CuAssertIntEquals(ct, n_left_blocks, vlun_info.n_vblocks);

	block_ids = malloc(n_left_blocks * sizeof(unsigned long));
	if (!block_ids) {
		perror("Could not allocate memory for test");
		exit(-1);
	}

	/* Allocate aligned memory to use direct IO */
	input_payload = (char*)memalign(PAGE_SIZE, PAGE_SIZE);
	if (!input_payload) {
		perror("Could not allocate alligned memory\n");
		exit(-1);
	}

	read_payload = (char*)memalign(PAGE_SIZE, PAGE_SIZE);
	if (!read_payload) {
		perror("Could not allocate alligned memory\n");
		exit(-1);
	}

	for (i = 0; i < vlun_info.n_vblocks; i++) {
		/* get block from lun 0*/
		vblock.vlun_id = 0;
		ret = ioctl(fd, NVM_PR_GET_BLOCK, &vblock);
		if (ret) {
			perror("Could not get new block from LightNVM BM");
			exit(-1);
		}

		block_ids[i] = vblock.id;
		printf("Lun: %d, Writing to block %llu - starting ppa: %llu, position: %lu\n",
				vblock.vlun_id, vblock.id, vblock.bppa, i);

		for (j = 0; j < vlun_info.n_pages_per_blk; j++) {
			memset(input_payload, j + i, PAGE_SIZE);

			ret = pwrite(fd, input_payload, PAGE_SIZE,
						(vblock.bppa + j) * 4096);
			if (ret != PAGE_SIZE) {
				perror("Could not write data to vblock\n");
			exit(-1);
			}

retry:
			ret = pread(fd, read_payload, PAGE_SIZE,
						(vblock.bppa + j) * 4096);
			if (ret != PAGE_SIZE) {
				if (errno == EINTR)
					goto retry;

				perror("Could not read data from vblock\n");
				exit(-1);
			}

			CuAssertByteArrayEquals(ct, input_payload, read_payload,
							PAGE_SIZE, PAGE_SIZE);

			memset(read_payload, 0, PAGE_SIZE);
		}
	}

	n_left_blocks = 0;
	ret = ioctl(fd, NVM_DEV_NFREE_BLOCKS, &n_left_blocks);
	if (ret)
		perror("Cloud not obtain number of vblocks in LUN");
	CuAssertIntEquals(ct, 0, n_left_blocks);

	for (i = 0; i < vlun_info.n_vblocks; i++) {
		vblock.vlun_id = 0;
		vblock.id = block_ids[i];

		ret = ioctl(fd, NVM_PR_PUT_BLOCK, &vblock);
		if (ret) {
			perror("Could not put block to LightNVM BM");
			exit(-1);
		}
	}

	n_left_blocks = 0;
	ret = ioctl(fd, NVM_DEV_NFREE_BLOCKS, &n_left_blocks);
	if (ret)
		perror("Cloud not obtain number of vblocks in LUN");

	CuAssertIntEquals(ct, vlun_info.n_vblocks, n_left_blocks);

	free(block_ids);
	free(input_payload);
	free(read_payload);
	printf("DONE\n");
}

CuSuite* bm_GetSuite()
{
	per_test_suite = CuSuiteNew();

	SUITE_ADD_TEST(per_test_suite, test_rw_1);
	SUITE_ADD_TEST(per_test_suite, test_rw_2);

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
	char lnvm_dev[NVM_DEV_MAX_LEN];
	int ret;

	if (argc != 2) {
		printf("Usage: %s lnvm_dev / lnvm_dev: LightNVM device\n",
									argv[0]);
		return -1;
	}

	if (strlen(argv[1]) > NVM_DEV_MAX_LEN) {
		printf("Argument lnvm_dev can be maximum %d characters\n",
							NVM_DEV_MAX_LEN - 1);
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

