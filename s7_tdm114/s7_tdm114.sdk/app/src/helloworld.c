#include <stdio.h>
#include "platform.h"
#include "xiic.h"
#include "xintc.h"
#include "xil_exception.h"
#include "camera_initial.h"
#include "xvtc.h"
#include "xv_demosaic.h"




#define IIC_dev 			XPAR_IIC_0_DEVICE_ID
#define int_dev 			XPAR_INTC_0_DEVICE_ID
#define IIC_SLAVE_ADDR		0x48
#define PCA9534_IIC_ADDR    0x20
#define INTC_DEVICE_INT_ID	XPAR_INTC_0_IIC_0_VEC_ID
#define BUFFER_SIZE		6

XIic  iic;
XIntc InterruptController;
XV_demosaic mosaic;

volatile u8 TransmitComplete;
volatile u8 ReceiveComplete;

int PCA9534_CTRL ();
int SetUpInterruptSystem();
void SendHandler(XIic *InstancePtr);
void ReceiveHandler(XIic *InstancePtr);
void StatusHandler(XIic *InstancePtr, int Event);
int Detect_Camera();
int Soft_Reset_Camera ();
int Initial_Camera();
int Initial_setting_2 (u32 *mt9m114_config , int mt9m114_config_QTY  );
int Initial_setting_1 (u32 *mt9m114_config , int mt9m114_config_QTY  );

int main()
{
	u32  Status;
	XIic_Config *iic_conf;
    XVtc	VtcInst;
    XVtc_Config *vtc_config;
    XVtc_Timing vtcTiming;
    XVtc_SourceSelect SourceSelect;
    XV_demosaic_Config *mosaic_config;


 	init_platform();

    printf("www.adiuvoengineering.com S7 Imager example\n\r");

    mosaic_config = XV_demosaic_LookupConfig(XPAR_XV_DEMOSAIC_0_DEVICE_ID);
    XV_demosaic_CfgInitialize(&mosaic,mosaic_config,mosaic_config->BaseAddress);

    XIntc_Initialize(&InterruptController, int_dev);
	SetUpInterruptSystem();
	iic_conf = XIic_LookupConfig(IIC_dev);
	Status =  XIic_CfgInitialize(&iic, iic_conf, iic_conf->BaseAddress);
	if (Status != XST_SUCCESS) {
		 printf("XIic initial is fail \n \r") ;
		return XST_FAILURE;
	}

	XIic_SetSendHandler(&iic, &iic,	(XIic_Handler) SendHandler);
	XIic_SetRecvHandler(&iic, &iic, (XIic_Handler) ReceiveHandler);
	XIic_SetStatusHandler(&iic, &iic,(XIic_StatusHandler) StatusHandler);
    vtc_config = XVtc_LookupConfig(XPAR_VTC_0_DEVICE_ID);
	XVtc_CfgInitialize(&VtcInst, vtc_config, vtc_config->BaseAddress);

	vtcTiming.HActiveVideo = 1280;
	vtcTiming.HFrontPorch = 65;
	vtcTiming.HSyncWidth = 55;
	vtcTiming.HBackPorch = 40;
	vtcTiming.HSyncPolarity = 0;
	vtcTiming.VActiveVideo = 800;
	vtcTiming.V0FrontPorch = 7;//8;
	vtcTiming.V0SyncWidth = 4;
	vtcTiming.V0BackPorch = 12;
	vtcTiming.V1FrontPorch = 7;
	vtcTiming.V1SyncWidth = 4;
	vtcTiming.V1BackPorch = 12;
	vtcTiming.VSyncPolarity = 0;
	vtcTiming.Interlaced = 0;

	memset((void *)&SourceSelect, 0, sizeof(SourceSelect));
	SourceSelect.VBlankPolSrc = 1;
	SourceSelect.VSyncPolSrc = 1;
	SourceSelect.HBlankPolSrc = 1;
	SourceSelect.HSyncPolSrc = 1;
	SourceSelect.ActiveVideoPolSrc = 1;
	SourceSelect.ActiveChromaPolSrc= 1;
	SourceSelect.VChromaSrc = 1;
	SourceSelect.VActiveSrc = 1;
	SourceSelect.VBackPorchSrc = 1;
	SourceSelect.VSyncSrc = 1;
	SourceSelect.VFrontPorchSrc = 1;
	SourceSelect.VTotalSrc = 1;
	SourceSelect.HActiveSrc = 1;
	SourceSelect.HBackPorchSrc = 1;
	SourceSelect.HSyncSrc = 1;
	SourceSelect.HFrontPorchSrc = 1;
	SourceSelect.HTotalSrc = 1;

	XVtc_RegUpdateEnable(&VtcInst);
	XVtc_SetGeneratorTiming(&VtcInst,&vtcTiming);
	XVtc_SetSource(&VtcInst, &SourceSelect);
	XVtc_EnableGenerator(&VtcInst);

    XIic_Reset(&iic);

    PCA9534_CTRL ();
    Detect_Camera();
    Soft_Reset_Camera();
    Initial_Camera();
    XV_demosaic_Set_HwReg_width(&mosaic,0x500);
    XV_demosaic_Set_HwReg_height(&mosaic,0x31f);
    XV_demosaic_Set_HwReg_bayer_phase(&mosaic,0x1);
    XV_demosaic_EnableAutoRestart(&mosaic);
    XV_demosaic_Start(&mosaic);


    while(1){

    }
    cleanup_platform();
    return 0;
}

int PCA9534_CTRL ()
{
  s32  Status ;
  u8 SendBuffer [2];
  TransmitComplete = 1;
  SendBuffer[0]= 0x03;
  SendBuffer[1]= 0x00;

  XIic_SetAddress(&iic,XII_ADDR_TO_SEND_TYPE,PCA9534_IIC_ADDR);
  XIic_Reset(&iic);
  XIic_Start(&iic);

  XIic_MasterSend(&iic, SendBuffer, 2);

  while ((TransmitComplete) || (XIic_IsIicBusy(&iic) == TRUE)) {

  }

  SendBuffer[0]= 0x01;
  SendBuffer[1]= 0x02;
  TransmitComplete = 1;
  XIic_MasterSend(&iic, SendBuffer, 2);
  while ((TransmitComplete) || (XIic_IsIicBusy(&iic) == TRUE)) {

  }


  usleep(500000);
  SendBuffer[0]= 0x01;
  SendBuffer[1]= 0x01;
  TransmitComplete = 1;
  XIic_MasterSend(&iic, SendBuffer, 2);
  while ((TransmitComplete) || (XIic_IsIicBusy(&iic) == TRUE)) {

  }

  printf("light LED successfully \n\r");
  usleep(1000);
  XIic_Stop(&iic);

 return XST_SUCCESS;
}

int Detect_Camera()
{
	s32  Status ;
    u8 SendBuffer [2];
    u8 RecvBuffer [2];
    u8 Options;

    TransmitComplete = 1;
    ReceiveComplete = 1;

    XIic_SetAddress(&iic,XII_ADDR_TO_SEND_TYPE,IIC_SLAVE_ADDR );
    XIic_SetOptions(&iic, XII_REPEATED_START_OPTION);
    //XIic_Reset(&iic);
    XIic_Start(&iic);

	SendBuffer[0]= 0;
	SendBuffer[1]= 0;

    Status = XIic_MasterSend(&iic, SendBuffer,2);
	if (Status != XST_SUCCESS) {
			return XST_FAILURE;
		}
   while ((TransmitComplete)) {

   }

    Status = XIic_MasterRecv(&iic, RecvBuffer,  2);

	if (Status != XST_SUCCESS) {
			return XST_FAILURE;
		  }
	while ((ReceiveComplete)) {

	}

	Options = XIic_GetOptions(&iic);
	XIic_SetOptions(&iic, Options &= ~XII_REPEATED_START_OPTION);

	if( RecvBuffer[0]==0x24 && RecvBuffer[1]==0x81) {
		print("detect MT9M114 camera successfully \n\r");
	  }
	else
	  {
		print("can't detect MT9M114 camera \n\r");
		return XST_FAILURE;
	  }
   XIic_Stop(&iic);
   return XST_SUCCESS;
}

int Soft_Reset_Camera ()
{
  s32  Status ;
  u8 SendBuffer [4];
  SendBuffer[0]= 0x00;
  SendBuffer[1]= 0x1a;
  SendBuffer[2]= 0x00;
  SendBuffer[3]= 0x01;
  XIic_Reset(&iic);
  XIic_SetAddress(&iic,XII_ADDR_TO_SEND_TYPE,IIC_SLAVE_ADDR );
  XIic_Start(&iic);
  TransmitComplete = 1;
  Status = XIic_MasterSend(&iic, SendBuffer, 4);
  if (Status != XST_SUCCESS) {
	return XST_FAILURE;
  }

  while ((TransmitComplete)) {
  }

  usleep(500000);
  SendBuffer[0]= 0x00;
  SendBuffer[1]= 0x1a;
  SendBuffer[2]= 0x00;
  SendBuffer[3]= 0x00;

  Status = XIic_MasterSend(&iic, SendBuffer, 4);
  if (Status != XST_SUCCESS) {
   return XST_FAILURE;
  }

  while ((TransmitComplete)) {
  }

  usleep(1000);
  XIic_Stop(&iic);
  return XST_SUCCESS;
}

int SetUpInterruptSystem()
{
	int Status;

	Status = XIntc_Connect(&InterruptController, INTC_DEVICE_INT_ID,
				   (XInterruptHandler)XIic_InterruptHandler,
				   &iic);
	if (Status != XST_SUCCESS) {
		return XST_FAILURE;
	}

	Status = XIntc_Start(&InterruptController, XIN_REAL_MODE);
	if (Status != XST_SUCCESS) {
		return XST_FAILURE;
	}

	XIntc_Enable(&InterruptController, INTC_DEVICE_INT_ID);

	Xil_ExceptionInit();

	Xil_ExceptionRegisterHandler(XIL_EXCEPTION_ID_INT,
				(Xil_ExceptionHandler)XIntc_InterruptHandler,
				&InterruptController);

	Xil_ExceptionEnable();

	return XST_SUCCESS;

}

 void StatusHandler(XIic *InstancePtr, int Event)
{

}

 void ReceiveHandler(XIic *InstancePtr)
{
	ReceiveComplete = 0;
	//printf("here rx \n\r");
}

 void SendHandler(XIic *InstancePtr)
{
	TransmitComplete = 0;
	//printf("here tx \n\r");
}

 int Initial_Camera( )
 {
 	s32  Status ;


 	Status =Initial_setting_1 ( mt9m114_config_1, mt9m114_config_QTY_1 );

 	  if (Status != XST_SUCCESS) {
 	 		  return XST_FAILURE;
 	      	   }

     Status =Initial_setting_1 ( mt9m114_config_2, mt9m114_config_QTY_2 );

 	  if (Status != XST_SUCCESS) {
 	 		  return XST_FAILURE;
 	      	   }
     Status =Initial_setting_1 ( mt9m114_config_3, mt9m114_config_QTY_3 );

 	  if (Status != XST_SUCCESS) {
 	 		  return XST_FAILURE;
 	      	   }
 	Status = Initial_setting_2 ( mt9m114_config_4 , mt9m114_config_QTY_4 );

 	  if (Status != XST_SUCCESS) {
 			   return XST_FAILURE;
 		      }

 	Status = Initial_setting_2 ( mt9m114_config_5 , mt9m114_config_QTY_5 );

 	  if (Status != XST_SUCCESS) {
 			   return XST_FAILURE;
 			   }

 	Status = Initial_setting_2 ( mt9m114_config_6 , mt9m114_config_QTY_6 );
 	  if (Status != XST_SUCCESS) {
 			   return XST_FAILURE;
 			   }
 	Status = Initial_setting_2 ( mt9m114_config_7 , mt9m114_config_QTY_7 );
 	  if (Status != XST_SUCCESS) {
 			   return XST_FAILURE;
 			   }
 	Status = Initial_setting_2 ( mt9m114_config_8 , mt9m114_config_QTY_8 );
 	  if (Status != XST_SUCCESS) {
 			   return XST_FAILURE;
 			   }
     Status = Initial_setting_2 ( mt9m114_config_9 , mt9m114_config_QTY_9);
 	  if (Status != XST_SUCCESS) {
 			   return XST_FAILURE;
 			   }
 	Status = Initial_setting_2 ( mt9m114_config_10 , mt9m114_config_QTY_10 );
 	  if (Status != XST_SUCCESS) {
 			   return XST_FAILURE;
 			   }
 	Status =Initial_setting_1 ( mt9m114_config_11, mt9m114_config_QTY_11 );
 	  if (Status != XST_SUCCESS) {
 			   return XST_FAILURE;
 			   }
     Status =Initial_setting_1 ( mt9m114_config_12, mt9m114_config_QTY_12 );
 	  if (Status != XST_SUCCESS) {
 			   return XST_FAILURE;
 			   }

     Status = Initial_setting_2 ( mt9m114_config_13 , mt9m114_config_QTY_13 );
 	  if (Status != XST_SUCCESS) {
 			   return XST_FAILURE;
 			   }
     Status = Initial_setting_2 ( mt9m114_config_14 , mt9m114_config_QTY_14 );
 	  if (Status != XST_SUCCESS) {
 			   return XST_FAILURE;
 			   }
     Status = Initial_setting_2 ( mt9m114_config_15 , mt9m114_config_QTY_15 );
 	  if (Status != XST_SUCCESS) {
 			   return XST_FAILURE;
 			   }
     Status = Initial_setting_2 ( mt9m114_config_16 , mt9m114_config_QTY_16 );
 	  if (Status != XST_SUCCESS) {
 			   return XST_FAILURE;
 			   }
     Status = Initial_setting_2 ( mt9m114_config_17 , mt9m114_config_QTY_17 );
 	  if (Status != XST_SUCCESS) {
 			   return XST_FAILURE;
 			   }
     Status = Initial_setting_2 ( mt9m114_config_18 , mt9m114_config_QTY_18 );
 	  if (Status != XST_SUCCESS) {
 			   return XST_FAILURE;
 			   }
     Status = Initial_setting_2 ( mt9m114_config_19 , mt9m114_config_QTY_19 );
 	  if (Status != XST_SUCCESS) {
 			   return XST_FAILURE;
 			   }
     Status = Initial_setting_2 ( mt9m114_config_20 , mt9m114_config_QTY_20 );
 	  if (Status != XST_SUCCESS) {
 			   return XST_FAILURE;
 			   }
     Status = Initial_setting_2 ( mt9m114_config_21 , mt9m114_config_QTY_21 );
 	  if (Status != XST_SUCCESS) {
 			   return XST_FAILURE;
 			   }
     Status = Initial_setting_2 ( mt9m114_config_22 , mt9m114_config_QTY_22 );
 	  if (Status != XST_SUCCESS) {
 			   return XST_FAILURE;
 			   }
     Status = Initial_setting_2 ( mt9m114_config_23 , mt9m114_config_QTY_23 );
 	  if (Status != XST_SUCCESS) {
 			   return XST_FAILURE;
 			   }
     Status = Initial_setting_2 ( mt9m114_config_24 , mt9m114_config_QTY_24 );
 	  if (Status != XST_SUCCESS) {
 			   return XST_FAILURE;
 			   }
     Status = Initial_setting_2 ( mt9m114_config_25 , mt9m114_config_QTY_25 );
 	  if (Status != XST_SUCCESS) {
 			   return XST_FAILURE;
 			   }
     Status = Initial_setting_2 ( mt9m114_config_26 , mt9m114_config_QTY_26 );
 	  if (Status != XST_SUCCESS) {
 			   return XST_FAILURE;
 			   }
     Status = Initial_setting_2 ( mt9m114_config_27 , mt9m114_config_QTY_27 );
 	  if (Status != XST_SUCCESS) {
 			   return XST_FAILURE;
 			   }
     Status = Initial_setting_2 ( mt9m114_config_28 , mt9m114_config_QTY_28 );
 	  if (Status != XST_SUCCESS) {
 			   return XST_FAILURE;
 			   }
     Status = Initial_setting_2 ( mt9m114_config_29 , mt9m114_config_QTY_29 );
 	  if (Status != XST_SUCCESS) {
 			   return XST_FAILURE;
 			   }
     Status = Initial_setting_2 ( mt9m114_config_30 , mt9m114_config_QTY_30 );
 	  if (Status != XST_SUCCESS) {
 			   return XST_FAILURE;
 			   }
     Status = Initial_setting_2 ( mt9m114_config_31 , mt9m114_config_QTY_31 );
 	  if (Status != XST_SUCCESS) {
 			   return XST_FAILURE;
 			   }
     Status = Initial_setting_2 ( mt9m114_config_32 , mt9m114_config_QTY_32 );
 	  if (Status != XST_SUCCESS) {
 			   return XST_FAILURE;
 			   }
     Status = Initial_setting_2 ( mt9m114_config_33 , mt9m114_config_QTY_33 );
 	  if (Status != XST_SUCCESS) {
 			   return XST_FAILURE;
 			   }
     Status = Initial_setting_2 ( mt9m114_config_34 , mt9m114_config_QTY_34 );
 	  if (Status != XST_SUCCESS) {
 			   return XST_FAILURE;
 			   }

     Status =Initial_setting_1 ( mt9m114_config_35, mt9m114_config_QTY_35 );
 	  if (Status != XST_SUCCESS) {
 			   return XST_FAILURE;
 			   }
     Status =Initial_setting_1 ( mt9m114_config_36, mt9m114_config_QTY_36 );
 	  if (Status != XST_SUCCESS) {
 			   return XST_FAILURE;
 			   }
     Status =Initial_setting_1 ( mt9m114_config_37, mt9m114_config_QTY_37 );
 	  if (Status != XST_SUCCESS) {
 			   return XST_FAILURE;
 			   }

 	  print("initial MT9M114 camera successfully \n\r");
  return XST_SUCCESS;
 }

 int  Initial_setting_1 ( u32 *mt9m114_config , int mt9m114_config_QTY  )
 {
 	s32  Status , byte_count;

     int i ;

     u8 SendBuffer[BUFFER_SIZE];


     for(i=0;i<mt9m114_config_QTY;i++){

 	     byte_count   = *(mt9m114_config + i*3 + 2) - 1;

         if(byte_count==4){
  	     SendBuffer[1]=  *(mt9m114_config + i*3);
          SendBuffer[0]=  (*(mt9m114_config + i*3))>>8;
 		 SendBuffer[3]=  *(mt9m114_config + i*3 + 1);
 		 SendBuffer[2]=  (*(mt9m114_config + i*3 + 1))>>8;
           }
         else
             if(byte_count==3){
             	SendBuffer[1]=  *(mt9m114_config + i*3);
             	SendBuffer[0]=  (*(mt9m114_config + i*3))>>8;
             	SendBuffer[2]=  *(mt9m114_config + i*3 + 1);
               }
             else
                 if(byte_count==6){

                  SendBuffer[1]=  *(mt9m114_config + i*3);
                  SendBuffer[0]=  (*(mt9m114_config + i*3))>>8;
            		 SendBuffer[5]=  *(mt9m114_config + i*3 + 1);
            		 SendBuffer[4]=  (*(mt9m114_config + i*3 + 1))>>8;
            		 SendBuffer[3]=  (*(mt9m114_config + i*3 + 1))>>16;
            		 SendBuffer[2]=  (*(mt9m114_config + i*3 + 1))>>24;
                   }

    TransmitComplete = 1;
 	Status = XIic_MasterSend(&iic, SendBuffer, byte_count);
	if (Status != XST_SUCCESS) {
	  return XST_FAILURE;
	}

    while ((TransmitComplete)) {
    }
    usleep(1000);

    }


 return XST_SUCCESS;

 }


 int Initial_setting_2 ( u32 *mt9m114_config , int mt9m114_config_QTY  )
 {
 	s32  Status , byte_count;

     int i ;

     u8 SendBuffer[BUFFER_SIZE];

     u16 reg_addr;

        reg_addr=  *mt9m114_config;

     for(i=1;i<mt9m114_config_QTY;i++){

 	     byte_count   = *(mt9m114_config + i*2 + 1) - 1;

         if(byte_count==4){
  	     SendBuffer[1]=   reg_addr;
          SendBuffer[0]=   reg_addr>>8;
 		 SendBuffer[3]=  *(mt9m114_config + i*2 );
 		 SendBuffer[2]=  (*(mt9m114_config + i*2 ))>>8;
 		 reg_addr=  reg_addr + 0x2 ;
           }
         else
             if(byte_count==3){
             	SendBuffer[1]=  reg_addr;
             	SendBuffer[0]=  reg_addr>>8;
             	SendBuffer[2]=  *(mt9m114_config + i*2 );

             	 reg_addr=  reg_addr + 0x1 ;
               }

         else
             if(byte_count==6){
      	     SendBuffer[1]=   reg_addr;
              SendBuffer[0]=   reg_addr>>8;
     		 SendBuffer[5]=  *(mt9m114_config + i*2 );
     		 SendBuffer[4]=  (*(mt9m114_config + i*2 ))>>8;
     		 SendBuffer[3]=  (*(mt9m114_config + i*2 ))>>16;
     		 SendBuffer[2]=  (*(mt9m114_config + i*2 ))>>24;

     		 reg_addr=  reg_addr + 0x4 ;
               }



    TransmitComplete =1;
 	Status = XIic_MasterSend(&iic, SendBuffer, byte_count);
  	     if (Status != XST_SUCCESS) {
  		  return XST_FAILURE;
       	}
  	   while ((TransmitComplete)) {
  	    }

  	    usleep(1000);

    }

 return XST_SUCCESS;

 }
