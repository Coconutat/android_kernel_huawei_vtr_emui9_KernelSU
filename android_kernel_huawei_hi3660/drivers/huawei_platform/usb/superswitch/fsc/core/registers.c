/* **************************************************************************
 * registers.c
 *
 * Implements the I2C register definitions/containers for the FUSB3601.
 * ************************************************************************** */

#include "port.h"
#include "registers.h"

/*
 * Returns a ptr to the cached value of the specified register in registers.
 * Note that this does not include reserved registers.
 */
FSC_U8 *FUSB3601_AddressToRegister(DeviceReg_t *registers, enum RegAddress address)
{
  FSC_U8 *reg = 0;

  switch (address) {
  case regVENDIDL:
    reg = &registers->VendIDL;
    break;
  case regVENDIDH:
    reg = &registers->VendIDH;
    break;
  case regPRODIDL:
    reg = &registers->ProdIDL;
    break;
  case regPRODIDH:
    reg = &registers->ProdIDH;
    break;
  case regDEVIDL:
    reg = &registers->DevIDL;
    break;
  case regDEVIDH:
    reg = &registers->DevIDH;
    break;
  case regTYPECREVL:
    reg = &registers->TypeCRevL;
    break;
  case regTYPECREVH:
    reg = &registers->TypeCRevH;
    break;
  case regUSBPDVER:
    reg = &registers->USBPDVer;
    break;
  case regUSBPDREV:
    reg = &registers->USBPDRev;
    break;
  case regPDIFREVL:
    reg = &registers->PDIFRevL;
    break;
  case regPDIFREVH:
    reg = &registers->PDIFRevH;
    break;
  case regALERTL:
    reg = &registers->AlertL.byte;
    break;
  case regALERTH:
    reg = &registers->AlertH.byte;
    break;
  case regALERTMSKL:
    reg = &registers->AlertMskL.byte;
    break;
  case regALERTMSKH:
    reg = &registers->AlertMskH.byte;
    break;
  case regPWRSTATMSK:
    reg = &registers->PwrStatMsk.byte;
    break;
  case regFAULTSTATMSK:
    reg = &registers->FaultStatMsk.byte;
    break;
  case regSTD_OUT_CFG:
    reg = &registers->StdOutCfg.byte;
    break;
  case regTCPC_CTRL:
    reg = &registers->TcpcCtrl.byte;
    break;
  case regROLECTRL:
    reg = &registers->RoleCtrl.byte;
    break;
  case regFAULTCTRL:
    reg = &registers->FaultCtrl.byte;
    break;
  case regPWRCTRL:
    reg = &registers->PwrCtrl.byte;
    break;
  case regCCSTAT:
    reg = &registers->CCStat.byte;
    break;
  case regPWRSTAT:
    reg = &registers->PwrStat.byte;
    break;
  case regFAULTSTAT:
    reg = &registers->FaultStat.byte;
    break;
  case regCOMMAND:
    reg = &registers->Command;
    break;
  case regDEVCAP1L:
    reg = &registers->DevCap1L.byte;
    break;
  case regDEVCAP1H:
    reg = &registers->DevCap1H.byte;
    break;
  case regDEVCAP2L:
    reg = &registers->DevCap2L.byte;
    break;
  case regSTD_OUT_CAP:
    reg = &registers->StdOutCap.byte;
    break;
  case regMSGHEADR:
    reg = &registers->MsgHeadr.byte;
    break;
  case regRXDETECT:
    reg = &registers->RxDetect.byte;
    break;
  case regRXBYTECNT:
    reg = &registers->RxByteCnt;
    break;
  case regRXSTAT:
    reg = &registers->RxStat.byte;
    break;
  case regRXHEADL:
    reg = &registers->RxHeadL;
    break;
  case regRXHEADH:
    reg = &registers->RxHeadH;
    break;
  case regTRANSMIT:
    reg = &registers->Transmit.byte;
    break;
  case regTXBYTECNT:
    reg = &registers->TxByteCnt;
    break;
  case regTXHEADL:
    reg = &registers->TxHeadL;
    break;
  case regTXHEADH:
    reg = &registers->TxHeadH;
    break;
  case regVBUS_VOLTAGE_L:
    reg = &registers->VBusVoltageL.byte;
    break;
  case regVBUS_VOLTAGE_H:
    reg = &registers->VBusVoltageH.byte;
    break;
  case regVBUS_SNK_DISCL:
    reg = &registers->VBusSnkDiscL.byte;
    break;
  case regVBUS_SNK_DISCH:
    reg = &registers->VBusSnkDiscH.byte;
    break;
  case regVBUS_STOP_DISCL:
    reg = &registers->VBusStopDiscL.byte;
    break;
  case regVBUS_STOP_DISCH:
    reg = &registers->VBusStopDiscH.byte;
    break;
  case regVALARMHCFGL:
    reg = &registers->VAlarmHCfgL.byte;
    break;
  case regVALARMHCFGH:
    reg = &registers->VAlarmHCfgH.byte;
    break;
  case regVALARMLCFGL:
    reg = &registers->VAlarmLCfgL.byte;
    break;
  case regVALARMLCFGH:
    reg = &registers->VAlarmLCfgH.byte;
    break;
  case regSCP_ENABLE1:
    reg = &registers->SCPEnable1.byte;
    break;
  case regSCP_ENABLE2:
    reg = &registers->SCPEnable2.byte;
    break;
  case regSCP_INT1_MSK:
    reg = &registers->SCPInt1Msk.byte;
    break;
  case regSCP_INT2_MSK:
    reg = &registers->SCPInt2Msk.byte;
    break;
  case regTIMER_SET1:
    reg = &registers->TimerSet1.byte;
    break;
  case regTIMER_SET2:
    reg = &registers->TimerSet2.byte;
    break;
  case regEVENT_1:
    reg = &registers->Event1.byte;
    break;
  case regEVENT_2:
    reg = &registers->Event2.byte;
    break;
  case regEVENT_3:
    reg = &registers->Event3.byte;
    break;
  case regDEFAULT_CMD:
    reg = &registers->DefaultCmd;
    break;
  case regDEFAULT_ADDR:
    reg = &registers->DefaultAddr;
    break;
  case regAUTO_CMD:
    reg = &registers->AutoCmd;
    break;
  case regAUTO_ADDR:
    reg = &registers->AutoAddr;
    break;
  case regAUTO_DATA0:
    reg = &registers->AutoData0;
    break;
  case regAUTO_DATA1:
    reg = &registers->AutoData1;
    break;
  case regAUTO_DATA2:
    reg = &registers->AutoData2;
    break;
  case regAUTO_DATA3:
    reg = &registers->AutoData3;
    break;
  case regAUTO_DATA4:
    reg = &registers->AutoData4;
    break;
  case regAUTO_BUFFER_ACK:
    reg = &registers->AutoBufferAck;
    break;
  case regAUTO_BUFFER_RX0:
    reg = &registers->AutoBufferRx0;
    break;
  case regAUTO_BUFFER_RX1:
    reg = &registers->AutoBufferRx1;
    break;
  case regAUTO_BUFFER_RX2:
    reg = &registers->AutoBufferRx2;
    break;
  case regAUTO_BUFFER_RX3:
    reg = &registers->AutoBufferRx3;
    break;
  case regRT_CMD:
    reg = &registers->RTCmd;
    break;
  case regRT_ADDR:
    reg = &registers->RTAddr;
    break;
  case regRT_ACK_RX:
    reg = &registers->RTAckRx;
    break;
  case regMUS_CONTROL_1:
    reg = &registers->MUSControl1.byte;
    break;
  case regMUS_CONTROL_2:
    reg = &registers->MUSControl2.byte;
    break;
  case regMUS_INTERRUPT:
    reg = &registers->MUSInterrupt.byte;
    break;
  case regMUS_INTERRUPT_MSK:
    reg = &registers->MUSIntMask.byte;
    break;
  case regMUS_TIMING:
    reg = &registers->MUSTiming.byte;
    break;
  case regDEVICE_TYPE:
    reg = &registers->DeviceType.byte;
    break;
  case regWD_RESET:
    reg = &registers->WDReset;
    break;
  case regWD_TIMING:
    reg = &registers->WDTiming;
    break;
  case regJIG_TIMING:
    reg = &registers->JigTiming.byte;
    break;
  case regWD_STATUS:
    reg = &registers->WDStatus.byte;
    break;
  case regWD_HISTORY_RESET:
    reg = &registers->WDHistoryReset.byte;
    break;
  case regFM_CONTROL1:
    reg = &registers->FMControl1.byte;
    break;
  case regFM_CONTROL2:
    reg = &registers->FMControl2.byte;
    break;
  case regFM_CONTROL3:
    reg = &registers->FMControl3.byte;
    break;
  case regFM_CONTROL4:
    reg = &registers->FMControl4.byte;
    break;
  case regFM_STATUS:
    reg = &registers->FMStatus.byte;
    break;
  case regVCONN_OCP:
    reg = &registers->VConnOCP.byte;
    break;
  case regPD_RESET:
    reg = &registers->PDReset.byte;
    break;
  case regDRP_TOGGLE:
    reg = &registers->DRPToggle.byte;
    break;
  case regSINK_TRANSMIT:
    reg = &registers->SinkTransmit.byte;
    break;
  case regSRC_FRSWAP:
    reg = &registers->SrcFRSwap.byte;
    break;
  case regSNK_FRSWAP:
    reg = &registers->SnkFRSwap.byte;
    break;
  case regDATA_OVP_CONTROL:
    reg = &registers->DataOVPControl.byte;
    break;
  case regDATA_OVP_INT:
    reg = &registers->DataOVPInt.byte;
    break;
  case regDATA_OVP_INT_MSK:
    reg = &registers->DataOVPMsk.byte;
    break;
  default:
    break;
  }
  return reg;
}

/* Populates data with contents of registers, excluding reserved registers. */
void FUSB3601_GetLocalRegisters(DeviceReg_t *registers, FSC_U8 *data, FSC_U32 length)
{
  /* TODO - Was called by hostcomm, but buffer size to small for current
   * register map size... not being used elsewhere.
   */
/*
  if (length >= TOTAL_REGISTER_CNT) {
    data[0] = registers->VendIDL;
    data[1] = registers->VendIDH;
    data[2] = registers->ProdIDL;
    data[3] = registers->ProdIDH;
    data[4] = registers->DevIDL;
    data[5] = registers->DevIDH;
    data[6] = registers->TypeCRevL;
    data[7] = registers->TypeCRevH;
    data[8] = registers->USBPDVer;
    data[9] = registers->USBPDRev;
    data[10] = registers->PDIFRevL;
    data[11] = registers->PDIFRevH;
    data[12] = registers->AlertL.byte;
    data[13] = registers->AlertH.byte;
    data[14] = registers->AlertMskL.byte;
    data[15] = registers->AlertMskH.byte;
    data[16] = registers->PwrStatMsk.byte;
    data[17] = registers->FaultStatMsk.byte;
    data[18] = registers->StdOutCfg.byte;
    data[19] = registers->TcpcCtrl.byte;
    data[20] = registers->RoleCtrl.byte;
    data[21] = registers->FaultCtrl.byte;
    data[22] = registers->PwrCtrl.byte;
    data[23] = registers->CCStat.byte;
    data[24] = registers->PwrStat.byte;
    data[25] = registers->FaultStat.byte;
    data[26] = registers->Command;
    data[27] = registers->DevCap1L.byte;
    data[28] = registers->DevCap1H.byte;
    data[29] = registers->DevCap2L.byte;
    data[31] = registers->StdOutCap.byte;
    data[32] = registers->MsgHeadr.byte;
    data[33] = registers->RxDetect.byte;
    data[34] = registers->RxByteCnt;
    data[35] = registers->RxStat.byte;
    data[36] = registers->RxHeadL;
    data[37] = registers->RxHeadH;
    data[38] = registers->RxData[0];
    data[39] = registers->RxData[1];
    data[40] = registers->RxData[2];
    data[41] = registers->RxData[3];
    data[42] = registers->RxData[4];
    data[43] = registers->RxData[5];
    data[44] = registers->RxData[6];
    data[45] = registers->RxData[7];
    data[46] = registers->RxData[8];
    data[47] = registers->RxData[9];
    data[48] = registers->RxData[10];
    data[49] = registers->RxData[11];
    data[50] = registers->RxData[12];
    data[51] = registers->RxData[13];
    data[52] = registers->RxData[14];
    data[53] = registers->RxData[15];
    data[54] = registers->RxData[16];
    data[55] = registers->RxData[17];
    data[56] = registers->RxData[18];
    data[57] = registers->RxData[19];
    data[58] = registers->RxData[20];
    data[59] = registers->RxData[21];
    data[60] = registers->RxData[22];
    data[61] = registers->RxData[23];
    data[62] = registers->RxData[24];
    data[63] = registers->RxData[25];
    data[64] = registers->RxData[26];
    data[65] = registers->RxData[27];
    data[66] = registers->Transmit.byte;
    data[67] = registers->TxByteCnt;
    data[68] = registers->TxHeadL;
    data[69] = registers->TxHeadH;
    data[70] = registers->TxData[0];
    data[71] = registers->TxData[1];
    data[72] = registers->TxData[2];
    data[73] = registers->TxData[3];
    data[74] = registers->TxData[4];
    data[75] = registers->TxData[5];
    data[76] = registers->TxData[6];
    data[77] = registers->TxData[7];
    data[78] = registers->TxData[8];
    data[79] = registers->TxData[9];
    data[80] = registers->TxData[10];
    data[81] = registers->TxData[11];
    data[82] = registers->TxData[12];
    data[83] = registers->TxData[13];
    data[84] = registers->TxData[14];
    data[85] = registers->TxData[15];
    data[86] = registers->TxData[16];
    data[87] = registers->TxData[17];
    data[88] = registers->TxData[18];
    data[89] = registers->TxData[19];
    data[90] = registers->TxData[20];
    data[91] = registers->TxData[21];
    data[92] = registers->TxData[22];
    data[93] = registers->TxData[23];
    data[94] = registers->TxData[24];
    data[95] = registers->TxData[25];
    data[96] = registers->TxData[26];
    data[97] = registers->TxData[27];
    data[98] = registers->VBusVoltageL.byte;
    data[99] = registers->VBusVoltageH.byte;
    data[100] = registers->VBusSnkDiscL.byte;
    data[101] = registers->VBusSnkDiscH.byte;
    data[102] = registers->VBusStopDiscL.byte;
    data[103] = registers->VBusStopDiscH.byte;
    data[104] = registers->VAlarmHCfgL.byte;
    data[105] = registers->VAlarmHCfgH.byte;
    data[106] = registers->VAlarmLCfgL.byte;
    data[107] = registers->VAlarmLCfgH.byte;
    data[108] = registers->SCPEnable1.byte;
    data[109] = registers->SCPEnable2.byte;
    data[110] = registers->SCPInt1Msk.byte;
    data[111] = registers->SCPInt2Msk.byte;
    data[112] = registers->TimerSet1.byte;
    data[113] = registers->TimerSet2.byte;
    data[114] = registers->Event1.byte;
    data[115] = registers->Event2.byte;
    data[116] = registers->Event3.byte;
    data[117] = registers->DefaultCmd;
    data[118] = registers->DefaultAddr;
    data[119] = registers->AutoCmd;
    data[120] = registers->AutoAddr;
    data[121] = registers->AutoData0;
    data[122] = registers->AutoData1;
    data[123] = registers->AutoData2;
    data[124] = registers->AutoData3;
    data[125] = registers->AutoData4;
    data[126] = registers->AutoBufferAck;
    data[127] = registers->AutoBufferRx0;
    data[128] = registers->AutoBufferRx1;
    data[129] = registers->AutoBufferRx2;
    data[130] = registers->AutoBufferRx3;
    data[131] = registers->RTCmd;
    data[132] = registers->RTAddr;
    data[133] = registers->RTBufferTx[0];
    data[134] = registers->RTBufferTx[1];
    data[135] = registers->RTBufferTx[2];
    data[136] = registers->RTBufferTx[3];
    data[137] = registers->RTBufferTx[4];
    data[138] = registers->RTBufferTx[5];
    data[139] = registers->RTBufferTx[6];
    data[140] = registers->RTBufferTx[7];
    data[141] = registers->RTBufferTx[8];
    data[142] = registers->RTBufferTx[9];
    data[143] = registers->RTBufferTx[10];
    data[144] = registers->RTBufferTx[11];
    data[146] = registers->RTBufferTx[12];
    data[147] = registers->RTBufferTx[13];
    data[148] = registers->RTBufferTx[14];
    data[149] = registers->RTBufferTx[15];
    data[150] = registers->RTBufferTx[16];
    data[151] = registers->RTAckRx;
    data[152] = registers->RTBufferRx[0];
    data[153] = registers->RTBufferRx[1];
    data[154] = registers->RTBufferRx[2];
    data[155] = registers->RTBufferRx[3];
    data[156] = registers->RTBufferRx[4];
    data[157] = registers->RTBufferRx[5];
    data[158] = registers->RTBufferRx[6];
    data[159] = registers->RTBufferRx[7];
    data[160] = registers->RTBufferRx[8];
    data[161] = registers->RTBufferRx[9];
    data[162] = registers->RTBufferRx[10];
    data[163] = registers->RTBufferRx[11];
    data[164] = registers->RTBufferRx[12];
    data[165] = registers->RTBufferRx[13];
    data[166] = registers->RTBufferRx[14];
    data[167] = registers->RTBufferRx[15];
    data[168] = registers->MUSControl1.byte;
    data[169] = registers->MUSControl2.byte;
    data[170] = registers->MUSInterrupt.byte;
    data[171] = registers->MUSIntMask.byte;
    data[172] = registers->MUSTiming.byte;
    data[173] = registers->WDReset;
    data[174] = registers->WDTiming;
    data[175] = registers->JigTiming.byte;
    data[176] = registers->WDStatus.byte;
    data[177] = registers->WDHistoryReset.byte;
    data[178] = registers->FMControl1.byte;
    data[179] = registers->FMControl2.byte;
    data[180] = registers->FMControl3.byte;
    data[181] = registers->FMControl4.byte;
    data[182] = registers->FMStatus.byte;
    data[183] = registers->VConnOCP.byte;
    data[184] = registers->PDReset.byte;
    data[185] = registers->DRPToggle.byte;
    data[186] = registers->SinkTransmit.byte;
    data[187] = registers->SrcFRSwap.byte;
    data[188] = registers->SnkFRSwap.byte;
    data[189] = registers->DataOVPControl.byte;
    data[190] = registers->DataOVPInt.byte;
    data[191] = registers->DataOVPMsk.byte;
  }
*/
}

void FUSB3601_RegGetRxData(DeviceReg_t *registers, FSC_U8 *data, FSC_U32 length)
{
  FSC_U8 i = 0;
  for (i = 0; i < length && i < COMM_BUFFER_LENGTH; ++i) {
    data[i] = registers->RxData[i];
  }
}

void FUSB3601_RegSetTxData(DeviceReg_t *registers, FSC_U8 *data, FSC_U32 length)
{
  FSC_U8 i = 0;
  for (i = 0; i < length && i < COMM_BUFFER_LENGTH; ++i) {
    registers->TxData[i] = data[i];
  }
}

void FUSB3601_RegSetBits(DeviceReg_t *registers, enum RegAddress address, FSC_U8 mask)
{
  FSC_U8 *reg = FUSB3601_AddressToRegister(registers, address);
  *reg |= mask;
}

void FUSB3601_RegClearBits(DeviceReg_t *registers, enum RegAddress address, FSC_U8 mask)
{
  FSC_U8 *reg = FUSB3601_AddressToRegister(registers, address);
  *reg &= ~mask;
}













