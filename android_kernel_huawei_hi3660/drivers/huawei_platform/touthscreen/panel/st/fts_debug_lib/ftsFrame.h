


#include "ftsSoftware.h"



//Number of data bytes for each node
#define BYTES_PER_NODE						2

#define OFFSET_LENGTH						2

int getOffsetFrame(u16 address, u16 *offset);
int getChannelsLength(void);
int getFrameData(u16 address, int size, short **frame);
int getMSFrame(u16 type, short **frame, int keep_first_row);
int getSSFrame(u16 type, short **frame);
int getNmsFrame(u16 type, short ***frames, int * sizes, int keep_first_row, int fs, int n);
int getSenseLen(void);
int getForceLen(void);
