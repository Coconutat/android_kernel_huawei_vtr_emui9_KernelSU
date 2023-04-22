/**
    @copyright: Huawei Technologies Co., Ltd. 2016-xxxx. All rights reserved.

    @file: bfm_hisi_dmd_info_priv.h

    @brief: define the dmd private modules' external public enum/macros/interface for BFM (Boot Fail Monitor)

    @version: 2.0

    @author: QiDechun ID: 216641

    @date: 2016-08-17

    @history:
*/

#ifndef BFM_HISI_DMD_INFO_H
#define BFM_HISI_DMD_INFO_H


/*----c++ support-------------------------------------------------------------------*/

#ifdef __cplusplus
extern "C" {
#endif


/************************************************************
Function:       get_dmd_err_num
Description:   get the most err number from DMD file about boot fail.
Input:           NA
Output:         errNumber     the dmd err number
                    count            the times of err occur 
                    errName       the mod name of this err
Return:         0:   find adapt err number
                    -1: not find.
************************************************************/
int get_dmd_err_num(unsigned int* errNum, unsigned int* count, char* errName);

#ifdef __cplusplus
}
#endif

#endif

