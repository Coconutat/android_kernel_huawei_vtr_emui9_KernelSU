

#ifndef _CONSTANT
#define _CONSTANT

#include "constant.h"
/********************************
*	Constant variables
*********************************/
/*Constant array length*/
#define INOUT_LEN (9)
#define RTT_LEN (12)
#define IN_LEN (17)
#define OUT_LEN (17)
#define INPKT_LEN (6)
#define DUPACK_LEN (6)
#define OUTPKT_LEN (6)
#define RTS_LEN (6)
#define SYN_LEN (9)

/*in/out*/
static const unsigned char inout_norm[INOUT_LEN] = {
	1, 2, 7, 12, 20, 37, 52, 90, 128
};
static const unsigned short inout_idx[INOUT_LEN] = {
	13, 16, 20, 26, 32, 40, 51, 64, 81
};

/*RTT*/
static const unsigned char rtt_norm[RTT_LEN] = {
	6, 20, 43, 69, 88, 99, 106, 112, 117, 119, 121, 125
};
static const unsigned short rtt_idx[RTT_LEN] = {
	158, 251, 398, 631, 1000, 1585, 2512, 3981, 6310,
	10000, 15849, 25119
};

/*speed in*/
static const unsigned char in_norm[IN_LEN] = {
	8, 13, 18, 30, 39, 46, 56, 64, 81, 88, 93, 97, 101,
	108, 115, 122, 128
};
static const unsigned short in_idx[IN_LEN] = {
	16, 25, 40, 63, 100, 158, 251, 398, 631, 1000, 1585,
	2512, 3981, 6310, 10000, 15849, 25119
};

/*packet speed in*/
static const unsigned char inpkt_norm[INPKT_LEN] = {
	31, 47, 70, 91, 109, 128
};
static const unsigned short inpkt_idx[INPKT_LEN] = {
	2, 3, 4, 6, 10, 16
};

/*out*/
static const unsigned char out_norm[OUT_LEN] = {
	7, 8, 22, 34, 42, 50, 56, 61, 68, 80, 95, 106, 114,
	119, 123, 126, 128
};
static const unsigned short out_idx[OUT_LEN] = {
	16, 25, 40, 63, 100, 158, 251, 398, 631, 1000, 1585,
	2512, 3981, 6310, 10000, 15849, 25119
};

/*dup ack*/
static const unsigned char dupack_norm[DUPACK_LEN] = {
	11, 37, 67, 90, 106, 128
};
static const unsigned short dupack_idx[DUPACK_LEN] = {
	3, 4, 6, 10, 16, 25
};

/*packet speed out*/
static const unsigned char outpkt_norm[OUTPKT_LEN] = {
	27, 46, 72, 95, 111, 128
};
static const unsigned short outpkt_idx[OUTPKT_LEN] = {
	2, 3, 4, 6, 10, 16
};

/*retransmit packet speed*/
static const unsigned char rts_norm[RTS_LEN] = {
	16, 74, 107, 121, 126, 128
};
static const unsigned short rts_idx[RTS_LEN] = {
	3, 4, 6, 10, 16, 25
};

/*syn packet speed*/
static const unsigned char syn_norm[SYN_LEN] = {
	11, 38, 83, 114, 119, 123, 125, 127, 128 

};
static const unsigned short syn_idx[SYN_LEN] = {
	1, 2, 3, 4, 5, 6, 7, 8, 9
};

static const unsigned char coef[9] = {
	5, 100, 27, 2, 2, 8, 2, 5, 31
};

#endif 
