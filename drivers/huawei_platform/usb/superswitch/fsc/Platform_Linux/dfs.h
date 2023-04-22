#ifndef __FUSB_DFS_H_
#define __FUSB_DFS_H_

#include "FSCTypes.h"

/*******************************************************************************
* Function:        fusb_InitializeDFS
* Input:           none
* Return:          0 on success, error code otherwise
* Description:     Initializes methods for using DebugFS.
*******************************************************************************/
FSC_S32 fusb_InitializeDFS(void);

FSC_S32 fusb_DFS_Cleanup(void);

#endif /* __FUSB_DFS_H_ */

