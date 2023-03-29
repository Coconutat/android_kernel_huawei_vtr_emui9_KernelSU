

#ifndef __PSR_DNS_H__
#define __PSR_DNS_H__

/* DNS QType */
enum {
	PSR_DNS_QTYPE_INVALID	= PSR_EN_INVALID,
	PSR_DNS_QTYPE_A         = 1,   /**< a host address  */
	PSR_DNS_QTYPE_AAAA      = 28,  /**< IP6 Address */

	PSR_DNS_QTYPE_END,
	PSR_DNS_QTYPE_BOTTOM	= PSR_EN_BUTT
};

#define PSR_DNS_DOMAIN_NAME_COMPRESSION (0x03)
#define PSR_DNS_DOMAIN_NAME_NO_COMPRESSION (0x00)

extern uint32_t psr_ins_dns(uint16_t proto, uint32_t direction, uint64_t ts,
			    struct psr_pkt_node *pkt_node,
			    struct psr_ctx_data *ctx_data,
			    struct psr_result *result);

#endif/* __PSR_DNS_H__ */
