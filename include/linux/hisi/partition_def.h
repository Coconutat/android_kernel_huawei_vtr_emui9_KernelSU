#ifndef _PARTITION_DEF_H_
#define _PARTITION_DEF_H_

typedef struct partition {
	char name[PART_NAMELEN];
	unsigned long long start;
	unsigned long long length;
	unsigned int flags;
}PARTITION;

#endif

