/* Management tests */

#define _GNU_SOURCE

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <string.h>
#include <errno.h>

#include "ioctl.h"
#include <liblightnvm.h>
#include "../CuTest/CuTest.h"

static CuSuite *per_test_suite = NULL;

static void list_info(CuTest *ct)
{
	struct nvm_ioctl_info c;
	int ret, i;

	memset(&c, 0, sizeof(struct nvm_ioctl_info));

	ret = nvm_get_info(&c);
	if (ret) {
		CuAssertTrue(ct, ret == 0);
		return;
	}

	printf("get_info\n");
	printf(" LightNVM version (%u, %u, %u). %u target type(s) registered.\n",
			c.version[0], c.version[1], c.version[2], c.tgtsize);

	for (i = 0; i < c.tgtsize; i++) {
		struct nvm_ioctl_info_tgt *tgt = &c.tgts[i];

		printf("  Type: %s (%u, %u, %u)\n",
				tgt->tgtname, tgt->version[0], tgt->version[1],
				tgt->version[2]);
	}
}

static void get_devices(CuTest *ct)
{
	struct nvm_ioctl_get_devices devs;
	int ret, i;

	ret = nvm_get_devices(&devs);
	if (ret) {
		CuAssertTrue(ct, ret == 0);
		return;
	}

	printf("get_devices\n");
	printf(" nr_devices: %u\n", devs.nr_devices);

	for (i = 0; i < devs.nr_devices && i < 31; i++) {
		struct nvm_ioctl_device_info *info = &devs.info[i];

		printf("  %s: %s (%u, %u, %u)\n", info->devname, info->bmname,
				info->bmversion[0], info->bmversion[1],
				info->bmversion[2]);
	}
}

static void create_tgt(CuTest *ct)
{
	struct nvm_ioctl_create c;
	int ret;

	sprintf(c.dev, "nulln0");
	sprintf(c.tgttype, "rrpc");
	sprintf(c.tgtname, "test");
	c.flags = 0;
	c.conf.type = 0;
	c.conf.s.lun_begin = 0;
	c.conf.s.lun_end = 0;

	ret = nvm_create_target(&c);

	CuAssertTrue(ct, ret == 0);
}

static void remove_tgt(CuTest *ct)
{
	struct nvm_ioctl_remove c;
	int ret;

	sprintf(c.tgtname, "test");
	c.flags = 0;

	ret = nvm_remove_target(&c);

	CuAssertTrue(ct, ret == 0);
}

CuSuite* cli_GetSuite()
{
	per_test_suite = CuSuiteNew();

	SUITE_ADD_TEST(per_test_suite, list_info);
	SUITE_ADD_TEST(per_test_suite, get_devices);
	SUITE_ADD_TEST(per_test_suite, create_tgt);
	SUITE_ADD_TEST(per_test_suite, remove_tgt);

	return per_test_suite;
}


void run_all_test(void)
{
	CuString *output = CuStringNew();
	CuSuite *suite = CuSuiteNew();

	CuSuiteAddSuite(suite, (CuSuite*) cli_GetSuite());

	CuSuiteRun(suite);
	CuSuiteSummary(suite, output);
	CuSuiteDetails(suite, output);
	printf("%s\n", output->buffer);

	CuStringDelete(output);
	CuSuiteDelete(suite);

	free(per_test_suite);
}

int main(void)
{
	run_all_test();
	return 0;
}
