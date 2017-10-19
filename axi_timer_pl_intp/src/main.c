/*
 * main.c
 * Pl_timer_intr
 *  Created on: Oct 17, 2017
 *      Author: KAB
 */
#include <stdio.h>
#include "xil_types.h"
#include "xtmrctr.h"
#include "xparameters.h"
#include "xil_io.h"
#include "xil_exception.h"
#include "xscugic.h"

#define print xil_printf

XScuGic InterruptController; /* Instance of the Interrupt Controller */
static XScuGic_Config *GicConfig;/* The configuration parameters of the controller */
extern char inbyte(void);
int counter_num = 0;

void Timer_InterruptHandler(void *data, u8 TmrCtrNumber)
{
	counter_num = counter_num+1;
	print(" Interrupt acknowledged: %d \n \r ", counter_num);
	print("\r\n");
	XTmrCtr_Stop(data,TmrCtrNumber);
	XTmrCtr_Reset(data,TmrCtrNumber);
	XTmrCtr_Start(data,TmrCtrNumber);
}

int SetUpInterruptSystem(XScuGic *XScuGicInstancePtr)
{
	/* Connect the interrupt controller interrupt handler to the hardware interrupt handling logic in the ARM processor. */
	Xil_ExceptionRegisterHandler(XIL_EXCEPTION_ID_INT,(Xil_ExceptionHandler) XScuGic_InterruptHandler, XScuGicInstancePtr);

	/* Enable interrupts in the ARM */
	Xil_ExceptionEnable();
	return XST_SUCCESS;
}

int ScuGicInterrupt_Init(u16 DeviceId,XTmrCtr *TimerInstancePtr)
{
	int Status;
	/* Initialize the interrupt controller driver so that it is ready to use. */
	GicConfig = XScuGic_LookupConfig(DeviceId);
	if (NULL == GicConfig) {
		return XST_FAILURE;
	}

	Status = XScuGic_CfgInitialize(&InterruptController, GicConfig, GicConfig->CpuBaseAddress);
	if (Status != XST_SUCCESS) {
		return XST_FAILURE;
	}

	/* Setup the Interrupt System */
	Status = SetUpInterruptSystem(&InterruptController);
	if (Status != XST_SUCCESS) {
		return XST_FAILURE;
	}

	/* Connect a device driver handler that will be called when an interrupt for the device occurs, the device driver handler performs the specific interrupt processing for the device */
	Status = XScuGic_Connect(&InterruptController, XPAR_FABRIC_AXI_TIMER_0_INTERRUPT_INTR,(Xil_ExceptionHandler)XTmrCtr_InterruptHandler, (void *)TimerInstancePtr);
	if (Status != XST_SUCCESS) {
		return XST_FAILURE;
	}

	/* Enable the interrupt for the device and then cause (simulate) an interrupt so the handlers will be called */
	XScuGic_Enable(&InterruptController, XPAR_FABRIC_AXI_TIMER_0_INTERRUPT_INTR);
	return XST_SUCCESS;
}

int main()
{
	XTmrCtr TimerInstancePtr;
	int Status;

	print("##### Application Starts #####\n\r");
	Status = XTmrCtr_Initialize(&TimerInstancePtr,XPAR_AXI_TIMER_0_DEVICE_ID);
	if(Status != XST_SUCCESS)
		print("TIMER INIT FAILED \n\r");

	/* Set Timer Handler */
	XTmrCtr_SetHandler(&TimerInstancePtr, Timer_InterruptHandler, &TimerInstancePtr);

	/*Setting timer Reset Value */
	XTmrCtr_SetResetValue(&TimerInstancePtr,0,0xf8000000); //Change with generic value

	/* Setting timer Option (Interrupt Mode And Auto Reload )*/
	XTmrCtr_SetOptions(&TimerInstancePtr, XPAR_AXI_TIMER_0_DEVICE_ID, (XTC_INT_MODE_OPTION | XTC_AUTO_RELOAD_OPTION ));

	/* SCUGIC interrupt controller Intialization, Registration of the Timer ISR*/
	Status=ScuGicInterrupt_Init(XPAR_PS7_SCUGIC_0_DEVICE_ID,&TimerInstancePtr);
	if(Status != XST_SUCCESS)
		print(" :( SCUGIC INIT FAILED \n\r");

	/* Start Timer */
	XTmrCtr_Start(&TimerInstancePtr,0);
	print("timer start \n\r");

	/* Wait For interrupt */
	print("Wait for the Timer interrupt to tigger \r\n");
	print("########################################\r\n");

	while(1)
	{
	}
	return 0;
}
