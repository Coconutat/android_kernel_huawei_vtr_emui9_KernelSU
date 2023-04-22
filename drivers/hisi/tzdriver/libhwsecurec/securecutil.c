/*******************************************************************************
* Copyright @ Huawei Technologies Co., Ltd. 1998-2014. All rights reserved.
* File name: securecutil.c
* Decription:
*             provides internal functions used by this library, such as memory
*             copy and memory move. Besides, include some helper function for
*             printf family API, such as vsnprintf_helper, SECUREC_PUTC_NOLOCK,
*             __putwc_nolock. Also includes some folat converting function, such
*             as cvt, ecvtbuf, fcvtbuf, cfltcvt.
* History:
*     1. Date: 2014/4/10
*        Author: LiShunda
*        Modification: move vswprintf_helper() function from this file to vswprintf.c,
*                      which make this file only contain ANSI string function. This will
*                      facilitate the ANSI string API building.
*     2. Date: 2014/4/10
*        Author: LiShunda
*        Modification: add int putWcharStrEndingZero(SECUREC_XPRINTF_STREAM* str, int zerosNum)
*                      function. In vswprintf.c, the original code use (SECUREC_PUTC_NOLOCK('\0', str) != EOF )
*                      four times and a do-while wrapper which is NOT easy to read and understand,
*                      so abstract this function.
*     3. Date: 2014/4/10
*        Author: LiShunda
*        Modification: change a variabel name "exp" in function "cfltcvt" to "expVal", for "exp" is a function name in <math.h>,
*                      which make pclint warning.
*     4. Date: 2014/4/10
*        Author: LiShunda
*        Modification: remove 'char* s__nullstring = "(null)"' and 'wchar_t* s__wnullstring = L"(null)"' to
*                      to avoid pclint warning on "redundant statement declared symbol's__nullstring'".
*     4. Date: 2014/5/7
*        Author: LiShunda
*        Modification: replace memcpy with memcpy_s at line 558
*     5. Date: 2014/5/20
*        Author: LiShunda
*        Modification: change the return type of function "__putwc_nolock" from unsigned short to
*                      wchar_t.
*     6. Date: 2014/6/4
*        Author: LiShunda
*        Modification: change the function name from "fcvtbuf" to "fcvtbuf_hw", and add "static" modifier.
*                      change the function name from "ecvtbuf" to "ecvtbuf_hw", and add "static" modifier.
*     7. Date: 2014/6/8
*         Author: LiShunda
*         Modification: move the functions of __putwc_nolock, write_char_w, write_multi_char_w
*                       and write_string_w from this file to secureprintoutput_w.c. In ANSI mode build,
*                       these functions are NOT needed, which can reduce code generation size
*                       and let securecutil.c NOT include <wchar.h>.
*     8. Date: 2014/7/1
*         Author:  LiShunda
*         Modification: move "securec__lookuptable_s[]" from this file to output.inl, which is to avoid
*                     redefinition when multiple secure c library are integrated.
********************************************************************************
*/

//#include <math.h>
#include "securec.h"
#include "securecutil.h"
#include "secureprintoutput.h"

#define EOF (-1)
//#include <string.h>
#include <stdarg.h>


/*verNumber<->verStr like:0X502<->SPC002;0X503<->SPC003...;0X510<->SPC010;0X511<->SPC011...*/
void getHwSecureCVersion(char *verStr, int bufSize, unsigned short *verNumber)
{
	if (verStr != NULL && bufSize > 0) {
		(void)strcpy_s(verStr, (size_t)bufSize, "Huawei Secure C V100R001C01SPC003");
	}
	if (verNumber != NULL) {
		*verNumber = (5 << 8 | 3);/*high Num << 8 | num of spc Ver*/
	}
}

void util_memmove(void *dst, const void *src, size_t count)
{
	UINT8T *pDest = (UINT8T *)dst;
	UINT8T *pSrc = (UINT8T *)src;

	if (dst <= src || pDest >= (pSrc + count))    _CHECK_BUFFER_OVERLAP {
		/*
		* Non-Overlapping Buffers
		* copy from lower addresses to higher addresses
		*/
		while (count--) {
			*pDest = *(UINT8T *)pSrc;
			++pDest;
			++pSrc;
		}
	} else {
		/*
		* Overlapping Buffers
		* copy from higher addresses to lower addresses
		*/
		pDest = pDest + count - 1;
		pSrc = pSrc + count - 1;

		while (count--) {
			*pDest = *pSrc;

			--pDest;
			--pSrc;
		}
	}
}
/*put a char to output stream */
#define SECUREC_PUTC_NOLOCK(_c,_stream)    (--(_stream)->_cnt >= 0 ? 0xff & (*(_stream)->_ptr++ = (char)(_c)) :  EOF)

int putWcharStrEndingZero(SECUREC_XPRINTF_STREAM *str, int zerosNum)
{
	int succeed = 0, i = 0;

	for (; i < zerosNum && (SECUREC_PUTC_NOLOCK('\0', str) != EOF); ++i) {
	}
	if (i == zerosNum) {
		succeed = 1;
	}
	return succeed;
}

int vsnprintf_helper(char *string, size_t count, const char *format, va_list arglist)
{
	SECUREC_XPRINTF_STREAM str;
	int retval;

	str._cnt = (int)count;
	str._ptr = string;

	retval = securec_output_s(&str, format, arglist);
	if ((retval >= 0) && (SECUREC_PUTC_NOLOCK('\0', &str) != EOF)) {
		return (retval);
	}

	if (string != NULL) {
		string[count - 1] = 0;
	}

	if (str._cnt < 0) {
		/* the buffer was too small; we return -2 to indicate truncation */
		return -2;
	}

	return -1;
}

/*
remove it */
void write_char_a(char ch, SECUREC_XPRINTF_STREAM *f, int *pnumwritten)
{
	if (SECUREC_PUTC_NOLOCK(ch, f) == EOF) {
		*pnumwritten = -1;
	} else {
		++(*pnumwritten);
	}
}


void write_multi_char_a(char ch, int num, SECUREC_XPRINTF_STREAM *f, int *pnumwritten)
{
	while (num-- > 0) {
		if (SECUREC_PUTC_NOLOCK(ch, f) == EOF) {
			*pnumwritten = -1;
			break;
		} else {
			++(*pnumwritten);
		}
	}
}



void write_string_a(char *string, int len, SECUREC_XPRINTF_STREAM *f, int *pnumwritten)
{
	while (len-- > 0) {
		if (SECUREC_PUTC_NOLOCK(*string, f) == EOF) {
			*pnumwritten = -1;
			break;
		} else {
			++(*pnumwritten);
			++string;
		}
	}
}

/* Following function "U64Div32" realized the operation of division between an unsigned 64-bits
 *     number and an unsigned 32-bits number.
 * these codes are contributed by Dopra team in syslib.
 */
