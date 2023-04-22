/* ASN.1 decoder
 *
 * Copyright (C) 2012 Red Hat, Inc. All Rights Reserved.
 * Written by David Howells (dhowells@redhat.com)
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public Licence
 * as published by the Free Software Foundation; either version
 * 2 of the Licence, or (at your option) any later version.
 */

#ifndef _LINUX_ASN1_DECODER_H
#define _LINUX_ASN1_DECODER_H

#include "asn1_ber_bytecode.h"

extern int oases_asn1_ber_decoder(const struct asn1_decoder *decoder,
				void *context, const unsigned char *data, size_t datalen);

#endif /* _LINUX_ASN1_DECODER_H */
