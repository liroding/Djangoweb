/*
 * Hif_Checklist.c
 *
 *  Created on: Jun 17, 2019
 *      Author: liroding
 */

#include "Hif_Checklist.h"
#include "ReadConf.h"
#include <Library/IoLib.h>

UINT64 mmiobase;
UINT32 SlaveMmioBase;
UINT32 lowtopA;

UINT8  DualSocket;
UINT8  SlaveBusNum;

volatile UINT8 sync_flag=0;

VOID EFIAPI SyncState(
	 IN  EFI_EVENT		 Event,
	 IN  VOID			 *Context
 	)
{
	 UINT64 Address;
	 sync_flag = 0x5A;
}

VOID EFIAPI Cpu1Cycle(
	 IN  EFI_EVENT		 Event,
	 IN  VOID			 *Context
 	)
{


	UINT64 Address,loop;
	UINT32 Data = 0;

	Address = 0x100000000;  //4G
    for(loop=0;loop<0x1000;loop++)
    {
    //	 Address =  AsiaPciAddress(CHX002_HB_BUS, CHX002_HB_DEV, CHX002_HIF_FUN, 0x01);
    //     AsiaMemoryWrite8(AsiaGetPcieMmioAddress(0x80000000, Address),0x11);
         MemoryWrite32(Address+QemuBase+loop*4,loop);
         Data = MemoryRead32(Address+QemuBase+loop*4);
    }

}


/*
void EFIAPI CpuNCycle(
	   void		*Context
 	)
{
	UINT8 *pCoreTarget,targetnode;

    UINT8 Bus,Dev,Func;
	ASIA_HIF_CONFIGURATION  *pHIFCfg;
	UINT32 Pciebase;
	UINT64 addr,beg,end,data;
	UINT8 i;


	pCoreTarget = (UINT8 *)Context;

	Debug_Print_t((DEBUG_LOG," CoreTarget = 0x%x \n\r\n",*pCoreTarget));

    if(*pCoreTarget < 8 )
    	targetnode = 0;
    else if((8 <=*pCoreTarget) && (*pCoreTarget<16))
    	targetnode = 1;
    else if((16 <=*pCoreTarget) && (*pCoreTarget<24) )
    	targetnode = 2;
    else if((24 <=*pCoreTarget) && (*pCoreTarget<32) )
    	targetnode = 3;
    else if((32 <=*pCoreTarget) && (*pCoreTarget<40))
    	targetnode = 4;
    else if((40 <=*pCoreTarget) && (*pCoreTarget<48) )
    	targetnode = 5;
    else if((48 <=*pCoreTarget) && (*pCoreTarget<56) )
    	targetnode = 6;
    else if((56 <=*pCoreTarget) && (*pCoreTarget<64) )
    	targetnode = 7;

	Pciebase = CoSimInstance.IniArg.PlatformArg_t.devarg.cfgarg.pciebaseaddr;
	pHIFCfg = CoSimInstance.pHIFPrvData_t->AsiaHIFPpi.HIFCfg;



    Bus  = pHIFCfg->Die[targetnode/2][targetnode%2].BusNum ;
    Dev  = pHIFCfg->Die[targetnode/2][targetnode%2].CosimDevNum;
    Func = pHIFCfg->Die[targetnode/2][targetnode%2].CosimFunNum;

    //read cfg add for debug
    for(i=0;i<0x10;i++)
    {
    	addr = AsiaPciAddress(Bus,Dev,Func,0);
    	AsiaMemoryRead32(AsiaGetPcieMmioAddress(Pciebase,addr));
    }

    GEN(&CoSimInstance,0, NULL);

}


void HCSS_HIF_Checklist_MulApNoBlock(COSIM_INSTANCE *CosimAttr,int argc, char *argv[])
{
	UINT64 addr=0;
	UINT8 Index;
	UINT32 Data;

	EFI_STATUS	Status;
	EFI_MP_SERVICES_PROTOCOL   *MpServices;
	//UINT8 		*Flag;
	UINTN		   NumberOfProcessors;
	UINTN		   NumberOfEnabledProcessors;
    UINT64 i;

	EFI_EVENT		  SyncEvent;
	UINT8			  finish;


    NumberOfProcessors = NumberOfEnabledProcessors = 1;
    Status = gST->BootServices->LocateProtocol (&gEfiMpServiceProtocolGuid, NULL, (VOID **)&MpServices);
	if(EFI_ERROR(Status)) {
	    Print(L"MP Service Protocol is un-available!\n");
	    gST->ConOut->OutputString (gST->ConOut, L"MP Service Protocol is un-available!\n\r");
	    MpServices = NULL;
	} else {
	    ;//Print(L"MP Service Protocol available!\n");
	}
	Status = EFI_SUCCESS;
	if(MpServices) {
	    Status = MpServices->GetNumberOfProcessors (MpServices, &NumberOfProcessors, &NumberOfEnabledProcessors);
	    if(!EFI_ERROR(Status))
	    	Print(L"%d processors found, %d enabled!\n", NumberOfProcessors, NumberOfEnabledProcessors);
	}

	Status = gBS->CreateEvent (
					EVT_NOTIFY_SIGNAL,
					TPL_NOTIFY,
					SyncState,
					NULL,
					&SyncEvent
					);
	if (EFI_ERROR (Status)) {
		  return ;
	}

	if(NumberOfEnabledProcessors > 1) {
			if(MpServices) {
				//Status = MpServices->StartupAllAPs (MpServices, Cpu1SnoopTest, TRUE, SyncState, 0x100000, NULL, NULL);
				printf("[LIRO-DEBUG ]sync_flag =%x\n",sync_flag);
				Status = MpServices->StartupThisAP (MpServices, Cpu1Cycle, 1, SyncEvent, 0, NULL, &finish);
				if(EFI_ERROR(Status)) {
					Print(L"Start AP fail!\n");
					NumberOfEnabledProcessors = 1;
				}
			//	printf("[before 3 ]sync_flag =%x\n",sync_flag);

			}
		}

	do{

	    	 Data = MemoryRead32(0x0+QemuBase);
	}while(finish==0 || sync_flag !=0x5a);
    printf("[LIRO-DEBUG ] finish=%x sync_flag=%x\n",finish,sync_flag);
    printf("Ap Core finish !!! \n");

}
*/

void HCSS_HIF_Checklist_MulApBlock(COSIM_INSTANCE *CosimAttr,int argc, char *argv[])
{
	UINT64 addr=0;
	UINT8 Index;
	UINT32 Data;

	EFI_STATUS	Status;
	EFI_MP_SERVICES_PROTOCOL   *MpServices;
	//UINT8 		*Flag;
	UINTN		   NumberOfProcessors;
	UINTN		   NumberOfEnabledProcessors;
    UINT64 i;

	EFI_EVENT		  SyncEvent;
	UINT8			  finish;


    NumberOfProcessors = NumberOfEnabledProcessors = 1;
    Status = gST->BootServices->LocateProtocol (&gEfiMpServiceProtocolGuid, NULL, (VOID **)&MpServices);
	if(EFI_ERROR(Status)) {
	    Print(L"MP Service Protocol is un-available!\n");
	    gST->ConOut->OutputString (gST->ConOut, L"MP Service Protocol is un-available!\n\r");
	    MpServices = NULL;
	} else {
	    ;//Print(L"MP Service Protocol available!\n");
	}
	Status = EFI_SUCCESS;
	if(MpServices) {
	    Status = MpServices->GetNumberOfProcessors (MpServices, &NumberOfProcessors, &NumberOfEnabledProcessors);
	    if(!EFI_ERROR(Status))
	    	Print(L"%d processors found, %d enabled!\n", NumberOfProcessors, NumberOfEnabledProcessors);
	}

	Status = gBS->CreateEvent (
					EVT_NOTIFY_SIGNAL,
					TPL_NOTIFY,
					SyncState,
					NULL,
					&SyncEvent
					);
	if (EFI_ERROR (Status)) {
		  return ;
	}

	if(NumberOfEnabledProcessors > 1) {
			if(MpServices) {
				//Status = MpServices->StartupAllAPs (MpServices, Cpu1SnoopTest, TRUE, SyncState, 0x100000, NULL, NULL);

				Status = MpServices->StartupThisAP (MpServices, Cpu1Cycle, 1, NULL, 0, NULL, &finish);
				if(EFI_ERROR(Status)) {
					Print(L"Start AP fail!\n");
					NumberOfEnabledProcessors = 1;
				}
			//	printf("[before 3 ]sync_flag =%x\n",sync_flag);
			}
		}

	do{
		 //  addr =  AsiaPciAddress(CHX002_HB_BUS, CHX002_HB_DEV, CHX002_HIF_FUN, 0x00);
	     //  AsiaMemoryWrite8(AsiaGetPcieMmioAddress(0x80000000, addr),0x10);
		  //  Data = *(volatile UINT32 *)(UINT64)(0+QemuBase);
		    Data = MemoryRead32(0+QemuBase);
	}while(finish==0);
    printf("[LIRO-DEBUG ] finish=%x\n",finish);
    printf("ap Core finish !!! \n");

}

/*


void COSIM_HIF_Checklist_CoreExecute(COSIM_INSTANCE *CosimAttr,int argc, char *argv[])
{


	UINT64 addr=0;
	UINT8 CoreSource,CoreTarget;
	UINT32 Pciebase;
	UINT8 bus,dev,func;
	ASIA_HIF_CONFIGURATION  *pHIFCfg;

	UINT8 Index;
	UINT32 Data;

	EFI_STATUS	Status;
	EFI_MP_SERVICES_PROTOCOL   *MpServices;

	UINTN		   NumberOfProcessors;
	UINTN		   NumberOfEnabledProcessors;
    UINT64 i;

	EFI_EVENT		  SyncEvent;
	UINT8			  finish;


	Pciebase = CosimAttr->IniArg.PlatformArg_t.devarg.cfgarg.pciebaseaddr;
	pHIFCfg = CosimAttr->pHIFPrvData_t->AsiaHIFPpi.HIFCfg;


	if(argc == 3)
	{
		CoreSource = (UINT8)atoi(argv[1]);
		CoreTarget = (UINT8)atoi(argv[2]);
	}
    if(CoreSource ==0)
    	return;


    Status = gST->BootServices->LocateProtocol (&gEfiMpServiceProtocolGuid, NULL, (VOID **)&MpServices);
	if(EFI_ERROR(Status)) {
	    Print(L"MP Service Protocol is un-available!\n");
//	    gST->ConOut->OutputString (gST->ConOut, L"MP Service Protocol is un-available!\n\r");
	    MpServices = NULL;
	} else {

	}

	Status = EFI_SUCCESS;
	if(MpServices) {
	    Status = MpServices->GetNumberOfProcessors (MpServices, &NumberOfProcessors, &NumberOfEnabledProcessors);
	    if(!EFI_ERROR(Status))
	    	Print(L"%d processors found, %d enabled!\n", NumberOfProcessors, NumberOfEnabledProcessors);
	}


	//createvent
	Status = gBS->CreateEvent (
					EVT_NOTIFY_SIGNAL,
					TPL_NOTIFY,
					SyncState,
					NULL,
					&SyncEvent
					);
	if (EFI_ERROR (Status)) {
		  return ;
	}

	if(NumberOfEnabledProcessors > 1) {
			if(MpServices) {

			//	Status = MpServices->StartupThisAP(MpServices, CpuNCycle, CoreSource, NULL, 0, (void *)&CoreTarget, &finish);
				Status = MpServices->StartupThisAP(MpServices, CpuNCycle, CoreSource, SyncEvent, 0, (void *)&CoreTarget, &finish);

				if(EFI_ERROR(Status)) {
					Print(L"Start AP fail!\n");
					NumberOfEnabledProcessors = 1;
				}

			}
	}

	bus = pHIFCfg->Die[0][0].BusNum ;
	dev = pHIFCfg->Die[0][0].CosimDevNum;
	func = pHIFCfg->Die[0][0].CosimFunNum;

	do{

	    	 Data = MemoryRead32(0x0+QemuBase);
		 //    addr = AsiaPciAddress(bus, dev, func, 0x0);
	//    	 AsiaMemoryRead32(AsiaGetPcieMmioAddress(Pciebase, addr));
		     randtime = time(NULL);   //Hif debug add
		     srand(randtime);
		 	 WaitForMicroSec(10);

	}while(finish==0 || sync_flag !=0x5a);


    Debug_Print_t((DEBUG_LOG,"  finish=%x\r\n",finish));
    printf("  Ap Core finish !!! \n\r\n");

}

*/