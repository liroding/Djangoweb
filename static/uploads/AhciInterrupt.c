//**********************************************************************
//**********************************************************************
//**                                                                  **
//**     Copyright (c) 2018 Shanghai Zhaoxin Semiconductor Co., Ltd.  **
//**                                                                  **
//**                Author: Mike Yuan, MikeYuan@zhaoxin.com
//**********************************************************************
//**********************************************************************


#include "AhciInterrupt.h"
//#include <>
#define PCI_DEV_MMBASE(Bus, Device, Function) \
    ( \
      (UINTN)PcdGet64(PcdPciExpressBaseAddress) + (UINTN) (Bus << 20) + (UINTN) (Device << 15) + (UINTN) \
        (Function << 12) \
    )
/*#define ATA_Device_DATA_FROM_THIS(a) \
	  CR (a, \
		  ATA_ATAPI_PASS_THRU_INSTANCE, \
		  AtaPassThru, \
		  ATA_ATAPI_PASS_THRU_SIGNATURE \
		  )
*/
#define ATA_DEVICE_SIGNATURE  SIGNATURE_32 ('A', 'B', 'I', 'D')

#define ATA_DEVICE_FROM_BLOCK_IO(a)         CR (a, ATA_DEVICE, BlockIo, ATA_DEVICE_SIGNATURE)

#define LPC_PCI_REG(Reg)          (PCI_DEV_MMBASE(0, 17, 0)+Reg)
#define APIC_PCI_REG(Reg)         (PCI_DEV_MMBASE(0,  0, 5)+Reg)
#define D2F0_REG(Reg)             (PCI_DEV_MMBASE(0,  2, 0)+Reg)
#define SLAVE_LPC_PCI_REG(Reg)    (PCI_DEV_MMBASE(SlaveSocketBus, 17, 0)+Reg)
#define SLAVE_APIC_PCI_REG(Reg)   (PCI_DEV_MMBASE(SlaveSocketBus,  0, 5)+Reg)
#define SLAVE_D2F0_REG(Reg)       (PCI_DEV_MMBASE(SlaveSocketBus,  2, 0)+Reg)

#define IS_CND001_Board 1
#define IS_CHX001_Board 2
#define IS_CHX002_Board 3
#define IS_CHX02_Single_Board 4
#define AHCI_GHC  0x04
#define AHCI_IS   0x08
#define AHCI_PORT_START                    0x0100
#define AHCI_PORT_REG_WIDTH                0x0080
#define AHCI_PORT_IE                       0x14
#define AHCI_PORT_IS                       0x10
#define DPE_INT                            (1<<5)
#define DHRE_INT                           (1)
#define EFI_AHCI_BAR_INDEX                  5
#define SlaveSocketBus                     0x80
EFI_PCI_IO_PROTOCOL *gPciIo=NULL;
volatile BOOLEAN HasEnterInterrupt = FALSE;
volatile BOOLEAN CoreStop = FALSE;
volatile UINT8 gIntProcessors[16];
UINT32	 InProcessCPUNum = 0;
UINT32   OutProcessCPUNum = 0;
volatile UINT32   IntNeedProcessors = 0;
volatile UINTN    gNumberOfProcessors = 0;
FILE *						Fp;
UINT32   gDebugMessage = 0;
UINT32   gFSBC = 0;
UINT32   gFSBC_Begin=0;
IA32_DESCRIPTOR				IdtrForBSP;
EFI_CPU_ARCH_PROTOCOL	  *mCpu;
UINTN gIRqNum;
UINT32  gDestMode;
UINT8   ProcessorLocalApicID[16];
UINT8   gBoardType;
UINT8   gSlaveSocket;
UINT8   Temp;
UINT8   gBSPProcessor = 0;
UINT8   gBSPProcessorHere = 0;
UINTN
Debug_InternalPrint (
  IN  CONST CHAR16                     *Format,
  IN  EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL  *Console,
  IN  VA_LIST                          Marker
  )
{
  EFI_STATUS  Status;
  UINTN       Return;
  CHAR16      *Buffer;
  UINTN       BufferSize;

  ASSERT (Format != NULL);
  ASSERT (((UINTN) Format & BIT0) == 0);
  ASSERT (Console != NULL);

  BufferSize = (320+ 1) * sizeof (CHAR16);

  Buffer = (CHAR16 *) AllocatePool(BufferSize);
  ASSERT (Buffer != NULL);

  Return = UnicodeVSPrint (Buffer, BufferSize, Format, Marker);

  if (Console != NULL && Return > 0) {
    //
    // To be extra safe make sure Console has been initialized
    //
    Status = Console->OutputString (Console, Buffer);
    if (EFI_ERROR (Status)) {
      Return = 0;
    }
  }

  FreePool (Buffer);

  return Return;
}

UINTN 
EFIAPI 
Debug_Print ( 
  IN CONST CHAR16  *Format, 
  ... 
  ) 
{ 
  VA_LIST Marker; 
  UINTN	 Return; 
  if(gDebugMessage==0){ 
	return EFI_SUCCESS; 
  }
  else if(gDebugMessage==1){
   VA_START (Marker, Format); 
   Return = Debug_InternalPrint (Format, gST->ConOut, Marker); 
   VA_END (Marker); 
   return Return; 
  }
  return EFI_SUCCESS;
}
UINT32
Mike_InternalAcpiGetTimerTick (
  VOID
  )
{
  return IoRead32(0x800 + 8);
}

VOID
Mike_InternalAcpiDelay (
  IN      UINT32                    Delay
  )
{
  UINT32                            Ticks;
  UINT32                            Times;

  Times    = Delay >> 22;
  Delay   &= BIT22 - 1;
  do {
    //
    // The target timer count is calculated here
    //
    Ticks    = Mike_InternalAcpiGetTimerTick () + Delay;
    Delay    = BIT22;
    //
    // Wait until time out
    // Delay >= 2^23 could not be handled by this function
    // Timer wrap-arounds are handled correctly by this function
    //
    while (((Ticks - Mike_InternalAcpiGetTimerTick ()) & BIT23) == 0) {
      CpuPause ();
    }
  } while (Times-- > 0);
}

UINTN
EFIAPI
Mike_MicroSecondDelay (
  IN      UINTN                     MicroSeconds
  )
{
  Mike_InternalAcpiDelay (
    (UINT32)DivU64x32 (
              MultU64x32 (
                MicroSeconds,
                3579545
                ),
              1000000u
              )
    );
  return MicroSeconds;
}

typedef unsigned long long  u64;
typedef unsigned long  DWORD, u32;
typedef unsigned short WORD, u16;
typedef unsigned char BYTE, u8;
typedef BYTE*    PBYTE;
typedef DWORD*   PDWORD;

#define SIZE 20
typedef struct
{
	char opcode[SIZE];
	char addr[SIZE];
	char mask[SIZE];
	char val[SIZE];
}CMD;
#define  STR_EQUAL       0
#define  STR_COMMENT  "//"
#define  MEM  0
#define  MSR  1
#define  LAPIC 2
typedef struct
{
	u32   id;
	const char* str1;
	const char* str2;
	const char* str3;
}OP_INFO;

OP_INFO op[] = {
{		MEM,		"MEM",			"Mem",			"mem" },
{		MSR,		"MSR",			"Msr",			"msr" },
{       LAPIC,      "LAPIC",        "Lapic",         "lApic"},
{		0,			"",        		"",				""  }
};

u32 SKIP_COMMENT(FILE* fp)
{
	u32 out = 0;
	BYTE cc;
	for ( ;; )
	{
		cc=(BYTE)fgetc(fp);
		if (cc == (BYTE)EOF) {
			out = 1;
			break;
		} else if (cc == '\n') {
			break;
		}
	}
	return out;
}
typedef struct
{
	u64 addr;
	u64 mask;
	u64 val;
}HEX_CMD_INFO_64;

typedef struct
{
	u32 addr;
	u32 mask;
	u32 val;
}HEX_CMD_INFO_32;
#define  C_0   '0'
#define  C_9   '9'
#define  C_a   'a'
#define  C_f   'f'
#define  C_A   'A'
#define  C_F   'F'

#define PRINT64(pstr, addr, mask, val) {\
Debug_Print(pstr);\
Debug_Print(L"Address: %llX, ", addr);\
Debug_Print(L"Mask: %llX, ", mask);\
Debug_Print(L"Value: %llX\n", val);\
}

u64 str_2_hex(char *pst)
{
    int i;
    u64 val=0;
    for ( i=0; pst[i] != '\0'; i++)
    {
        val *= 0x10;
        if (pst[i] >= C_0 && pst[i] <= C_9)
            val |= pst[i] - '0';
        if (pst[i] >= C_a && pst[i] <= C_f)
            val |= pst[i] - 'a' + 0xA;
        if (pst[i] >= C_A && pst[i] <= C_F)
            val |= pst[i] - 'A' + 0xA;
    }
    return val;
}
CHAR16 *
Ascii2Unicode (
  OUT CHAR16         *UnicodeStr,
  IN  CHAR8          *AsciiStr
  )
/*++

Routine Description:
  Converts ASCII characters to Unicode.

Arguments:
  UnicodeStr - the Unicode string to be written to. The buffer must be large enough.
  AsciiStr   - The ASCII string to be converted.

Returns:
  The address to the Unicode string - same as UnicodeStr.

--*/
{
  CHAR16  *Str;

  Str = UnicodeStr;

  while (TRUE) {

    *(UnicodeStr++) = (CHAR16) *AsciiStr;

    if (*(AsciiStr++) == '\0') {
      return Str;
    }
  }
}

u64 rdmsr(DWORD addr)
{
	return AsmReadMsr64(addr);
}

void wrmsr(DWORD addr, u64 qvalue )
{
	AsmWriteMsr64(addr, qvalue);
}

void modify_msr(DWORD addr, u64 mask, u64 qmodify_val )
{
	u64 qval= 0;
	qval = rdmsr(addr);
	//printf(" Before Msr: [%X]=%llX\n",addr,qval);
	qval &= (u64)(~mask);
	qval |= qmodify_val & mask;
	//printf(" After  Msr: [%X]=%llX\n",addr,qval);
	wrmsr(addr, qval);
}
HEX_CMD_INFO_64	MSRSet[16][20];
HEX_CMD_INFO_64 LApic[16][20];
UINT64          TPRMSR[16][20];
UINT32 iMsr[16];
UINT32 iLApic[16];
VOID execute_bat ( IN  VOID  *Buffer)
{
    UINT8 cpuindex= *(UINT8*)Buffer;
	UINT32 Index=0;
    /*UINT32	Eax, Ebx;
	UINT32	cpuid;
	UINT32 Index=0;
	AsmCpuid(1, &Eax, &Ebx, NULL, NULL);
	cpuid = (Ebx >> 24) & 0xFF;*/
	for(Index=0;Index<iMsr[cpuindex];Index++){	
		modify_msr((DWORD)MSRSet[cpuindex][Index].addr, MSRSet[cpuindex][Index].mask, MSRSet[cpuindex][Index].val );
	}
}

VOID Interrupt_execute_bat(char* pstr)
{
	CMD				cmd = {0};
	FILE *			fp;
	HEX_CMD_INFO_32	info32 = {0};
	HEX_CMD_INFO_64	info64 = {0};
	u64				val = 0;
	u32				out;
	u32				fd = 0, idx;
	char            Processor[SIZE];
	CHAR16         UnicodeStr[100];
	UINT32          Index;
	UINT32          FistIndex=0;
    fp = fopen(pstr, "r");
	if (!fp){
		Print(L"Open %s Failed\n",pstr);
        return;
	}
	else{
		Ascii2Unicode(UnicodeStr,pstr);
		Debug_Print(L"Open %s Successed\n",UnicodeStr);
	}
	for (out = 0, fd = 0 ;fscanf(fp, "%s", cmd.opcode) != EOF && out == 0; fd = 0)
	{
		if ( strncmp(cmd.opcode, STR_COMMENT, 2) == 0)
		{
			out = SKIP_COMMENT(fp);
			continue;
		}

		for (idx = 0; idx < sizeof(op)/sizeof(OP_INFO) && !fd ; idx++) //check op
		{
			if ( strcmp(cmd.opcode, op[idx].str1) == STR_EQUAL ||
				strcmp(cmd.opcode, op[idx].str2) == STR_EQUAL ||
				strcmp(cmd.opcode, op[idx].str3) == STR_EQUAL)
			{
				if (strlen(op[idx].str1) == strlen(cmd.opcode))
				{
					fd = 1;
					break;
				}
			}
		}

		if (fd) {
			switch (idx)
			{

				//MEM opcode
				case MEM:
				   if ( fscanf(fp, "%s", cmd.addr) != EOF &&
			       fscanf(fp, "%s", cmd.mask) != EOF &&
			       fscanf(fp, "%s", cmd.val ) != EOF )
		           {
			         out = SKIP_COMMENT(fp);
		           }
					PRINT64(L"Modify Memory : ",
					info64.addr = str_2_hex(cmd.addr),
					info64.mask = str_2_hex(cmd.mask),
					info64.val  = str_2_hex(cmd.val)
					);
					val = *(u64*)(info64.addr);
					if (info64.mask != 0) {
						*(u64*)(info64.addr) = val & ~info64.mask | info64.val & info64.mask;
					}

					break;

				//MSR opcode
				case MSR:
					if (fscanf(fp, "%s", Processor)!= EOF &&
						fscanf(fp, "%s", cmd.addr) != EOF &&
			            fscanf(fp, "%s", cmd.mask) != EOF &&
			            fscanf(fp, "%s", cmd.val ) != EOF )
		            {
			               out = SKIP_COMMENT(fp);
		            }
					Debug_Print(L"Core ");
					for(Index=0;Index<16;Index++){
						if(((str_2_hex(Processor)>>Index&0x1))==1){
							FistIndex = Index;
							Debug_Print(L"%x ,",Index) ;
							MSRSet[Index][iMsr[Index]].addr= str_2_hex(cmd.addr);
					        MSRSet[Index][iMsr[Index]].mask = str_2_hex(cmd.mask);
					        MSRSet[Index][iMsr[Index]].val = str_2_hex(cmd.val);
							iMsr[Index]++;
						}
					}
					PRINT64(L"Modify MSR    : ",
					MSRSet[FistIndex][iMsr[FistIndex]].addr= str_2_hex(cmd.addr),
					MSRSet[FistIndex][iMsr[FistIndex]].mask = str_2_hex(cmd.mask),
					MSRSet[FistIndex][iMsr[FistIndex]].val = str_2_hex(cmd.val)
					);
					break;
				case LAPIC:
					if (fscanf(fp, "%s", Processor)!= EOF &&
						fscanf(fp, "%s", cmd.addr) != EOF &&
			            fscanf(fp, "%s", cmd.mask) != EOF &&
			            fscanf(fp, "%s", cmd.val ) != EOF )
		            {
			               out = SKIP_COMMENT(fp);
		            }
			        Debug_Print(L"Core ");
					for(Index=0;Index<16;Index++){
						if(((str_2_hex(Processor)>>Index&0x1))==1){
							FistIndex = Index;
							Debug_Print(L"%x ,",Index) ;
							LApic[Index][iLApic[Index]].addr= str_2_hex(cmd.addr);
					        LApic[Index][iLApic[Index]].mask = str_2_hex(cmd.mask);
					        LApic[Index][iLApic[Index]].val = str_2_hex(cmd.val);
							iLApic[Index]++;
						}
					}
					PRINT64(L"Modify LAPIC    : ",
					LApic[FistIndex][iLApic[FistIndex]].addr= str_2_hex(cmd.addr),
					LApic[FistIndex][iLApic[FistIndex]].mask = str_2_hex(cmd.mask),
					LApic[FistIndex][iLApic[FistIndex]].val = str_2_hex(cmd.val)
					);
					break;
					
			}
			memset(&info32, 0, sizeof(info32));
			memset(&info64, 0, sizeof(info64));
		}

	}

	fclose(fp);

	return ;
}


UINT32
EFIAPI
AhciReadReg (
  IN EFI_PCI_IO_PROTOCOL  *PciIo,
  IN  UINT32              Offset
  )
{
  UINT32                  Data;

  ASSERT (PciIo != NULL);
  
  Data = 0;

  PciIo->Mem.Read (
               PciIo,
               EfiPciIoWidthUint32,
               EFI_AHCI_BAR_INDEX,
               (UINT64) Offset,
               1,
               &Data
               );

  return Data;
}

/**
  Write AHCI Operation register.

  @param  PciIo        The PCI IO protocol instance.
  @param  Offset       The operation register offset.
  @param  Data         The data used to write down.

**/
VOID
EFIAPI
AhciWriteReg (
  IN EFI_PCI_IO_PROTOCOL  *PciIo,
  IN UINT32               Offset,
  IN UINT32               Data
  )
{
  ASSERT (PciIo != NULL);

  PciIo->Mem.Write (
               PciIo,
               EfiPciIoWidthUint32,
               EFI_AHCI_BAR_INDEX,
               (UINT64) Offset,
               1,
               &Data
               );

  return ;
}
#define FSBC_SIZE_256MB 0x0
#define FSBC_SIZE_512MB 0x1
#define FSBC_C2P        BIT0
#define FSBC_P2C        BIT1
#define FSBC_C2M        BIT2
#define FSBC_Asyc       BIT4
#define FSBC_Debug      BIT5
#define FSBC_All        BIT7

VOID FSBC_MSR_Dump()
{
  /*UINT8 i = 0;
  UINT64 MsrValue;
  DEBUG((EFI_D_ERROR,"Begin to Dump FSBC MSR\n"));
  DEBUG((EFI_D_ERROR,"======================\n"));
  for(i=0;i<9;i++){
        MsrValue = AsmReadMsr64(0x1604+i);
        DEBUG((EFI_D_ERROR,"FSBC_MSR[%x]:%llx\n",0x1604+i,MsrValue));
  }     
  DEBUG((EFI_D_ERROR,"======================\n"));*/
}

VOID FSBC_Config(UINT8 Size,UINT8 Cycle_Type)
{
  /*EFI_PHYSICAL_ADDRESS     Address;
  UINT32  Length = 0;
  EFI_STATUS               Status;
  UINT64                   MsrValue;
  Address = 0x40000000;
  switch (Size)
  {
    case FSBC_SIZE_256MB:
                Length = 0x10000000;
                break;
        case FSBC_SIZE_512MB:
                Length = 0x20000000;
                break;
        default:
                Length = 0x10000000;
                break;
  }
  Status =gBS->AllocatePages(
                        AllocateAddress,
                    EfiReservedMemoryType,
                    EFI_SIZE_TO_PAGES (Length),
                    &Address);
  if(Status!=0){
        Print(L"Mike_AllocateFailed:%d\n",Status);
        return ;
  }
  Status = mCpu->SetMemoryAttributes (
                              mCpu,
                              Address,
                              Length, 
                              EFI_MEMORY_UC
                              );
  
  if(Status!=0){
        Print(L"Mike_SetMemoryAttribtuesFaild:%d\n",Status);
  }
   DEBUG((EFI_D_ERROR,"Begin to Config FSBC MSR,dump to dram\n"));
   MsrValue = AsmReadMsr64(0x160B);
   DEBUG((EFI_D_ERROR,"FSBC_MSR[%x]:%llx\n",0x160B,MsrValue));
   MsrValue = MsrValue|Address|((UINT64)((UINT64)Size<<40));
   AsmWriteMsr64(0x160B, MsrValue);
   DEBUG((EFI_D_ERROR,"0x160B with address write done!\n"));
   MsrValue = AsmReadMsr64(0x160B);
   DEBUG((EFI_D_ERROR,"FSBC_MSR[%x]:%llx\n",0x160B,MsrValue));
   MsrValue = AsmReadMsr64(0x1609);
   MsrValue |=(((UINT64)Cycle_Type)<<8);
   AsmWriteMsr64(0x1609, MsrValue);
   //FSBC_MSR_Dump();
   return;*/
}
VOID FSBC_START_Dump()
{
  /* UINT64 MsrValue;
   DEBUG((EFI_D_ERROR,"0x1609 write once all!\n"));
   MsrValue = AsmReadMsr64(0x1609);
   DEBUG((EFI_D_ERROR,"FSBC_MSR[%x]:%llx\n",0x1609,MsrValue));
   MsrValue |= ((UINT64)0x07);
   DEBUG((EFI_D_ERROR,"FSBC_MSR[160B]:%llx\n",MsrValue));
   AsmWriteMsr64(0x1609, MsrValue);*/
}
VOID FSBC_END_Dump()
{
   /*UINT64 MsrValue;
   MsrValue = AsmReadMsr64(0x1609);
   MsrValue &= ((UINT64)~0x07);
   AsmWriteMsr64(0x1609, MsrValue);*/
}
VOID
Ahci_APIC_Test(EFI_BLOCK_IO_PROTOCOL 	*BlockIo,UINT8*  pRBuffer)
{
	EFI_STATUS   Status;
	UINTN    BufSize = 512;
	if(BlockIo==NULL){
		Print(L"BlockIo is NULL\n");
		return;
	}
	/*if(gFSBC==1){
	 if(gFSBC_Begin==1){
		FSBC_START_Dump();
		IoWrite8(0x80,0x88);
	 }
	}*/
	Status = BlockIo->ReadBlocks(
		                        BlockIo,
		                        0,
		                        0,
		                        BufSize,
		                        pRBuffer
		                        );
	if(Status!=0){
		Print(L"ReadBlocks_error:%x\n",Status);
	}
}
UINT32 mPort = 0;
VOID Clear_Interrupt()
{
//Clear PxIs bit0(Device to Host Register FIS Interrupt), bit5(Descriptor Processed)
  UINT32 Value32 = 0;
  Value32 = AhciReadReg (gPciIo,AHCI_PORT_START+mPort*AHCI_PORT_REG_WIDTH+AHCI_PORT_IS);
  AhciWriteReg(gPciIo,AHCI_PORT_START+mPort*AHCI_PORT_REG_WIDTH+AHCI_PORT_IS,Value32);
  Value32 = AhciReadReg(gPciIo,AHCI_IS);
  AhciWriteReg(gPciIo,AHCI_IS,Value32);//Clear IS all bits
}
VOID
EFIAPI
AHCI_ReadInterruptHandler (
  IN EFI_EXCEPTION_TYPE   InterruptType,
  IN EFI_SYSTEM_CONTEXT   SystemContext
  )
{
  UINT32	Eax, Ebx;
  UINT32	cpuid;
  UINT64		i = 500;
  UINT32    Index = 0;
  AsmCpuid(1, &Eax, &Ebx, NULL, NULL);
  cpuid = (Ebx >> 24) & 0xFF;
  gIntProcessors[cpuid]=1;
  IoWrite8(0x80,0xBB);
  InterlockedIncrement(&InProcessCPUNum);
  while(InProcessCPUNum != IntNeedProcessors)
  {
    i--;
	if(i == 0) break; 
	CpuPause();
  }
  IoWrite8(0x80,0xCC);
  for(Index=0;Index<16;Index++){
  	if(gIntProcessors[Index]==1){
		break;
  	}
  }
  if((cpuid==Index)&&(HasEnterInterrupt==FALSE))
  {
  HasEnterInterrupt = TRUE;
  Clear_Interrupt();
  // Local APIC EOI
  SendApicEoi ();
  IoWrite8(0x80,0xDD);
  Mike_MicroSecondDelay(500);  
  IoWrite8(0x80,0xEE);
  }


}
EFI_LEGACY_8259_PROTOCOL  *gLegacy8259;
#define NB_IO_APIC_BASE  0xFECC0000
#define SB_IO_APIC_BASE  0xFEC00000
#define MASTER_SB_IO_APIC_BASE  0xFEC00000
#define MASTER_NB_IO_APIC_BASE  0xFEC01000
#define SLAVE_SB_IO_APIC_BASE  0xFEC08000
#define SLAVE_NB_IO_APIC_BASE  0xFEC09000
#define NB_IO_APIC_ID    10
#define SB_IO_APIC_ID     9
#define MASTER_SB_IO_APIC_ID 0x10
#define MASTER_NB_IO_APIC_ID 0x11
#define SLAVE_SB_IO_APIC_ID 0x12
#define SLAVE_NB_IO_APIC_ID 0x13
UINTN  IO_Apic_Base;
UINT32
EFIAPI
Mike_IoApicRead (
  IN UINTN  Index
  )
{
  ASSERT (Index < 0x100);
  MmioWrite8 (IO_Apic_Base + IOAPIC_INDEX_OFFSET, (UINT8)Index);
  return MmioRead32 (IO_Apic_Base + IOAPIC_DATA_OFFSET);
}
UINT32
EFIAPI
Mike_IoApicWrite (
  IN UINTN   Index,
  IN UINT32  Value
  )
{
  ASSERT (Index < 0x100);
  MmioWrite8 (IO_Apic_Base + IOAPIC_INDEX_OFFSET, (UINT8)Index);
  return MmioWrite32 (IO_Apic_Base + IOAPIC_DATA_OFFSET, Value);
}
VOID
EFIAPI
Mike_IoApicEnableInterrupt (
  IN UINTN    Irq,
  IN BOOLEAN  Enable
  )
{
  IO_APIC_VERSION_REGISTER         Version;
  IO_APIC_REDIRECTION_TABLE_ENTRY  Entry;

  Version.Uint32 = Mike_IoApicRead (IO_APIC_VERSION_REGISTER_INDEX);
  ASSERT (Version.Bits.MaximumRedirectionEntry < 0xF0);
  ASSERT (Irq <= Version.Bits.MaximumRedirectionEntry);

  Entry.Uint32.Low = Mike_IoApicRead (IO_APIC_REDIRECTION_TABLE_ENTRY_INDEX + Irq * 2);
  Entry.Bits.Mask = Enable ? 0 : 1;
  Mike_IoApicWrite (IO_APIC_REDIRECTION_TABLE_ENTRY_INDEX + Irq * 2, Entry.Uint32.Low);
}
VOID
EFIAPI
Mike_IoApicConfigureInterrupt (
  IN UINTN    Irq,
  IN UINTN    Vector,
  IN UINTN    DeliveryMode,
  IN UINTN    DestinationMode,
  IN BOOLEAN  LevelTriggered,
  IN BOOLEAN  AssertionLevel,
  IN UINTN    Core
  )
{
  IO_APIC_VERSION_REGISTER         Version;
  IO_APIC_REDIRECTION_TABLE_ENTRY  Entry;

  Version.Uint32 = Mike_IoApicRead (IO_APIC_VERSION_REGISTER_INDEX);//Zoey: Read Index=01,which contain APIC Version and MaximumRedirectionEntry
  ASSERT (Version.Bits.MaximumRedirectionEntry < 0xF0);
  ASSERT (Irq <= Version.Bits.MaximumRedirectionEntry);
  ASSERT (Vector <= 0xFF);
  ASSERT (DeliveryMode < 8 && DeliveryMode != 6 && DeliveryMode != 3);

  //Zoey:Set RT entry low 32 bits
  Entry.Uint32.Low = Mike_IoApicRead (IO_APIC_REDIRECTION_TABLE_ENTRY_INDEX + Irq * 2);
  Entry.Bits.Vector          = (UINT8)Vector;
  Entry.Bits.DeliveryMode    = (UINT32)DeliveryMode;
  Entry.Bits.DestinationMode = (UINT32)DestinationMode; 
  Entry.Bits.Polarity        = AssertionLevel ? 0 : 1;
  Entry.Bits.TriggerMode     = LevelTriggered ? 1 : 0;
  Entry.Bits.Mask            = 1;
  Mike_IoApicWrite (IO_APIC_REDIRECTION_TABLE_ENTRY_INDEX + Irq * 2, Entry.Uint32.Low);

  //Zoey:Set RT entry high 32 bits
  Entry.Uint32.High = Mike_IoApicRead (IO_APIC_REDIRECTION_TABLE_ENTRY_INDEX + Irq * 2 + 1);
  //Entry.Bits.DestinationID = GetApicId ();
  Entry.Bits.DestinationID = (UINT32)Core;
  Entry.Uint32.High&=~(0xFF0000);
  Mike_IoApicWrite (IO_APIC_REDIRECTION_TABLE_ENTRY_INDEX + Irq * 2 + 1, Entry.Uint32.High);
}
UINT16 Find_Device_PCIEPort(EFI_DEVICE_PATH_PROTOCOL  *DevicePath,UINT8 INTX)
{
	UINT8 Value8;
	UINTN Device= 0;
	UINTN Func = 0;
	UINTN PortFunc = 0;
	UINTN PortDev = 0;
	//UINT32 ClassCode;
	//UINT8  SecondNum;
	UINT16 IrqNum = 0;
	EFI_STATUS		Status;
	UINT8			Rintrtapic;
	//EFI_STATUS   Status;
	//EFI_ACPI_SUPPORT_PROTOCOL 					*AcpiSupport=NULL;
	//EFI_ACPI_COMMON_HEADER  *Table=NULL;
	UINT32 BridgeCount = 0;
	UINT8  TempINTX;
	//CHAR8* Sign;	 
	//UINTN 				 Handle;
	UINTN	  i = 0;
	UINT8 DeviceList[0x100];
	while(!IsDevicePathEnd(DevicePath)){
		DevicePath = NextDevicePathNode(DevicePath);
		if((DevicePath->Type==0x1)&&(DevicePath->Type==0x1)){
			PortDev = *((UINT8*)((UINT8*)DevicePath+0x5))&0xff;
			PortFunc = *((UINT8*)((UINT8*)DevicePath+0x4))&0xff;
			if(BridgeCount==0){
				Device = PortDev;
				Func = PortFunc;
			}
			DeviceList[BridgeCount] = (UINT8)PortDev;
			BridgeCount++;
		}
			
	}
/*	for(Device=0;Device<0xff;Device++){
		for(Func=0;Func<0x8;Func++){
		  ClassCode = MmioRead32(PCI_DEV_MMBASE(0,	Device, Func)+0x08);
		  ClassCode&=~0xff;
		  if(ClassCode==0x06040000){
		  SecondNum = MmioRead8(PCI_DEV_MMBASE(0,  Device, Func)+0x19);
		  if(SecondNum==((UINT8)BusNumber)){
			 IsFound = TRUE;
			 goto Found;
			}
		  }
		}
	}*/
	Debug_Print(L"Device In PCIE Port:%x-%x\n",Device,Func);
	if(BridgeCount>2){
		TempINTX = INTX;
		Debug_Print(L"Device Behind in PCIE-Bridge:");
		for(i=BridgeCount-1;i>1;i--){
			TempINTX = ((TempINTX + DeviceList[i]) % 4);
		}
		
		Debug_Print(L"INT%x Chang to INT%x\n",0xA+INTX,0xA+TempINTX);
		INTX = TempINTX;
	}
	if(gBoardType==IS_CND001_Board){
		if(Device==0x2&&Func==0){
			IrqNum = 0x18+INTX;
		}
		else if(Device==0x3&&Func==0){
			IrqNum = 0x18+4+INTX;
		}
		else if(Device==0x4&&Func==0){
			IrqNum = 0x18+8+INTX;
		}
		else if(Device==0x5&&Func==0){
			IrqNum = 0x18+12+INTX;
		}
		else if(Device==0x6&&Func==0){
			IrqNum = 0x18+16+INTX;
		}
		else if(Device==0x7&&Func==0){
			if(INTX==0x00||INTX==0x01){
				Value8 = 0x3;
				Status = gPciIo->Pci.Read (
						gPciIo,
						EfiPciIoWidthUint8,
						0x9B,
						1,
						&Value8
						);
				Status = gPciIo->Pci.Read (
						gPciIo,
						EfiPciIoWidthUint8,
						0x3D,
						1,
						&Value8
						);
			if(Value8==0x1||Value8==0x2){
				Print(L"Can't Change INx.Can't Test APIC in Port %x-%x\n",Device,Func);
				IrqNum = 0;
				return IrqNum;
			}
			else{
				IrqNum = 0x18+22;
			}
			}
		}
	  }
	  if(gBoardType==IS_CHX001_Board){
		if(Device==0x2&&Func==0){
			Rintrtapic = MmioRead8(PCI_DEV_MMBASE(0,0,0)+0x362);
			Debug_Print(L"Rintrtapic:%x\n",((Rintrtapic&0x30)>>4));
			if(((Rintrtapic&0x30)>>4)==0x00){
				 IrqNum = 0x18+8+INTX;
			}
			else if(((Rintrtapic&0x30)>>4)==0x01){
				if(INTX==0x3){
				  IrqNum =0x18+8;
				}
				else{
					IrqNum = 0x18+9+INTX;
				}
				
			}
			else if(((Rintrtapic&0x30)>>4)==0x2){
				if(INTX==0x0){
				  IrqNum =0x18+10;
				}
				else if(INTX==0x01){
					IrqNum =0x18+11;
				}
				else if(INTX==0x02){
					IrqNum =0x18+8;
				}
				else if(INTX==0x03){
					IrqNum =0x18+9;
				}
			}
			else if(((Rintrtapic&0x30)>>4)==0x3){
				if(INTX==0x0){
				  IrqNum =0x18+11;
				}
				else if(INTX==0x01){
					IrqNum =0x18+8;
				}
				else if(INTX==0x02){
					IrqNum =0x18+9;
				}
				else if(INTX==0x03){
					IrqNum =0x18+10;
				}
			}
		}
		else if(Device==0x3&&Func==0){
			IrqNum = 0x18+4+INTX;
		}
		else if(Device==0x3&&Func==1){
			IrqNum = 0x18+8+INTX;
		}
		else if(Device==0x3&&Func==2){
			IrqNum = 0x18+12+INTX;
		}
		else if(Device==0x3&&Func==3){
			IrqNum = 0x18+16+INTX;
		}
		else if(Device==0x3&&Func==3){
			IrqNum = 0x18+16+INTX;
		}
		else if(Device==0x4&&Func==0){
			IrqNum = 0x18+INTX;
		}
		else if(Device==0x4&&Func==1){
			if(INTX==0x00||INTX==0x01){
				Value8 = 0x3;
				Status = gPciIo->Pci.Read (
						gPciIo,
						EfiPciIoWidthUint8,
						0x9B,
						1,
						&Value8
						);
				Status = gPciIo->Pci.Read (
						gPciIo,
						EfiPciIoWidthUint8,
						0x3D,
						1,
						&Value8
						);
			if(Value8==0x1||Value8==0x2){
				Print(L"Can't Change INx.\nCan't Test APIC in Port %x-%x\n",Device,Func);
				IrqNum = 0;
				return IrqNum;
			}
			else{
				Debug_Print(L"Change to INTC\n");
				IrqNum = 0x18+22;
			}
		   }
		}
		else if(Device==0x5&&Func==0){
			Rintrtapic = MmioRead8(PCI_DEV_MMBASE(0,0,0)+0x360);
			Debug_Print(L"Rintrtapic:%x\n",((Rintrtapic&0x30)>>4));
			if(((Rintrtapic&0x30)>>4)==0x00){
				 IrqNum = 0x18+INTX;
			}
			else if(((Rintrtapic&0x30)>>4)==0x01){
				if(INTX==0x3){
				  IrqNum =0x18;
				}
				else{
					IrqNum = 0x18+1+INTX;
				}
				
			}
			else if(((Rintrtapic&0x30)>>4)==0x2){
				if(INTX==0x0){
				  IrqNum =0x18+2;
				}
				else if(INTX==0x01){
					IrqNum =0x18+3;
				}
				else if(INTX==0x02){
					IrqNum =0x18;
				}
				else if(INTX==0x03){
					IrqNum =0x18+1;
				}
			}
			else if(((Rintrtapic&0x30)>>4)==0x3){
				if(INTX==0x0){
				  IrqNum =0x18+3;
				}
				else if(INTX==0x01){
					IrqNum =0x18;
				}
				else if(INTX==0x02){
					IrqNum =0x18+1;
				}
				else if(INTX==0x03){
					IrqNum =0x18+2;
				}
			}
		}
		else if(Device==0x5&&Func==1){
			Rintrtapic = MmioRead8(PCI_DEV_MMBASE(0,0,0)+0x361);
			Debug_Print(L"Rintrtapic:%x\n",((Rintrtapic&0x30)>>4));
			if(((Rintrtapic&0x30)>>4)==0x00){
				 IrqNum = 0x18+4+INTX;
			}
			else if(((Rintrtapic&0x30)>>4)==0x01){
				if(INTX==0x3){
				  IrqNum =0x18+4;
				}
				else{
					IrqNum = 0x18+5+INTX;
				}
				
			}
			else if(((Rintrtapic&0x30)>>4)==0x2){
				if(INTX==0x0){
				  IrqNum =0x18+6;
				}
				else if(INTX==0x01){
					IrqNum =0x18+7;
				}
				else if(INTX==0x02){
					IrqNum =0x18+4;
				}
				else if(INTX==0x03){
					IrqNum =0x18+5;
				}
			}
			else if(((Rintrtapic&0x30)>>4)==0x3){
				if(INTX==0x0){
				  IrqNum =0x18+7;
				}
				else if(INTX==0x01){
					IrqNum =0x18+4;
				}
				else if(INTX==0x02){
					IrqNum =0x18+5;
				}
				else if(INTX==0x03){
					IrqNum =0x18+6;
				}
			}
		}
	  }
	  if((gBoardType==IS_CHX002_Board)||(gBoardType==IS_CHX02_Single_Board)){
        if(Device==0x3){
			IrqNum = 0x1C+(UINT16)(Func*4)+INTX;
		}
		else if(Device==0x4){
			IrqNum = 0x20+(UINT16)(Func*4)+INTX;
		}
		else if(Device==0x5){
			IrqNum = 0x24+(UINT16)(Func*4)+INTX;
		}
		else{
			 Print(L"Error Pcie Port\n");
		     IrqNum = 0x0;
		}
		
	  }
	return	IrqNum;
}

VOID Ahci_Report_Error(UINT16 Port)
{
   EFI_STATUS   Status;
   UINT16      Value16;
   Status = gPciIo->Pci.Read (
                        gPciIo,
                        EfiPciIoWidthUint16,
                        0x06,
                        1,
                        &Value16
                        );
   Print(L"Register_0x06:%x\n",Value16);
   Status = gPciIo->Pci.Read (
                        gPciIo,
                        EfiPciIoWidthUint16,
                        0x04,
                        1,
                        &Value16
                        );
  Print(L"Register_0x04:%x\n",Value16);
  Print(L"AHCI_GHC:%x\n",AhciReadReg (gPciIo,AHCI_GHC));
  Print(L"AHCI_IS:%x\n",AhciReadReg (gPciIo,0x08));
  Print(L"Port_IE:%x\n",AhciReadReg (gPciIo,AHCI_PORT_START+Port*AHCI_PORT_REG_WIDTH+AHCI_PORT_IE));
  Print(L"Port_IS:%x\n",AhciReadReg (gPciIo,AHCI_PORT_START+Port*AHCI_PORT_REG_WIDTH+AHCI_PORT_IS));
}
/*VOID IDE_Set_Port(UINT16 Port)
{

}
*/
VOID AHCI_Set_Port(UINT16 Port)
{
  
  UINT32 Value32;
  Value32 = 0xFFFFFFFF;
  //DEBUG ((EFI_D_ERROR, __FUNCTION__"\n"));
  AhciWriteReg(gPciIo, AHCI_PORT_START+Port*AHCI_PORT_REG_WIDTH+AHCI_PORT_IS,Value32);
  //Mike_Set Port 0 IS_E
  //Mike_Set Port 0 IE _S
  Value32 = AhciReadReg (gPciIo,AHCI_PORT_START+Port*AHCI_PORT_REG_WIDTH+AHCI_PORT_IE);
  //DEBUG((EFI_D_ERROR,"PxIE is %x\n",Value32));
  Value32 |=(DPE_INT|DHRE_INT);
  AhciWriteReg(gPciIo,AHCI_PORT_START+Port*AHCI_PORT_REG_WIDTH+AHCI_PORT_IE,Value32);
  //Mike_Set Port 0 IE_E
  //Mike_Set Global IS _S
  Value32 = 0xFFFFFFFF;
  AhciWriteReg(gPciIo,AHCI_IS,Value32);
  //Mike_Set Global IS_E
  //Mike_Set Global IE _S
  Value32 = AhciReadReg (gPciIo,AHCI_GHC);
  Value32 |=(1<<1);
  AhciWriteReg(gPciIo,AHCI_GHC,Value32);
  //Mike_Set Global IE_E
  
}
VOID AHCI_Clear_Port(UINT16 Port)
{
  
  UINT32 Value32;
  Value32 = 0xFFFFFFFF;
  //DEBUG ((EFI_D_ERROR, __FUNCTION__"\n"));
  AhciWriteReg(gPciIo, AHCI_PORT_START+Port*AHCI_PORT_REG_WIDTH+AHCI_PORT_IS,Value32);
  Value32 = AhciReadReg (gPciIo,AHCI_PORT_START+Port*AHCI_PORT_REG_WIDTH+AHCI_PORT_IE);
  Value32 &=~(DPE_INT|DHRE_INT);
  AhciWriteReg(gPciIo,AHCI_PORT_START+Port*AHCI_PORT_REG_WIDTH+AHCI_PORT_IE,Value32);
  Value32 = 0xFFFFFFFF;
  AhciWriteReg(gPciIo,AHCI_IS,Value32);
  Value32 = AhciReadReg (gPciIo,AHCI_GHC);
  Value32 &=~(1<<1);
  AhciWriteReg(gPciIo,AHCI_GHC,Value32);
  
}

#define AHCI_SB_IRQ 0x15
#define AHCI_NB_IRQ 0x18
VOID
EFIAPI
Mike_WriteLocalApicReg (
  IN UINTN  MmioOffset,
  IN UINT32 Value
  )
{
  ASSERT ((MmioOffset & 0xf) == 0);

  MmioWrite32 (0xfee00000 + MmioOffset, Value);
  return;
}
UINT32
EFIAPI
Mike_ReadLocalApicReg (
  IN UINTN  MmioOffset
  )
{
  ASSERT ((MmioOffset & 0xf) == 0);
  return MmioRead32 (0xfee00000 + MmioOffset);
}
VOID 
EFIAPI
DefaultSet()
{
  UINT32	Eax, Ebx;
  UINT32	cpuid;
  UINT32    Data32;
  AsmCpuid(1, &Eax, &Ebx, NULL, NULL);
  cpuid = (Ebx >> 24) & 0xFF;
  ////////////////////////////////////////////////////////////
  if(gDestMode==1){
  	if(cpuid<8)
    {
  	Data32 = Mike_ReadLocalApicReg (0xD0);
	Data32&=~0xFF000000;
	Data32|=((1<<cpuid)<<24);
	Mike_WriteLocalApicReg (0xD0,Data32);
  	}
	if(cpuid==8){
	  Data32 = Mike_ReadLocalApicReg (0xD0);
	  Data32&=~0xFF000000;
	  Data32|=(4<<24);
	  Mike_WriteLocalApicReg (0xD0,Data32);
	}
  }
  if(gDestMode==2){
  	Data32 = Mike_ReadLocalApicReg (0xD0);
	Data32&=~0xFF000000;
  	if(gNumberOfProcessors<=4){
		if(cpuid==0){
		  Data32|=(0x01<<24);
		}
		else if(cpuid==1){
		  Data32|=(0x02<<24);
		}
		else if(cpuid==2){
		  Data32|=(0x11<<24);
		}
		else if(cpuid==3){
			Data32|=(0x12<<24);
		}
		// Temp for CHX002 DualSocket Haps_S
		else if(cpuid==8){
			Data32|=(0x11<<24);
		}
		else if(cpuid==9){
			Data32|=(0x12<<24);
		}
		// Temp for CHX002 DualSocket Haps_E
  	}
	else if(gNumberOfProcessors==8){
		if(cpuid==0){
		  Data32|=(0x01<<24);
		}
		else if(cpuid==1){
		  Data32|=(0x02<<24);
		}
		else if(cpuid==2){
		  Data32|=(0x04<<24);
		}
		else if(cpuid==3){
			Data32|=(0x08<<24);
		}
		else if(cpuid==4){
			Data32|=(0x11<<24);
		}
		else if(cpuid==5){
			Data32|=(0x12<<24);
		}
		else if(cpuid==6){
			Data32|=(0x14<<24);
		}
		else if(cpuid==7){
			Data32|=(0x18<<24);
		}
	}
	else if(gNumberOfProcessors==16){
	   if(cpuid==0){
		  Data32|=(0x01<<24);
		}
		else if(cpuid==1){
		  Data32|=(0x02<<24);
		}
		else if(cpuid==2){
		  Data32|=(0x04<<24);
		}
		else if(cpuid==3){
			Data32|=(0x08<<24);
		}
		else if(cpuid==4){
			Data32|=(0x11<<24);
		}
		else if(cpuid==5){
			Data32|=(0x12<<24);
		}
		else if(cpuid==6){
			Data32|=(0x14<<24);
		}
		else if(cpuid==7){
			Data32|=(0x18<<24);
		}
		else if(cpuid==8){
			Data32|=(0x21<<24);
		}
		else if(cpuid==9){
			Data32|=(0x22<<24);
		}
		else if(cpuid==10){
			Data32|=(0x24<<24);
		}
		else if(cpuid==11){
			Data32|=(0x28<<24);
		}
		else if(cpuid==12){
			Data32|=(0x31<<24);
		}
		else if(cpuid==13){
			Data32|=(0x32<<24);
		}
		else if(cpuid==14){
			Data32|=(0x34<<24);
		}
		else if(cpuid==15){
			Data32|=(0x38<<24);
		}
	}
	Mike_WriteLocalApicReg (0xD0,Data32);
  }
  ///////////////////Edit TPR///////////////////////////////
  if(gDestMode==2){
  Data32 = Mike_ReadLocalApicReg (0x80);
        switch(cpuid){
			case 0:
				Data32=0x90;
				break;
			case 1:
				Data32=0x80;
				break;
			case 2:
				Data32=0x70;
				break;
			case 3:
				Data32=0x60;
				break;
			case 4:
				Data32=0x50;
				break;
			case 5:
				Data32=0x40;
				break;
			case 6:
				Data32=0x30;
				break;
			case 7:
				Data32=0x20;
				break;
			case 8:
				Data32=0x90;
				break;
			case 9:
				Data32=0x80;
				break;
			case 10:
				Data32=0x70;
				break;
			case 11:
				Data32=0x60;
				break;
			case 12:
				Data32=0x50;
				break;
			case 13:
				Data32=0x40;
				break;
			case 14:
				Data32=0x30;
				break;
			case 15:
				Data32=0x20;
				break;
        }
  // Temp for CHX002 DualSocket Haps_S
  if(gNumberOfProcessors<=4){
  	      if(cpuid==0x8){
		  	Data32=0x70;
  	      }
		  if(cpuid==0){
		  	Data32 = 0x90;
		  }
		  if(cpuid==1){
		  	Data32 = 0x70;
		  }
		  if(cpuid==0x9){
		  	Data32 =0x60;
		  }
  }
  // Temp for CHX002 DualSocket Haps_E
  Mike_WriteLocalApicReg (0x80,Data32);
  }
  /////////////////////////////////////////////////////////
}
VOID 
EFIAPI
LDR_TPR_Set(UINT8 cpuindex)
{
   //UINT32	Eax, Ebx;
   //UINT32	cpuid;
   UINT32   Index = 0;
   UINT64   val;
   HEX_CMD_INFO_64*  pTemp;
   //AsmCpuid(1, &Eax, &Ebx, NULL, NULL);
   //cpuid = (Ebx >> 24) & 0xFF;
   for(Index=0;Index<iLApic[cpuindex];Index++){
  	  pTemp = &LApic[cpuindex][Index];
	  val = *(u64*)(pTemp->addr);
	  if (pTemp->mask != 0) {
       *(u64*)(pTemp->addr) = val & ~pTemp->mask | pTemp->val &pTemp->mask;
	  }
	  
   }
}

VOID
EFIAPI
ProgramLogic(UINT8 cpuindex)
{
  UINT32	Eax, Ebx;
  UINT32	cpuid;
  UINT32    Data32;
  AsmCpuid(1, &Eax, &Ebx, NULL, NULL);
  cpuid = (Ebx >> 24) & 0xFF;
  //Clear LDR Void TPR Disable run Tools Again BSP TPR will into HIF 
  Data32 = Mike_ReadLocalApicReg (0x80);
  Data32=0x00;
  Mike_WriteLocalApicReg (0x80,Data32);
  Data32 = Mike_ReadLocalApicReg (0xD0);
  Data32&=~0xFF000000;
  Mike_WriteLocalApicReg (0xD0,Data32);
  ////////////////////////////////////////////////////////////////
  /////////////////////Edit DFR//////////////////////////////////
  Data32 = Mike_ReadLocalApicReg (0xE0);
  if(gDestMode==1){
  	Data32|=0xF0000000;
  }
  else if(gDestMode==2){
  	Data32&=~0xF0000000;
  }
  Mike_WriteLocalApicReg (0xE0,Data32);
  if(iLApic[cpuindex]==0){
     DefaultSet();
  }
  else{
     LDR_TPR_Set(cpuindex);
  }

  //if(gDebugMessage==1){
  	LApic[cpuid][0].val = Mike_ReadLocalApicReg(0xE0);
	LApic[cpuid][1].val = Mike_ReadLocalApicReg(0xD0);
	LApic[cpuid][2].val = Mike_ReadLocalApicReg(0x80);
  //}
  return ;
  
}
VOID DumpTPRMSR(IN  VOID  *Buffer)
{
  UINT32	Eax, Ebx;
  UINT32	cpuid;
  AsmCpuid(1, &Eax, &Ebx, NULL, NULL);
  cpuid = (Ebx >> 24) & 0xFF;
  TPRMSR[cpuid][0] = AsmReadMsr64(0x162B);
  TPRMSR[cpuid][1] = AsmReadMsr64(0x162C);
  TPRMSR[cpuid][2] = AsmReadMsr64(0x162D);
  TPRMSR[cpuid][3] = AsmReadMsr64(0x162E);
}
VOID pre_init (IN  VOID  *Buffer)
{

	UINT32	Eax, Ebx;
	UINT32	cpuid;
	EFI_STATUS	Status;
    UINT8 cpuindex= *(UINT8*)Buffer;
	AsmCpuid(1, &Eax, &Ebx, NULL, NULL);
	cpuid = (Ebx >> 24) & 0xFF;
	Status = mCpu->RegisterInterruptHandler(mCpu, gIRqNum, NULL);
    Status = mCpu->RegisterInterruptHandler(mCpu, gIRqNum, AHCI_ReadInterruptHandler);
    if (EFI_ERROR (Status)) {
    Print(L"Unable to register Sata interrupt with CPU Arch Protocol. Status is %d\n",Status);
	return ;
    }
	if(cpuid == 0x00)
	{
		//BSP, Read IDT table
		 AsmReadIdtr (&IdtrForBSP);
		 ProgramLogic(cpuindex);
	}else
	{
		//AP to fill the BSP's IDT table
		AsmWriteIdtr(&IdtrForBSP);
		ProgramVirtualWireMode ();
        DisableLvtInterrupts ();
	    EfiEnableInterrupts();
		ProgramLogic(cpuindex);
		if(cpuid==8||cpuid==1){
			IoWrite8(0x80,0x88);
		}
  	    while(CoreStop==FALSE);
		TPRMSR[cpuid][0] = AsmReadMsr64(0x162B);
        TPRMSR[cpuid][1] = AsmReadMsr64(0x162C);
        TPRMSR[cpuid][2] = AsmReadMsr64(0x162D);
        TPRMSR[cpuid][3] = AsmReadMsr64(0x162E);
	    if(cpuid==8||cpuid==1){
			IoWrite8(0x80,0x99);
		}
	    InterlockedIncrement(&OutProcessCPUNum);
	    if(cpuid==8||cpuid==1){
			IoWrite8(0x80,0xAA);
		}
	}	
    return;

}
VOID
EFIAPI
CallBack (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  )
{
  return;
}

void GlobVariableInit()
{
  UINT32 	   Index=0;
  for(Index=0;Index<16;Index++){
  	gIntProcessors[Index] = 0;
	iMsr[Index] = 0;
	iLApic[Index] = 0;
	ProcessorLocalApicID[Index] = 0;
  }
  HasEnterInterrupt = FALSE;
  CoreStop = FALSE;
  InProcessCPUNum = 0;
  OutProcessCPUNum = 0;
  IntNeedProcessors = 0;
  gNumberOfProcessors = 0;
  gBoardType =IS_CHX001_Board;
  gSlaveSocket = 0;
  
}
EFI_STATUS
EFIAPI
Ahci_APIC_Initialize (
  IN EFI_HANDLE                   Controller,
  EFI_BLOCK_IO_PROTOCOL 	*BlockIo,
  EFI_DEVICE_PATH_PROTOCOL  *DevicePath,
  UINT16                    Port,
  IN char                   **Argv
  )
{
  EFI_MP_SERVICES_PROTOCOL   *MpServices;
  UINTN		   NumberOfProcessors = 0;
  UINTN		   NumberOfEnabledProcessors;
  UINTN          ProcessorIndex=0;
  EFI_STATUS   Status;
  UINT8  Value8;
  UINT16       Value16;
  UINT32 	   Index=0;
  EFI_LEGACY_8259_PROTOCOL  *mLegacy8259;
  UINTN BusNumber;
  UINTN DeviceNumber;
  UINTN FunctionNumber;
  UINTN SegmentNumber;
  UINT8* pRBuffer1 = NULL;
  UINT8* pRBuffer2 = NULL; 
  UINTN  IRqNum = 0;
  UINT8  Io_Apic_Id = 0;
  UINTN  DeliveryMode=0;
  UINTN  Core=0; 
  EFI_EVENT   ApsEvent=NULL;
  UINTN    DestinationMode = 0;
  EFI_PROCESSOR_INFORMATION  ProcessorInfoBuffer;
  mPort = Port;
  Print(L"=======Sata  LAPIC Interrupt Test Start(V1.0)==============\n\n");
  fprintf(Fp, "%s\n","=======Sata  LAPIC Interrupt Test Start(V1.0)==============\n");
  Debug_Print(L"Board Type: ");
  switch(gBoardType){
  	case 1:
		 Debug_Print(L"CND001\n");
		 break;
    case 2:
		 Debug_Print(L"CHX001\n");
		 break;
	case 3:
		 Debug_Print(L"CHX002\n");
		 break;
	case 4:
		 Debug_Print(L"CHX002 Single\n");
		 break;
	default:
		Debug_Print(L"Error Board Type\n");
		gBoardType = 2;
		break;
  }
  Debug_Print(L"++++++++++Interrupt Information+++++++\n");
  
  //Read Data Before Enable Interrupt for compare with Enable Interrupt Read Data_S
  pRBuffer1 = AllocatePool(512);
  if(pRBuffer1==NULL){
  	  Print(L"Allocate_pRBuffer_Error\n");
	  goto Exit;
  }
  pRBuffer2 = AllocatePool(512);
  if(pRBuffer2==NULL){
  	  Print(L"Allocate_pRBuffer_Error\n");
	  goto Exit;
  }
  Ahci_APIC_Test(BlockIo,pRBuffer1);
  if(gPciIo==NULL)
  {  
	  Status = gBS->OpenProtocol (
                  Controller,
                  &gEfiPciIoProtocolGuid,
                  (VOID**)&gPciIo,
                  NULL,
                  NULL,
                  EFI_OPEN_PROTOCOL_GET_PROTOCOL
                  );
      if(Status!=0||gPciIo==NULL){
	  	 Print(L"mike_OpenPciIo_Error:%p\n",gPciIo);
	     goto Exit;
      }
  }

  Status= gPciIo->GetLocation(
  	                          gPciIo,
  	                          &SegmentNumber,
  	                          &BusNumber,
  	                          &DeviceNumber,
  	                          &FunctionNumber
  	                          );
  if(Status!=0){
  	Print(L"mike_Pci_GetLocation_Error:%x\n",Status);
	goto Exit;
  }
  if(BusNumber>=SlaveSocketBus){
  	gSlaveSocket = 1;
	Debug_Print(L"AHCI Card in SlaveSocket\n");
  }
  else{
  	if(gBoardType==IS_CHX002_Board){
		Debug_Print(L"AHCI Card in MasterSocket\n");
  	}
  }
  Debug_Print(L"Bus:%x;Device:%x;Func:%x;Port:%x\n",BusNumber,DeviceNumber,FunctionNumber,Port);
  if(BusNumber>=1){
  	Status = gPciIo->Pci.Read(
                        gPciIo,
                        EfiPciIoWidthUint8,
                        0x3D,
                        1,
                        &Value8
                        );
	switch(Value8)
	{
	case 1:
		Debug_Print(L"Device USE  INTA\n");
		break;
	case 2:
		Debug_Print(L"Device USE  INTB\n");
		break;
	case 3:
		Debug_Print(L"Device USE  INTC\n");
		break;
	case 4:
		Debug_Print(L"Device USE  INTD\n");
		break;
	default:
		Debug_Print(L"Not Found Device USE INTX\n");
		break;
	}
  	IRqNum = Find_Device_PCIEPort(DevicePath,Value8-1);
	if(IRqNum==0){
	Print(L"Can't find IRQ\n");
	while(1);
	}
	else{
     Debug_Print(L"IRQ=%x\n",IRqNum);
	}
	if(gBoardType==IS_CHX002_Board){
	     if(!gSlaveSocket){
	        Io_Apic_Id = MASTER_NB_IO_APIC_ID;
  	        IO_Apic_Base = MASTER_NB_IO_APIC_BASE;
	        //enable nb apic
	        MmioWrite8(APIC_PCI_REG(0x240),MmioRead8(APIC_PCI_REG(0x240))|BIT7);
	     }
		 else{
		    Io_Apic_Id = SLAVE_NB_IO_APIC_ID;
  	        IO_Apic_Base = SLAVE_NB_IO_APIC_BASE;
	        //enable nb apic
	        MmioWrite8(SLAVE_APIC_PCI_REG(0x240),MmioRead8(SLAVE_APIC_PCI_REG(0x240))|BIT7);
		 }
    }
	else{
	   Io_Apic_Id = NB_IO_APIC_ID;
  	   IO_Apic_Base = NB_IO_APIC_BASE;
	   //enable nb apic
	  MmioWrite8(APIC_PCI_REG(0x240),MmioRead8(APIC_PCI_REG(0x240))|BIT7);
	}

  }
  else{
  	Io_Apic_Id = SB_IO_APIC_ID;
  	IO_Apic_Base = SB_IO_APIC_BASE;
    IRqNum = AHCI_SB_IRQ;
	//enable sb apic
    MmioWrite8(LPC_PCI_REG(0x58),MmioRead8(LPC_PCI_REG(0x58))|BIT6);
  }
 //Debug_Print(L"IO_Apic_Base=%x\n",IO_Apic_Base);
 //set  APIC ID ;
 MmioWrite8(IO_Apic_Base, 0x00);
 MmioWrite32(IO_Apic_Base + 0x10,Io_Apic_Id<<24);
 //set  APIC use FSB,Interrupt Delivery Mechanism is a Front-side Bus Message
 MmioWrite8(IO_Apic_Base, 0x03);
 MmioWrite32(IO_Apic_Base+ 0x10, 0x01);

 // Find the 8259  protocol.
 Status = gBS->LocateProtocol (&gEfiLegacy8259ProtocolGuid, NULL, (VOID **) &mLegacy8259);
 ASSERT_EFI_ERROR (Status);
 // Find the CPU architectural protocol.
 Status = gBS->LocateProtocol (&gEfiCpuArchProtocolGuid, NULL, (VOID **) &mCpu);
 ASSERT_EFI_ERROR (Status);
 if(gFSBC==1){
   //FSBC_Config(FSBC_SIZE_256MB,FSBC_All);
 }
 Status = gBS->LocateProtocol (&gEfiLegacy8259ProtocolGuid, NULL, (VOID **) &mLegacy8259);
 ASSERT_EFI_ERROR (Status);
 //Status = mCpu->Init(mCpu, 0x80+IRqNum);
 ////////////////////////////////////////////////////////////
 //Get MP_Service Protocol
  Status = gBS->LocateProtocol (&gEfiMpServiceProtocolGuid, NULL, (VOID**)&MpServices);
  if (EFI_ERROR (Status)) {
  	  Print(L"Unable to initialize MP protocol interface!");
	  ASSERT_EFI_ERROR (Status);
  }
  if(MpServices) {
	 Status = MpServices->GetNumberOfProcessors (MpServices, &NumberOfProcessors, &NumberOfEnabledProcessors);
	 if(!EFI_ERROR(Status))
	  Debug_Print(L"%d processors found, %d enabled!\n",NumberOfProcessors, NumberOfEnabledProcessors);
     gNumberOfProcessors = NumberOfProcessors;
	 for(Index=0;Index<NumberOfProcessors;Index++){
	 Status = MpServices->GetProcessorInfo(MpServices,Index,&ProcessorInfoBuffer);
	 if(Status!=0){
	 	Print(L"GetProcessorInfo Error:%x\n",Status);
		break;
	 }
	 ProcessorLocalApicID[Index] = (UINT8)(ProcessorInfoBuffer.ProcessorId);
	 }
  } 
 if(strcmp(Argv[1],"FX")==0){
  	DeliveryMode = IO_APIC_DELIVERY_MODE_FIXED;
  }
  else if(strcmp(Argv[1],"LP")==0){
  	DeliveryMode = IO_APIC_DELIVERY_MODE_LOWEST_PRIORITY;
  }
  if(strcmp(Argv[2],"PH")==0){
  	 DestinationMode = 0;
	 gDestMode = 0;
	 IntNeedProcessors = 1;
	 Core =strtol(Argv[3], (char **)NULL, 16);
	 //Core = atoi(Argv[3]);
	 if(Core>ProcessorLocalApicID[NumberOfProcessors-1]){
	 	Print(L"Core More than the Max Processor\n");
		Core = ProcessorLocalApicID[NumberOfProcessors-1];
	 }
	 Debug_Print(L"PH:Core:%x\n",Core);
  }
  else if(strcmp(Argv[2],"LF")==0)
  {
     DestinationMode = 1;
	 gDestMode = 1;
	 Core =strtol(Argv[3], (char **)NULL, 16);
	 //Core = atoi(Argv[3]);
	 Core &=0xFF;
	 for(Index=0;Index<8;Index++){
	 	if(((Core>>Index)&0x1)==1){
			IntNeedProcessors++;
	 	}
	 }
	 Debug_Print(L"LF:SetCore:%x\n",IntNeedProcessors);
	 if(IntNeedProcessors>=NumberOfProcessors){
	 	IntNeedProcessors = (UINT32)NumberOfProcessors;
	 }
	 if(strcmp(Argv[1],"LP")==0){
	 	IntNeedProcessors = 1;
	 }
	 Debug_Print(L"LF:NeedCore:%x\n",IntNeedProcessors);
  }
  else if(strcmp(Argv[2],"LC")==0){
  	 DestinationMode = 1;
	 gDestMode = 2;
	 IntNeedProcessors = 1;
	 Core =strtol(Argv[3], (char **)NULL, 16);
	 Debug_Print(L"LC:SetCore:%x\n",Core);
  }
  gIRqNum = 0x90+IRqNum;
  if(gIRqNum>=0xF0){
  	gIRqNum = 0xF0;
  }
  if(strcmp(Argv[4],"-v")==0||strcmp(Argv[6],"-v")==0){
  	 if(strcmp(Argv[4],"-v")==0){
  	   gIRqNum =strtol(Argv[5], (char **)NULL, 16);
	 }
	 else if(strcmp(Argv[6],"-v")==0){
	  gIRqNum =strtol(Argv[7], (char **)NULL, 16);
	 }
  }
  Debug_Print(L"Vector:%x\n",gIRqNum);
  Debug_Print(L"+++++++++++++++End++++++++++++++++++++\n");
  if(strcmp(Argv[4],"-f")==0||strcmp(Argv[6],"-f")==0){
  	Debug_Print(L"+++++++++++Parse Script++++++++++++++++\n");
	 if(strcmp(Argv[4],"-f")==0){
  	   Interrupt_execute_bat(Argv[5]);
	 }
	 else if(strcmp(Argv[6],"-f")==0){
	 	Interrupt_execute_bat(Argv[7]);
	 }
	 ProcessorIndex = 0;
	 execute_bat((VOID*)&ProcessorIndex);
	 for(ProcessorIndex=1;ProcessorIndex<NumberOfProcessors;ProcessorIndex++){
	  Status = MpServices->StartupThisAP(MpServices,execute_bat,ProcessorIndex,NULL,0,(VOID*)&ProcessorIndex,NULL);
     }
	Debug_Print(L"++++++++++++++End++++++++++++++++++++++\n");
  }
  ProcessorIndex = 0;
  pre_init((VOID*)&ProcessorIndex);
  Status = gBS->CreateEvent (
                  EVT_NOTIFY_SIGNAL,
                  TPL_NOTIFY,
                  CallBack,
                  NULL,
                  &ApsEvent
                  );
  for(ProcessorIndex=1;ProcessorIndex<NumberOfProcessors;ProcessorIndex++){

	  Status = MpServices->StartupThisAP(MpServices,pre_init,ProcessorIndex,ApsEvent,0,(VOID*)&ProcessorIndex,NULL);
  }
  Debug_Print(L"++++++++DFR LDR TPR Information+++++++\n");
  for(Index=0;Index<NumberOfProcessors;Index++){
  	Debug_Print(L"Core:%x ",ProcessorLocalApicID[Index]);
  	Debug_Print(L"DFR:%x ",LApic[ProcessorLocalApicID[Index]][0].val);
	Debug_Print(L"LDR:%x ",LApic[ProcessorLocalApicID[Index]][1].val);
    Debug_Print(L"TPR:%x\n",LApic[ProcessorLocalApicID[Index]][2].val);
  }
  Debug_Print(L"++++++++++++End++++++++++++++++++++++++\n");
 //////////////////////////////////////////////////////////////
  //Debug_Print(L"Core:%x\n",Core); 
  Mike_IoApicConfigureInterrupt (IRqNum%0x18, gIRqNum, DeliveryMode,DestinationMode,TRUE, FALSE,Core);
  //DEBUG((EFI_D_ERROR,"IoApicConfigureInterrupt\n"));
  //Mike_Enable_CMD.ID_S
  Status = gPciIo->Pci.Read (
                        gPciIo,
                        EfiPciIoWidthUint16,
                        0x04,
                        1,
                        &Value16
                        );
  Value16&=~(1<<10);
  Status = gPciIo->Pci.Write(
                        gPciIo,
                        EfiPciIoWidthUint16,
                        0x04,
                        1,
                        &Value16
                        );
  //Mike_Enable_CMD.ID_E
  
  AHCI_Set_Port(Port);
  Mike_IoApicEnableInterrupt(IRqNum%0x18,TRUE);  
  gFSBC_Begin = 1;
 {
  Status = mLegacy8259->DisableIrq (mLegacy8259, 0);
  ASSERT_EFI_ERROR (Status);
  Ahci_APIC_Test(BlockIo,pRBuffer2);
  gBSPProcessorHere = 1;
  mLegacy8259->EnableIrq (mLegacy8259, 0, FALSE);
  ASSERT_EFI_ERROR (Status);
 }
  Mike_MicroSecondDelay(500000);
  CoreStop = TRUE;
  Mike_MicroSecondDelay(1000000);
  Debug_Print(L"Num of %x Aps Leave StartupAPs\n",OutProcessCPUNum);
  Debug_Print(L"+++++++++++Result++++++++++++++++++++++\n");
  Index = 0;
  while(HasEnterInterrupt==FALSE)
  {
	  //DEBUG((EFI_D_ERROR,"HasEnterInterrupt%x\n",HasEnterInterrupt));

	Index++;
	if(Index>=0x100000){
	Print(L"AHCI APIC Interrupt Fail\n");
	fprintf(Fp, "%s\n","AHCI APIC Interrupt Fail\n");
	goto Exit;
	}
  }
  for(Index=0;Index<512;Index++){
  	if(pRBuffer1[Index]!=pRBuffer2[Index]){
		Print(L"Read_Data_Error:(Index:%x)(First:%x;Second:%x)\n",Index,pRBuffer1[Index],pRBuffer2[Index]);
		goto Exit;
  	}
  }
  for(Index=0;Index<16;Index++){
  	if(gIntProcessors[Index]==1){
		Print(L"Processor:%x Entered Interrupt\n",Index);
		fprintf(Fp, "Processor:%x Entered Interrupt\n",Index);
  	}
  }
  //MmioWrite8(IO_Apic_Base, (UINT8)(0x10+(IRqNum%0x18)*2));
  //MmioWrite8(IO_Apic_Base, (UINT8)(0x10+(IRqNum%0x18)*2+1));
  Debug_Print(L"IRQ_Entry_High:%x\n",MmioRead64(IO_Apic_Base+0x10));
  Print(L"AHCI LAPIC Interrupt Success\n");
  fprintf(Fp, "%s\n","AHCI LAPIC Interrupt Success\n");
  Debug_Print(L"+++++++++++End+++++++++++++++++++++++++\n\n");
  if(gDebugMessage){
   Debug_Print(L"++++++++TPR MSR Information+++++++\n");
   TPRMSR[0][0] = AsmReadMsr64(0x162B);
   TPRMSR[0][1] = AsmReadMsr64(0x162C);
   TPRMSR[0][2] = AsmReadMsr64(0x162D);
   TPRMSR[0][3] = AsmReadMsr64(0x162E);
   for(Index=0;Index<NumberOfProcessors;Index++){
   	  Temp=ProcessorLocalApicID[Index];
   	  Print(L"Core:%x ",Temp);
      Print(L"162B~E:%llx %llx %llx %llx\n",TPRMSR[Temp][0],TPRMSR[Temp][1],TPRMSR[Temp][2],TPRMSR[Temp][3]);
   }
   Debug_Print(L"++++++++++++End++++++++++++++++++++++++\n");
  }
  Print(L"===========================================================\n");
  fprintf(Fp, "%s\n","=============================================================\n");
  HasEnterInterrupt=FALSE;
  Mike_IoApicEnableInterrupt(IRqNum%0x18,FALSE);
  //Status = mCpu->RegisterInterruptHandler(mCpu, gIRqNum, NULL);
  //AHCI_Clear_Port(Port);
  FreePool(pRBuffer1);
  FreePool(pRBuffer2);
  Status =gBS->CloseProtocol(
  	                Controller, 
  	                &gEfiPciIoProtocolGuid,
                    NULL,
                    NULL
                    );
  Status = gBS->CloseEvent(ApsEvent);
  return EFI_SUCCESS;
Exit:
  Ahci_Report_Error(Port);
  MmioWrite8(IO_Apic_Base, (UINT8)(0x10+(IRqNum%0x18)*2));
  Print(L"IRQ_Entry_Low:%x\n",MmioRead64(IO_Apic_Base+0x10));
  if(MmioRead64(IO_Apic_Base+0x10)&(1<<14)){
  	Print(L"Send Eoi\n");
  	SendApicEoi ();
	Print(L"IRQ_Entry_Low:%x\n",MmioRead64(IO_Apic_Base+0x10));
  }
  MmioWrite8(IO_Apic_Base, (UINT8)(0x10+(IRqNum%0x18)*2+1));
  Print(L"IRQ_Entry_High:%x\n",MmioRead64(IO_Apic_Base+0x10));
  HasEnterInterrupt=FALSE;
  Status = mCpu->RegisterInterruptHandler(mCpu, gIRqNum, NULL);
  Mike_IoApicEnableInterrupt(IRqNum%0x18,FALSE);
  AHCI_Clear_Port(Port);
  FreePool(pRBuffer1);
  FreePool(pRBuffer2);
  Status =gBS->CloseProtocol(
  	                Controller, 
  	                &gEfiPciIoProtocolGuid,
                    NULL,
                    NULL
                    );
  Status = gBS->CloseEvent(ApsEvent);
  return EFI_SUCCESS;
}
///////////////////////////Mike_Add_Test_ACHI_Code  ////////////////////////////
int
main (
  IN int Argc,
  IN char **Argv
  )

{
  EFI_STATUS                      Status;
  EFI_HANDLE                      SataController = NULL;
  EFI_BLOCK_IO_PROTOCOL           *zBlockIo;
  UINT8                           Port;
  UINT32							i=0;
  UINT32						     j=0;
  int                                Index=0;
  UINT8								 Find_Flag=0;
  EFI_HANDLE							 *HandleBIOBuffer;
  UINTN								 NumberOfBIOHandles;
  EFI_HANDLE							 *HandleDIFBuffer;
  UINTN								 NumberOfDIFHandles;
  EFI_DEVICE_PATH_PROTOCOL			 *DevicePath;
  EFI_DEVICE_PATH_PROTOCOL			 *DevPathNode;
  SHELL_STATUS                         ShellStatus;
  ATA_DEVICE                           *AtaDevice;

  ShellStatus = SHELL_SUCCESS;
  GlobVariableInit();
  if(strcmp(Argv[4],"-i")==0||strcmp(Argv[6],"-i")==0||strcmp(Argv[8],"-i")==0){
  	gDebugMessage = 1;
  }
  for(Index=0;Index<Argc;Index++){
  	if(strcmp(Argv[Index],"-fsbc")==0){
		gFSBC = 1;
		break;
  	}
  }
  for(Index=0;Index<Argc;Index++){
  	if(strcmp(Argv[Index],"-b")==0){
		gBoardType =(UINT8)strtol(Argv[Index+1], (char **)NULL, 16);
		break;
  	}
  }
	////////////Find BlockIoProtocol////////////////////////////
  Status = gBS->LocateHandleBuffer (
					 ByProtocol,
					 &gEfiBlockIoProtocolGuid,
					 NULL,
					 &NumberOfBIOHandles,
					 &HandleBIOBuffer
					 );
  if (EFI_ERROR (Status))
  {
      Print(L"LocateBlockIoProtocol Error!\n");
	  return (ShellStatus);
  }
  Status = gBS->LocateHandleBuffer (
					 ByProtocol,
					 &gEfiDiskInfoProtocolGuid,
					 NULL,
					 &NumberOfDIFHandles,
					 &HandleDIFBuffer
					 );
  if (EFI_ERROR (Status))
  {
      Print(L"LocateDiskInfoProtocol Error!\n");
	  return (ShellStatus);
  }

  for (i = 0; i < NumberOfBIOHandles; i++)
  {
	   for(j=0;j<NumberOfDIFHandles;j++)
	   {
		 if(HandleBIOBuffer[i]==HandleDIFBuffer[j])
		  {
  
			Status = gBS->HandleProtocol (
					   HandleBIOBuffer[i],
					   &gEfiDevicePathProtocolGuid,
					   (VOID**) &DevicePath
					   );
  
				DevPathNode = DevicePath;
				while (!IsDevicePathEnd (DevPathNode))
				{
				
				 if((DevPathNode->Type==0x3)&&(DevPathNode->SubType==0x1))
				  {    
				       //Debug_Print(L"Sata IDE Mode\n");
					   Find_Flag = 1;
					   break;
				  }
				 else if((DevPathNode->Type==0x3)&&(DevPathNode->SubType==0x12))
				  {
				       //Debug_Print(L"Sata AHCI Mode\n");
					   Find_Flag = 1;
					   break;
				  }
				 DevPathNode = NextDevicePathNode (DevPathNode);
				}
  
			if(Find_Flag==1)
			{
			  DevPathNode = DevicePath;
			  if(!IsDevicePathEnd (DevPathNode))
			  {
			   //Print(L"(%d,%d)",DevPathNode->Type,DevPathNode->SubType);
			   DevPathNode = NextDevicePathNode (DevPathNode);
			  }
			  while(!IsDevicePathEnd (DevPathNode))
			  {
			   //Print(L"/(%d,%d)",DevPathNode->Type,DevPathNode->SubType);
			   DevPathNode = NextDevicePathNode (DevPathNode);
			  }
			  //Print(L"\n");
  
			  /*DevPathNode = DevicePath;
			  if (!IsDevicePathEnd (DevPathNode))
			  {
			   Textdevicepath=Device2TextProtocol->ConvertDevicePathToText(DevPathNode,TRUE,TRUE);
			   Print(L"%s",Textdevicepath);
			   if(Textdevicepath) gBS->FreePool(Textdevicepath);
			  }*/
			  //Print(L"\n");
			  break;
			}
		  }
	   }
	   if(Find_Flag==1)
	   {
		 break;
	   }
  }
  if(Find_Flag==1)
  {
       //SataHandle = HandleBIOBuffer[i];
	  Status = gBS->HandleProtocol (
					   HandleBIOBuffer[i],
					   &gEfiBlockIoProtocolGuid,
					   (VOID**) &zBlockIo
					   );
	  
	  ASSERT_EFI_ERROR (Status);
	  //Print(L"BlockIo open\n");
	  AtaDevice = ATA_DEVICE_FROM_BLOCK_IO(zBlockIo);
	  SataController = AtaDevice->AtaBusDriverData->Controller;
  }
  else
  {
      Print(L"ERROR: Can't find BlockIO!\n");
	  return (ShellStatus);
  }

  Port = (UINT8)AtaDevice->Port;
  //Debug_Print(L"port is %d!\n",Port);

  if(strcmp(Argv[1],"-?")==0||strcmp(Argv[1],"help")==0||Argc<2){
  	Print(L"=======================Help Information=====================================================\n\n");
  	Print(L"AhciInterrupt.efi DeliveryMode DestinationMode Destination [-f Script] [-v VetorNumber] [-i] [-b BoardType]\n");
    Print(L"DeliveryMode----LP:Lowest Priority Mode; FX:Fixed Mode\n");
	Print(L"DestinationMode---PH:Phsical Mode ;LC:Logical Cluster Mode; LF: Logical Flat Mode\n");
	Print(L"Destination--- Set Interrupt to One Or Multiple Processor\n");
    Print(L"Script---Script FileName,Can Edit Register,MSR,LocalAPIC and Pass to Tool\n");
    Print(L"VetorNumer---Set a VetorNumber to Interrupt\n");
	Print(L"-i--- Display Debug Information \n");
	Print(L"-b--- BoardType 1:CND001 2:CHX001 3:CHX002 Default:CHX001\n\n");
	Print(L"++++++++Simple Example++++++++++++++++++++++++++++++++++++\n");
	Print(L"AhciInterrupt.efi FX PH 0x1 \n");
	Print(L"Test Fixed Phsical Interrupt, set Destination Core 1\n");
	Print(L"++++++++Advanced Example++++++++++++++++++++++++++++++++++\n");
	Print(L"AhciInterrupt.efi LP LC 0xFF -f TPR.txt -v 0x99 -i\n");
	Print(L"Test Lowest Pority Logical Cluster Mode, give Tool TPR.txt\n");
    Print(L"sript to set set 0x99 as Vector and Display Deubg INfo\n\n");
	Print(L"==============================================================================================\n");
	return 0;
  }
  Fp = fopen("Interrupt_LOG.txt", "a+");
  if(!Fp){
  	Print(L"Open Interrup_LOG.txt fail\n");
  }
  for(Index=0;Index<Argc;Index++){
  	 fprintf(Fp, "%s ", Argv[Index]);
  }
  fprintf(Fp, "\n");
  Ahci_APIC_Initialize(SataController,zBlockIo,AtaDevice->DevicePath,Port,Argv);
  fclose(Fp);
  return 0;
}

