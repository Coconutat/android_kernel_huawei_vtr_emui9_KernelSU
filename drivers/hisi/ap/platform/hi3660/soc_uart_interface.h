#ifndef __SOC_UART_INTERFACE_H__
#define __SOC_UART_INTERFACE_H__ 
#ifdef __cplusplus
    #if __cplusplus
        extern "C" {
    #endif
#endif
#define SOC_UART_UARTDR_ADDR(base) ((base) + (0x000))
#define SOC_UART_UARTRSR_UARTECR_ADDR(base) ((base) + (0x004))
#define SOC_UART_UARTFR_ADDR(base) ((base) + (0x018))
#define SOC_UART_UARTLPR_ADDR(base) ((base) + (0x020))
#define SOC_UART_UARTIBRD_ADDR(base) ((base) + (0x024))
#define SOC_UART_UARTFBRD_ADDR(base) ((base) + (0x028))
#define SOC_UART_UARTLCR_H_ADDR(base) ((base) + (0x02C))
#define SOC_UART_UARTCR_ADDR(base) ((base) + (0x030))
#define SOC_UART_UARTIFLS_ADDR(base) ((base) + (0x034))
#define SOC_UART_UARTIMSC_ADDR(base) ((base) + (0x038))
#define SOC_UART_UARTRIS_ADDR(base) ((base) + (0x03C))
#define SOC_UART_UARTMIS_ADDR(base) ((base) + (0x040))
#define SOC_UART_UARTICR_ADDR(base) ((base) + (0x044))
#define SOC_UART_UARTDMACR_ADDR(base) ((base) + (0x048))
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned short value;
    struct
    {
        unsigned short data : 8;
        unsigned short fe : 1;
        unsigned short pe : 1;
        unsigned short be : 1;
        unsigned short oe : 1;
        unsigned short reserved : 4;
    } reg;
} SOC_UART_UARTDR_UNION;
#endif
#define SOC_UART_UARTDR_data_START (0)
#define SOC_UART_UARTDR_data_END (7)
#define SOC_UART_UARTDR_fe_START (8)
#define SOC_UART_UARTDR_fe_END (8)
#define SOC_UART_UARTDR_pe_START (9)
#define SOC_UART_UARTDR_pe_END (9)
#define SOC_UART_UARTDR_be_START (10)
#define SOC_UART_UARTDR_be_END (10)
#define SOC_UART_UARTDR_oe_START (11)
#define SOC_UART_UARTDR_oe_END (11)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned short value;
    struct
    {
        unsigned short fe : 1;
        unsigned short pe : 1;
        unsigned short be : 1;
        unsigned short oe : 1;
        unsigned short reserved_0: 1;
        unsigned short cer : 3;
        unsigned short reserved_1: 8;
    } reg;
} SOC_UART_UARTRSR_UARTECR_UNION;
#endif
#define SOC_UART_UARTRSR_UARTECR_fe_START (0)
#define SOC_UART_UARTRSR_UARTECR_fe_END (0)
#define SOC_UART_UARTRSR_UARTECR_pe_START (1)
#define SOC_UART_UARTRSR_UARTECR_pe_END (1)
#define SOC_UART_UARTRSR_UARTECR_be_START (2)
#define SOC_UART_UARTRSR_UARTECR_be_END (2)
#define SOC_UART_UARTRSR_UARTECR_oe_START (3)
#define SOC_UART_UARTRSR_UARTECR_oe_END (3)
#define SOC_UART_UARTRSR_UARTECR_cer_START (5)
#define SOC_UART_UARTRSR_UARTECR_cer_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned short value;
    struct
    {
        unsigned short cts : 1;
        unsigned short reserved_0: 2;
        unsigned short busy : 1;
        unsigned short rxfe : 1;
        unsigned short txff : 1;
        unsigned short rxff : 1;
        unsigned short txfe : 1;
        unsigned short reserved_1: 8;
    } reg;
} SOC_UART_UARTFR_UNION;
#endif
#define SOC_UART_UARTFR_cts_START (0)
#define SOC_UART_UARTFR_cts_END (0)
#define SOC_UART_UARTFR_busy_START (3)
#define SOC_UART_UARTFR_busy_END (3)
#define SOC_UART_UARTFR_rxfe_START (4)
#define SOC_UART_UARTFR_rxfe_END (4)
#define SOC_UART_UARTFR_txff_START (5)
#define SOC_UART_UARTFR_txff_END (5)
#define SOC_UART_UARTFR_rxff_START (6)
#define SOC_UART_UARTFR_rxff_END (6)
#define SOC_UART_UARTFR_txfe_START (7)
#define SOC_UART_UARTFR_txfe_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned short value;
    struct
    {
        unsigned short lrda : 8;
        unsigned short reserved : 8;
    } reg;
} SOC_UART_UARTLPR_UNION;
#endif
#define SOC_UART_UARTLPR_lrda_START (0)
#define SOC_UART_UARTLPR_lrda_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned short value;
    struct
    {
        unsigned short baud_divint : 16;
    } reg;
} SOC_UART_UARTIBRD_UNION;
#endif
#define SOC_UART_UARTIBRD_baud_divint_START (0)
#define SOC_UART_UARTIBRD_baud_divint_END (15)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned short value;
    struct
    {
        unsigned short baud_divfrac : 6;
        unsigned short reserved : 10;
    } reg;
} SOC_UART_UARTFBRD_UNION;
#endif
#define SOC_UART_UARTFBRD_baud_divfrac_START (0)
#define SOC_UART_UARTFBRD_baud_divfrac_END (5)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned short value;
    struct
    {
        unsigned short brk : 1;
        unsigned short pen : 1;
        unsigned short eps : 1;
        unsigned short stp2 : 1;
        unsigned short fen : 1;
        unsigned short wlen : 2;
        unsigned short sps : 1;
        unsigned short reserved : 8;
    } reg;
} SOC_UART_UARTLCR_H_UNION;
#endif
#define SOC_UART_UARTLCR_H_brk_START (0)
#define SOC_UART_UARTLCR_H_brk_END (0)
#define SOC_UART_UARTLCR_H_pen_START (1)
#define SOC_UART_UARTLCR_H_pen_END (1)
#define SOC_UART_UARTLCR_H_eps_START (2)
#define SOC_UART_UARTLCR_H_eps_END (2)
#define SOC_UART_UARTLCR_H_stp2_START (3)
#define SOC_UART_UARTLCR_H_stp2_END (3)
#define SOC_UART_UARTLCR_H_fen_START (4)
#define SOC_UART_UARTLCR_H_fen_END (4)
#define SOC_UART_UARTLCR_H_wlen_START (5)
#define SOC_UART_UARTLCR_H_wlen_END (6)
#define SOC_UART_UARTLCR_H_sps_START (7)
#define SOC_UART_UARTLCR_H_sps_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned short value;
    struct
    {
        unsigned short uarten : 1;
        unsigned short siren : 1;
        unsigned short sirlp : 1;
        unsigned short reserved_0: 4;
        unsigned short lbe : 1;
        unsigned short txe : 1;
        unsigned short rxe : 1;
        unsigned short reserved_1: 1;
        unsigned short rts : 1;
        unsigned short reserved_2: 2;
        unsigned short rtsen : 1;
        unsigned short ctsen : 1;
    } reg;
} SOC_UART_UARTCR_UNION;
#endif
#define SOC_UART_UARTCR_uarten_START (0)
#define SOC_UART_UARTCR_uarten_END (0)
#define SOC_UART_UARTCR_siren_START (1)
#define SOC_UART_UARTCR_siren_END (1)
#define SOC_UART_UARTCR_sirlp_START (2)
#define SOC_UART_UARTCR_sirlp_END (2)
#define SOC_UART_UARTCR_lbe_START (7)
#define SOC_UART_UARTCR_lbe_END (7)
#define SOC_UART_UARTCR_txe_START (8)
#define SOC_UART_UARTCR_txe_END (8)
#define SOC_UART_UARTCR_rxe_START (9)
#define SOC_UART_UARTCR_rxe_END (9)
#define SOC_UART_UARTCR_rts_START (11)
#define SOC_UART_UARTCR_rts_END (11)
#define SOC_UART_UARTCR_rtsen_START (14)
#define SOC_UART_UARTCR_rtsen_END (14)
#define SOC_UART_UARTCR_ctsen_START (15)
#define SOC_UART_UARTCR_ctsen_END (15)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned short value;
    struct
    {
        unsigned short txiflsel : 3;
        unsigned short rxiflsel : 3;
        unsigned short rtsflsel : 3;
        unsigned short reserved : 7;
    } reg;
} SOC_UART_UARTIFLS_UNION;
#endif
#define SOC_UART_UARTIFLS_txiflsel_START (0)
#define SOC_UART_UARTIFLS_txiflsel_END (2)
#define SOC_UART_UARTIFLS_rxiflsel_START (3)
#define SOC_UART_UARTIFLS_rxiflsel_END (5)
#define SOC_UART_UARTIFLS_rtsflsel_START (6)
#define SOC_UART_UARTIFLS_rtsflsel_END (8)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned short value;
    struct
    {
        unsigned short reserved_0: 1;
        unsigned short ctsmim : 1;
        unsigned short reserved_1: 2;
        unsigned short rxim : 1;
        unsigned short txim : 1;
        unsigned short rtim : 1;
        unsigned short feim : 1;
        unsigned short peim : 1;
        unsigned short beim : 1;
        unsigned short oeim : 1;
        unsigned short reserved_2: 5;
    } reg;
} SOC_UART_UARTIMSC_UNION;
#endif
#define SOC_UART_UARTIMSC_ctsmim_START (1)
#define SOC_UART_UARTIMSC_ctsmim_END (1)
#define SOC_UART_UARTIMSC_rxim_START (4)
#define SOC_UART_UARTIMSC_rxim_END (4)
#define SOC_UART_UARTIMSC_txim_START (5)
#define SOC_UART_UARTIMSC_txim_END (5)
#define SOC_UART_UARTIMSC_rtim_START (6)
#define SOC_UART_UARTIMSC_rtim_END (6)
#define SOC_UART_UARTIMSC_feim_START (7)
#define SOC_UART_UARTIMSC_feim_END (7)
#define SOC_UART_UARTIMSC_peim_START (8)
#define SOC_UART_UARTIMSC_peim_END (8)
#define SOC_UART_UARTIMSC_beim_START (9)
#define SOC_UART_UARTIMSC_beim_END (9)
#define SOC_UART_UARTIMSC_oeim_START (10)
#define SOC_UART_UARTIMSC_oeim_END (10)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned short value;
    struct
    {
        unsigned short reserved_0: 1;
        unsigned short ctsmris : 1;
        unsigned short reserved_1: 2;
        unsigned short rxris : 1;
        unsigned short txris : 1;
        unsigned short rtris : 1;
        unsigned short feris : 1;
        unsigned short peris : 1;
        unsigned short beris : 1;
        unsigned short oeris : 1;
        unsigned short reserved_2: 5;
    } reg;
} SOC_UART_UARTRIS_UNION;
#endif
#define SOC_UART_UARTRIS_ctsmris_START (1)
#define SOC_UART_UARTRIS_ctsmris_END (1)
#define SOC_UART_UARTRIS_rxris_START (4)
#define SOC_UART_UARTRIS_rxris_END (4)
#define SOC_UART_UARTRIS_txris_START (5)
#define SOC_UART_UARTRIS_txris_END (5)
#define SOC_UART_UARTRIS_rtris_START (6)
#define SOC_UART_UARTRIS_rtris_END (6)
#define SOC_UART_UARTRIS_feris_START (7)
#define SOC_UART_UARTRIS_feris_END (7)
#define SOC_UART_UARTRIS_peris_START (8)
#define SOC_UART_UARTRIS_peris_END (8)
#define SOC_UART_UARTRIS_beris_START (9)
#define SOC_UART_UARTRIS_beris_END (9)
#define SOC_UART_UARTRIS_oeris_START (10)
#define SOC_UART_UARTRIS_oeris_END (10)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned short value;
    struct
    {
        unsigned short reserved_0: 1;
        unsigned short ctsmmis : 1;
        unsigned short reserved_1: 2;
        unsigned short rxmis : 1;
        unsigned short txmis : 1;
        unsigned short rtmis : 1;
        unsigned short femis : 1;
        unsigned short pemis : 1;
        unsigned short bemis : 1;
        unsigned short oemis : 1;
        unsigned short reserved_2: 5;
    } reg;
} SOC_UART_UARTMIS_UNION;
#endif
#define SOC_UART_UARTMIS_ctsmmis_START (1)
#define SOC_UART_UARTMIS_ctsmmis_END (1)
#define SOC_UART_UARTMIS_rxmis_START (4)
#define SOC_UART_UARTMIS_rxmis_END (4)
#define SOC_UART_UARTMIS_txmis_START (5)
#define SOC_UART_UARTMIS_txmis_END (5)
#define SOC_UART_UARTMIS_rtmis_START (6)
#define SOC_UART_UARTMIS_rtmis_END (6)
#define SOC_UART_UARTMIS_femis_START (7)
#define SOC_UART_UARTMIS_femis_END (7)
#define SOC_UART_UARTMIS_pemis_START (8)
#define SOC_UART_UARTMIS_pemis_END (8)
#define SOC_UART_UARTMIS_bemis_START (9)
#define SOC_UART_UARTMIS_bemis_END (9)
#define SOC_UART_UARTMIS_oemis_START (10)
#define SOC_UART_UARTMIS_oemis_END (10)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned short value;
    struct
    {
        unsigned short reserved_0: 1;
        unsigned short ctsic : 1;
        unsigned short reserved_1: 2;
        unsigned short rxic : 1;
        unsigned short txic : 1;
        unsigned short rtic : 1;
        unsigned short feic : 1;
        unsigned short peic : 1;
        unsigned short beic : 1;
        unsigned short oeic : 1;
        unsigned short reserved_2: 5;
    } reg;
} SOC_UART_UARTICR_UNION;
#endif
#define SOC_UART_UARTICR_ctsic_START (1)
#define SOC_UART_UARTICR_ctsic_END (1)
#define SOC_UART_UARTICR_rxic_START (4)
#define SOC_UART_UARTICR_rxic_END (4)
#define SOC_UART_UARTICR_txic_START (5)
#define SOC_UART_UARTICR_txic_END (5)
#define SOC_UART_UARTICR_rtic_START (6)
#define SOC_UART_UARTICR_rtic_END (6)
#define SOC_UART_UARTICR_feic_START (7)
#define SOC_UART_UARTICR_feic_END (7)
#define SOC_UART_UARTICR_peic_START (8)
#define SOC_UART_UARTICR_peic_END (8)
#define SOC_UART_UARTICR_beic_START (9)
#define SOC_UART_UARTICR_beic_END (9)
#define SOC_UART_UARTICR_oeic_START (10)
#define SOC_UART_UARTICR_oeic_END (10)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned short value;
    struct
    {
        unsigned short rxdmae : 1;
        unsigned short txdmae : 1;
        unsigned short dmaonerr : 1;
        unsigned short reserved : 13;
    } reg;
} SOC_UART_UARTDMACR_UNION;
#endif
#define SOC_UART_UARTDMACR_rxdmae_START (0)
#define SOC_UART_UARTDMACR_rxdmae_END (0)
#define SOC_UART_UARTDMACR_txdmae_START (1)
#define SOC_UART_UARTDMACR_txdmae_END (1)
#define SOC_UART_UARTDMACR_dmaonerr_START (2)
#define SOC_UART_UARTDMACR_dmaonerr_END (2)
#ifdef __cplusplus
    #if __cplusplus
        }
    #endif
#endif
#endif
