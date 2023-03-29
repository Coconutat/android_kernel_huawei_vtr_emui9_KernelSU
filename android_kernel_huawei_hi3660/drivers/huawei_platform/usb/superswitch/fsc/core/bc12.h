/* **************************************************************************
 * bc12.h
 *
 * Defines the BC 1.2 spec implementation.
 * ************************************************************************** */
#ifndef FSC_BC12_H_
#define FSC_BC12_H_

#include "platform.h"
#include "port.h"

void ConnectBC12(struct Port *port);
void ProcessBC12(struct Port *port);

#endif /* FSC_BC12_H_ */
