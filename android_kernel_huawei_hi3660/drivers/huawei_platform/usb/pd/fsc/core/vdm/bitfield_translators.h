/****************************************************************************
 * Company:         Fairchild Semiconductor
 *
 * Author           Date          Comment
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 * G. Noblesmith
 *
 *
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 *
 * Software License Agreement:
 *
 * The software supplied herewith by Fairchild Semiconductor (the “Company”)
 * is supplied to you, the Company's customer, for exclusive use with its
 * USB Type C / USB PD products.  The software is owned by the Company and/or
 * its supplier, and is protected under applicable copyright laws.
 * All rights are reserved. Any use in violation of the foregoing restrictions
 * may subject the user to criminal sanctions under applicable laws, as well
 * as to civil liability for the breach of the terms and conditions of this
 * license.
 *
 * THIS SOFTWARE IS PROVIDED IN AN “AS IS” CONDITION. NO WARRANTIES,
 * WHETHER EXPRESS, IMPLIED OR STATUTORY, INCLUDING, BUT NOT LIMITED
 * TO, IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE APPLY TO THIS SOFTWARE. THE COMPANY SHALL NOT,
 * IN ANY CIRCUMSTANCES, BE LIABLE FOR SPECIAL, INCIDENTAL OR
 * CONSEQUENTIAL DAMAGES, FOR ANY REASON WHATSOEVER.
 *
 *****************************************************************************/
#ifdef FSC_HAVE_VDM

#ifndef __VDM_BITFIELD_TRANSLATORS_H__
#define __VDM_BITFIELD_TRANSLATORS_H__

#include "../platform.h"

#define CABLE_HW_VERSION_OFFSET 28
#define CABLE_FW_VERSION_OFFSET 24
#define VDO_VERSION_OFFSET 21
#define VCONN_FULL_POWER_OFFSET 5
#define CVONN_REQUIREMENT_OFFSET 4
#define VBUS_REQUIREMENT_OFFSET 3
#define USB_SS_SUPP_OFFSET 0

#define CABLE_HW_VERSION_MASK 0x0000000F
#define CABLE_FW_VERSION_MASK 0x0000000F
#define VDO_VERSION_MASK 0x00000007
#define VCONN_FULL_POWER_MASK 0x00000007
#define CVONN_REQUIREMENT_MASK 0x00000001
#define VBUS_REQUIREMENT_MASK 0x00000001
#define USB_SS_SUPP_MASK 0x00000007

/*
 * Functions that convert bits into internal header representations...
 */
	UnstructuredVdmHeader	getUnstructuredVdmHeader(FSC_U32 in);	// converts 32 bits into an unstructured vdm header struct
	StructuredVdmHeader	getStructuredVdmHeader(FSC_U32 in);		// converts 32 bits into a structured vdm header struct
	IdHeader				getIdHeader(FSC_U32 in);					// converts 32 bits into an ID Header struct
	VdmType					getVdmTypeOf(FSC_U32 in);				// returns structured/unstructured vdm type

/*
 * Functions that convert internal header representations into bits...
 */
	FSC_U32		getBitsForUnstructuredVdmHeader(UnstructuredVdmHeader in);	// converts unstructured vdm header struct into 32 bits
	FSC_U32		getBitsForStructuredVdmHeader(StructuredVdmHeader in);		// converts structured vdm header struct into 32 bits
	FSC_U32		getBitsForIdHeader(IdHeader in);							// converts ID Header struct into 32 bits

/*
 * Functions that convert bits into internal VDO representations...
 */
	CertStatVdo			getCertStatVdo(FSC_U32 in);
	ProductVdo				getProductVdo(FSC_U32 in);
	CableVdo				getCableVdo(FSC_U32 in);
	AmaVdo					getAmaVdo(FSC_U32 in);

/*
 * Functions that convert internal VDO representations into bits...
 */
	FSC_U32 getBitsForProductVdo(ProductVdo in);	// converts Product VDO struct into 32 bits
	FSC_U32 getBitsForCertStatVdo(CertStatVdo in);	// converts Cert Stat VDO struct into 32 bits
	FSC_U32	getBitsForCableVdo(CableVdo in);		// converts Cable VDO struct into 32 bits
	FSC_U32	getBitsForAmaVdo(AmaVdo in);			// converts AMA VDO struct into 32 bits

#endif // header guard

#endif // FSC_HAVE_VDM
