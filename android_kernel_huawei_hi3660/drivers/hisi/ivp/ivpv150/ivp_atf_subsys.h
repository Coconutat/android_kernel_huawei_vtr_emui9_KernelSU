#ifndef _IVP_ATF_H_
#define _IVP_ATF_H_

#define IVP_SLV_SECMODE (0xC500bb00)
#define IVP_MST_SECMODE (0xC500bb01)

enum secmode {
    IVP_SEC = 0,
    IVP_NONSEC
};

extern int ivpatf_change_slv_secmod(unsigned int mode);
extern int ivpatf_change_mst_secmod(unsigned int mode);

#endif
