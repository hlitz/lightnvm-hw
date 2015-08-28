#ifndef _DFT_IOCTL_H_
#define _DFT_IOCTL_H_

#define LNVM_PUT_BLOCK			21525
#define LNVM_GET_BLOCK			21528
#define LNVM_GET_NPAGES			21530

typedef unsigned long long sector_t;

struct dft_block_info {
	unsigned long n_pages;
};

struct dft_block {
	unsigned long lun;
	sector_t bppa;
	unsigned long id;
	void *internals;
};

#endif //_DFT_IOCTL_H_
