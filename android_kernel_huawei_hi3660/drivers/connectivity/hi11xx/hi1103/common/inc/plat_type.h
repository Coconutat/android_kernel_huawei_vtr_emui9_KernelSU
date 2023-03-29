
#ifndef __PLAT_TYPE_H__
#define __PLAT_TYPE_H__

/*****************************************************************************
  1 Include other Head file
*****************************************************************************/

/*****************************************************************************
  2 Define macro
*****************************************************************************/
#define STATIC  static

#if (defined(_PRE_PC_LINT) || defined(WIN32))
/* Note: lint -e530 says don't complain about uninitialized variables for
   this varible.  Error 527 has to do with unreachable code.
   -restore restores checking to the -save state */
#define UNREF_PARAM(P)  \
    /*lint -save -e527 -e530 */ \
    { \
        (P) = (P); \
    } \
    /*lint -restore */
#else
#define UNREF_PARAM(P)
#endif

#define PRINT_LINE    PS_PRINT_ERR("%s:%d!\n",__FUNCTION__,__LINE__);
/*****************************************************************************
  3 STRUCT DEFINE
*****************************************************************************/

/*****************************************************************************
  4 EXTERN VARIABLE
*****************************************************************************/

/*****************************************************************************
  5 EXTERN FUNCTION
*****************************************************************************/

/*****************************************************************************
  6 typedef
*****************************************************************************/
typedef unsigned char               uint8;
typedef char                        int8;
typedef unsigned short              uint16;
typedef short                       int16;
typedef unsigned int                uint32;
typedef int                         int32;
typedef unsigned long               uint64;
typedef long                        int64;
//typedef unsigned long               ulong;//use oal_ulong

#endif /* PLAT_TYPE_H */


