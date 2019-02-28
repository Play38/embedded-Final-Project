/********************************************************************
 FileName:     main.c
 Dependencies: See INCLUDES section
 Processor:   PIC18 or PIC24 USB Microcontrollers
 Hardware:    The code is natively intended to be used on the following
        hardware platforms: PICDEM� FS USB Demo Board,
        PIC18F87J50 FS USB Plug-In Module, or
        Explorer 16 + PIC24 USB PIM.  The firmware may be
        modified for use on other USB platforms by editing the
        HardwareProfile.h file.
 Complier:    Microchip C18 (for PIC18) or C30 (for PIC24)
 Company:   Microchip Technology, Inc.
 
 Software License Agreement:
 
 The software supplied herewith by Microchip Technology Incorporated
 (the �Company�) for its PIC� Microcontroller is intended and
 supplied to you, the Company�s customer, for use solely and
 exclusively on Microchip PIC Microcontroller products. The
 software is owned by the Company and/or its supplier, and is
 protected under applicable copyright laws. All rights are reserved.
 Any use in violation of the foregoing restrictions may subject the
 user to criminal sanctions under applicable laws, as well as to
 civil liability for the breach of the terms and conditions of this
 license.
 
 THIS SOFTWARE IS PROVIDED IN AN �AS IS� CONDITION. NO WARRANTIES,
 WHETHER EXPRESS, IMPLIED OR STATUTORY, INCLUDING, BUT NOT LIMITED
 TO, IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 PARTICULAR PURPOSE APPLY TO THIS SOFTWARE. THE COMPANY SHALL NOT,
 IN ANY CIRCUMSTANCES, BE LIABLE FOR SPECIAL, INCIDENTAL OR
 CONSEQUENTIAL DAMAGES, FOR ANY REASON WHATSOEVER.
 
********************************************************************
 File Description:
 
 Change History:
  Rev   Date         Description
  1.0   11/19/2004   Initial release
  2.1   02/26/2007   Updated for simplicity and to use common
                     coding style
********************************************************************/
 
 
//  ========================    INCLUDES    ========================
#ifdef _VISUAL
#include "VisualSpecials.h"
#endif // VISUAL
 
#include "GenericTypeDefs.h"
#include "Compiler.h"
#include "HardwareProfile.h"
 
#include "mtouch.h"
 
#include "BMA150.h"
 
#include "oled.h"
 
#include "soft_start.h"
 
 
//  ========================    CONFIGURATION   ========================
 
#if defined(PIC18F46J50_PIM)
   //Watchdog Timer Enable bit:
     #pragma config WDTEN = OFF          //WDT disabled (control is placed on SWDTEN bit)
   //PLL Prescaler Selection bits:
     #pragma config PLLDIV = 3           //Divide by 3 (12 MHz oscillator input)
   //Stack Overflow/Underflow Reset Enable bit:
     #pragma config STVREN = ON            //Reset on stack overflow/underflow enabled
   //Extended Instruction Set Enable bit:
     #pragma config XINST = OFF          //Instruction set extension and Indexed Addressing mode disabled (Legacy mode)
   //CPU System Clock Postscaler:
     #pragma config CPUDIV = OSC1        //No CPU system clock divide
   //Code Protection bit:
     #pragma config CP0 = OFF            //Program memory is not code-protected
   //Oscillator Selection bits:
     #pragma config OSC = ECPLL          //HS oscillator, PLL enabled, HSPLL used by USB
   //Secondary Clock Source T1OSCEN Enforcement:
     #pragma config T1DIG = ON           //Secondary Oscillator clock source may be selected
   //Low-Power Timer1 Oscillator Enable bit:
     #pragma config LPT1OSC = OFF        //Timer1 oscillator configured for higher power operation
   //Fail-Safe Clock Monitor Enable bit:
     #pragma config FCMEN = OFF           //Fail-Safe Clock Monitor disabled
   //Two-Speed Start-up (Internal/External Oscillator Switchover) Control bit:
     #pragma config IESO = OFF           //Two-Speed Start-up disabled
   //Watchdog Timer Postscaler Select bits:
     #pragma config WDTPS = 32768        //1:32768
   //DSWDT Reference Clock Select bit:
     #pragma config DSWDTOSC = INTOSCREF //DSWDT uses INTOSC/INTRC as reference clock
   //RTCC Reference Clock Select bit:
     #pragma config RTCOSC = T1OSCREF    //RTCC uses T1OSC/T1CKI as reference clock
   //Deep Sleep BOR Enable bit:
     #pragma config DSBOREN = OFF        //Zero-Power BOR disabled in Deep Sleep (does not affect operation in non-Deep Sleep modes)
   //Deep Sleep Watchdog Timer Enable bit:
     #pragma config DSWDTEN = OFF        //Disabled
   //Deep Sleep Watchdog Timer Postscale Select bits:
     #pragma config DSWDTPS = 8192       //1:8,192 (8.5 seconds)
   //IOLOCK One-Way Set Enable bit:
     #pragma config IOL1WAY = OFF        //The IOLOCK bit (PPSCON<0>) can be set and cleared as needed
   //MSSP address mask:
     #pragma config MSSP7B_EN = MSK7     //7 Bit address masking
   //Write Protect Program Flash Pages:
     #pragma config WPFP = PAGE_1        //Write Protect Program Flash Page 0
   //Write Protection End Page (valid when WPDIS = 0):
     #pragma config WPEND = PAGE_0       //Write/Erase protect Flash Memory pages starting at page 0 and ending with page WPFP[5:0]
   //Write/Erase Protect Last Page In User Flash bit:
     #pragma config WPCFG = OFF          //Write/Erase Protection of last page Disabled
   //Write Protect Disable bit:
     #pragma config WPDIS = OFF          //WPFP[5:0], WPEND, and WPCFG bits ignored
 
#else
    #error No hardware board defined, see "HardwareProfile.h" and __FILE__
#endif
 
 
 
//  ========================    Global VARIABLES    ========================
#pragma udata
//You can define Global Data Elements here
unsigned char RA0='0', RA1='1', RA2='2', RA3='3';
typedef struct{
	int day;
	int month;
} date;
typedef struct{
	int second;
	int minute;
	int hour;
} time;
time Time;
date Date;
int timeflag = 0;
int interval_24 = 1;
int PM;
//  ========================    PRIVATE PROTOTYPES  ========================
static void InitializeSystem(void);
static void ProcessIO(void);
static void UserInit(void);
static void YourHighPriorityISRCode();
static void YourLowPriorityISRCode();
 
BOOL CheckButtonPressed(void);
 
//  ========================    VECTOR REMAPPING    ========================
#if defined(__18CXX)
  //On PIC18 devices, addresses 0x00, 0x08, and 0x18 are used for
  //the reset, high priority interrupt, and low priority interrupt
  //vectors.  However, the current Microchip USB bootloader
  //examples are intended to occupy addresses 0x00-0x7FF or
  //0x00-0xFFF depending on which bootloader is used.  Therefore,
  //the bootloader code remaps these vectors to new locations
  //as indicated below.  This remapping is only necessary if you
  //wish to program the hex file generated from this project with
  //the USB bootloader.  If no bootloader is used, edit the
  //usb_config.h file and comment out the following defines:
  //#define PROGRAMMABLE_WITH_SD_BOOTLOADER
 
  #if defined(PROGRAMMABLE_WITH_SD_BOOTLOADER)
    #define REMAPPED_RESET_VECTOR_ADDRESS     0xA000
    #define REMAPPED_HIGH_INTERRUPT_VECTOR_ADDRESS  0xA008
    #define REMAPPED_LOW_INTERRUPT_VECTOR_ADDRESS 0xA018
  #else
    #define REMAPPED_RESET_VECTOR_ADDRESS     0x00
    #define REMAPPED_HIGH_INTERRUPT_VECTOR_ADDRESS  0x08
    #define REMAPPED_LOW_INTERRUPT_VECTOR_ADDRESS 0x18
  #endif
 
  #if defined(PROGRAMMABLE_WITH_SD_BOOTLOADER)
  extern void _startup (void);        // See c018i.c in your C18 compiler dir
  #pragma code REMAPPED_RESET_VECTOR = REMAPPED_RESET_VECTOR_ADDRESS
  void _reset (void)
  {
      _asm goto _startup _endasm
  }
  #endif
  #pragma code REMAPPED_HIGH_INTERRUPT_VECTOR = REMAPPED_HIGH_INTERRUPT_VECTOR_ADDRESS
  void Remapped_High_ISR (void)
  {
       _asm goto YourHighPriorityISRCode _endasm
  }
  #pragma code REMAPPED_LOW_INTERRUPT_VECTOR = REMAPPED_LOW_INTERRUPT_VECTOR_ADDRESS
  void Remapped_Low_ISR (void)
  {
       _asm goto YourLowPriorityISRCode _endasm
  }
 
  #if defined(PROGRAMMABLE_WITH_SD_BOOTLOADER)
  //Note: If this project is built while one of the bootloaders has
  //been defined, but then the output hex file is not programmed with
  //the bootloader, addresses 0x08 and 0x18 would end up programmed with 0xFFFF.
  //As a result, if an actual interrupt was enabled and occured, the PC would jump
  //to 0x08 (or 0x18) and would begin executing "0xFFFF" (unprogrammed space).  This
  //executes as nop instructions, but the PC would eventually reach the REMAPPED_RESET_VECTOR_ADDRESS
  //(0x1000 or 0x800, depending upon bootloader), and would execute the "goto _startup".  This
  //would effective reset the application.
 
  //To fix this situation, we should always deliberately place a
  //"goto REMAPPED_HIGH_INTERRUPT_VECTOR_ADDRESS" at address 0x08, and a
  //"goto REMAPPED_LOW_INTERRUPT_VECTOR_ADDRESS" at address 0x18.  When the output
  //hex file of this project is programmed with the bootloader, these sections do not
  //get bootloaded (as they overlap the bootloader space).  If the output hex file is not
  //programmed using the bootloader, then the below goto instructions do get programmed,
  //and the hex file still works like normal.  The below section is only required to fix this
  //scenario.
  #pragma code HIGH_INTERRUPT_VECTOR = 0x08
  void High_ISR (void)
  {
       _asm goto REMAPPED_HIGH_INTERRUPT_VECTOR_ADDRESS _endasm
  }
  #pragma code LOW_INTERRUPT_VECTOR = 0x18
  void Low_ISR (void)
  {
       _asm goto REMAPPED_LOW_INTERRUPT_VECTOR_ADDRESS _endasm
  }
  #endif  //end of "#if defined(||defined(PROGRAMMABLE_WITH_USB_MCHPUSB_BOOTLOADER))"
 
  #pragma code
 
//  ========================    Application Interrupt Service Routines  ========================
  //These are your actual interrupt handling routines.
  #pragma interrupt YourHighPriorityISRCode
  void YourHighPriorityISRCode()
  {
	if(INTCONbits.T0IF)
	{
    	Time.second++;
	
		if(Time.second == 60)
		{
			Time.second = 0;
			Time.minute++;
		}

		if(Time.minute == 60)
		{
			Time.minute = 0;
			Time.hour++;
		}
	
		if(Time.hour == 24)
		{
			Time.hour = 0;
			timeflag = 1;
		}


		if (timeflag)
		{
		
			switch(Date.month)
			{
				case 1:
				{
					if(Date.day == 31)
					{
						Date.month++;
						Date.day = 1;
					}
					else
						Date.day++;
					break;
					
				}
				case 2:
				{
					if(Date.day == 28)
					{
						Date.month++;
						Date.day = 1;
					}
					else
						Date.day++;
					break;
				}
				case 3:
				{
					if(Date.day == 31)
					{
						Date.month++;
						Date.day = 1;
					}
					else
						Date.day++;
					break;
				}
				case 4:
				{
					if(Date.day == 30)
					{
						Date.month++;
						Date.day = 1;
					}
					else
						Date.day++;
					break;
				}
				case 5:
				{
					if(Date.day == 31)
					{
						Date.month++;
						Date.day = 1;
					}
					else
						Date.day++;
					break;
				}
				case 6:
				{
					if(Date.day == 30)
					{
						Date.month++;
						Date.day = 1;
					}
					else
						Date.day++;
					break;
				}
				case 7:
				{
					if(Date.day == 31)
					{
						Date.month++;
						Date.day = 1;
					}
					else
						Date.day++;
					break;
				}
				case 8:
				{
					if(Date.day == 31)
					{
						Date.month++;
						Date.day = 1;
					}
					else
						Date.day++;
					break;
				}
				case 9:
				{
					if(Date.day == 30)
					{
						Date.month++;
						Date.day = 1;
					}
					else
						Date.day++;
					break;
				}
				case 10:
				{
					if(Date.day == 31)
					{
						Date.month++;
						Date.day = 1;
					}
					else
						Date.day++;
					break;
				}
				case 11:
				{
					if(Date.day == 30)
					{
						Date.month++;
						Date.day = 1;
					}
					else
						Date.day++;
					break;
				}
				case 12:
				{
					if(Date.day == 31)
					{
						Date.month = 1;
						Date.day = 1;
					}
					else
						Date.day++;
					break;
				}
			}
			timeflag = 0;
		}
	INTCONbits.TMR0IF = 0 ;
	}
 
  } //This return will be a "retfie fast", since this is in a #pragma interrupt section
  #pragma interruptlow YourLowPriorityISRCode
  void YourLowPriorityISRCode()
  {
    //Check which interrupt flag caused the interrupt.
    //Service the interrupt
    //Clear the interrupt flag
    //Etc.
 
  } //This return will be a "retfie", since this is in a #pragma interruptlow section
#endif
 
 
 
 
//  ========================    Board Initialization Code   ========================
#pragma code
#define ROM_STRING rom unsigned char*
 
/******************************************************************************
 * Function:        void UserInit(void)
 *
 * PreCondition:    None
 *
 * Input:           None
 *
 * Output:          None
 *
 * Side Effects:    None
 *
 * Overview:        This routine should take care of all of the application code
 *                  initialization that is required.
 *
 * Note:            
 *
 *****************************************************************************/
void UserInit(void)
{
  /* Initialize the mTouch library */
  mTouchInit();
 
  /* Call the mTouch callibration function */
  mTouchCalibrate();
 
  /* Initialize the accelerometer */
  InitBma150();
 
  /* Initialize the oLED Display */
   ResetDevice();  
   FillDisplay(0x00);
 
}//end UserInit
 
 
/********************************************************************
 * Function:        static void InitializeSystem(void)
 *
 * PreCondition:    None
 *
 * Input:           None
 *
 * Output:          None
 *
 * Side Effects:    None
 *
 * Overview:        InitializeSystem is a centralize initialization
 *                  routine. All required USB initialization routines
 *                  are called from here.
 *
 *                  User application initialization routine should
 *                  also be called from here.                  
 *
 * Note:            None
 *******************************************************************/
static void InitializeSystem(void)
{
    // Soft Start the APP_VDD
   while(!AppPowerReady())
        ;
 
    #if defined(PIC18F87J50_PIM) || defined(PIC18F46J50_PIM)
  //On the PIC18F87J50 Family of USB microcontrollers, the PLL will not power up and be enabled
  //by default, even if a PLL enabled oscillator configuration is selected (such as HS+PLL).
  //This allows the device to power up at a lower initial operating frequency, which can be
  //advantageous when powered from a source which is not gauranteed to be adequate for 48MHz
  //operation.  On these devices, user firmware needs to manually set the OSCTUNE<PLLEN> bit to
  //power up the PLL.
    {
        unsigned int pll_startup_counter = 600;
        OSCTUNEbits.PLLEN = 1;  //Enable the PLL and wait 2+ms until the PLL locks before enabling USB module
        while(pll_startup_counter--);
    }
    //Device switches over automatically to PLL output after PLL is locked and ready.
    #endif
 
    #if defined(PIC18F46J50_PIM)
  //Configure all I/O pins to use digital input buffers
    ANCON0 = 0xFF;                  // Default all pins to digital
    ANCON1 = 0xFF;                  // Default all pins to digital
    #endif
   
    UserInit();
 
}//end InitializeSystem
 
 
 
//  ========================    Application Code    ========================
 
 
/********************************************************************
 * Function:        void main(void)
 *
 * PreCondition:    None
 *
 * Input:           None
 *
 * Output:          None
 *
 * Side Effects:    None
 *
 * Overview:        Main program entry point.
 *
 * Note:            None
 *******************************************************************/
static char toprint[24];
void mainTraverse(int c);

BOOL CheckButtonPressed(void)
{
   if(PORTBbits.RB0 == 0) 
		return TRUE;
	else 
		return FALSE;
}
 
int GetA2D(){   //Set ADCON and start input
    BYTE x;
    int a2d;
    x=2;
    ADCON0 = 0b00010011;
    while(x){
    x=x&ADCON0;
    }
    a2d=(int)ADRESH;
    a2d= a2d<<8;          
    a2d+=(int)ADRESL;
    return a2d;
}
 
int CheckLRVolt(unsigned int x){
    if(x>600)
        return 0;
    else return 1;
}
 
int CheckUDVolt(unsigned int x,unsigned int y){
    if(x<y && y-x>75 && x < 900){
        return 1;       //Up
	}
    else if (y<x && x-y>75 && y < 800){ 
		return 2;}       //Down
   else return 0;   //Not pushed
}
 
 
int GetAccVal(char c){  //Check the accelerometer's readings
	/*
	x: msb 3 lsb 2
	y: msb 5 lsb 4
	z: msb 7 lsb 6
	*/
	BYTE msb,lsb;
	BYTE mask= 0b10000000;
	int signextend= 0xFC00;//2^15+2^14+2^13+2^12+2^11+2^10+2^9
	int val=0,n1,n2;
    if(c=='x'){n1=3;n2=2;}
    if(c=='y'){n1=5;n2=4;}
    if(c=='z'){n1=7;n2=6;}
    msb=BMA150_ReadByte(n1);
    lsb=BMA150_ReadByte(n2);
    lsb=lsb>>6;
    val+=(int)msb;
    val=val<<2;
    val+=(int)lsb;
    mask= mask&msb;
    if(mask== 0b10000000){
        val|=signextend;
    }
    return val;
}
 

void clearScreen(){
	int i;
	for(i=1;i<8;i++){
		oledPutROMString("                    ", i, 0); 
	}
}
void clearScreen0(){
	int i;
	for(i=0;i<8;i++){
		oledPutROMString("                                 ", i, 0); 
	}
}
void menuClock(time t)
{
		int timeprint;

		sprintf(timeprint, "%02d", t.second);
		oledPutString(timeprint, 0 ,2*58,1);
		sprintf(timeprint, "%02d", t.minute);
		oledPutString(timeprint, 0 ,2*48,1);

		if(interval_24)
		{
			sprintf(timeprint, "%02d", t.hour);
			oledPutString(timeprint, 0 ,2*38,1); 
		}
		else
		{
			if(Time.hour>=13 && t.hour <=23)
				sprintf(timeprint, "%02d", (t.hour % 2));
			oledPutString(timeprint, 0 ,2*38,1); 

		}
				sprintf(toprint,":");
				oledPutString(toprint, 0 ,2*55,1);
				oledPutString(toprint, 0 ,2*45,1);	

}
void IntervalMenu() //potenciometer
{
    int i , z, pot;
	int currChoice=1;
	clearScreen();
	while(1)
	{
    	sprintf(toprint,"Select interval");
    	oledPutString(toprint, 0, 0,1);  
 	
		

		pot = GetA2D();
		if(pot < 511)
			currChoice=1;
		else if(pot > 511 && pot < 1023)
			currChoice=2;

		sprintf(toprint, "24h mode");
		if(currChoice == 1)oledPutString(toprint, 1 ,2*6,0);
	   		else oledPutString(toprint, 1 ,2*6,1);
		sprintf(toprint, "12h mode");
		if(currChoice == 2)oledPutString(toprint, 2 ,2*6,0);
	   		else oledPutString(toprint, 2 ,2*6,1);

		if( CheckLRVolt(mTouchReadButton(RA3)) ) // L to return to main menu
		{
		clearScreen0();
		return 0;
		}
		if( CheckLRVolt(mTouchReadButton(RA0)) ) // R to choose
			{
				if(currChoice == 1)
					interval_24 = 1;
				else if( currChoice == 2)
					interval_24 = 0;
			}
		
 		DelayMs(60);
	}
}
void digClock(time t)
{
		int timeprint;

		sprintf(timeprint, "%2d", t.second);
		oledPutString(timeprint, 4 ,2*40,1);
		sprintf(timeprint, "%2d", t.minute);
		oledPutString(timeprint, 4 ,2*30,1);

		if(interval_24)
		{
			sprintf(timeprint, "%2d", t.hour);
			oledPutString(timeprint, 4 ,2*20,1); 
		}
		else
		{
			if(Time.hour>=13 && t.hour <=23)
				sprintf(timeprint, "%2d", (t.hour % 2));
			oledPutString(timeprint, 4 ,2*20,1); 
			
			if(Time.hour>=0 && t.hour <=11)
			{
				sprintf(toprint,"AM");
				oledPutString(toprint, 5 ,2*6,1);
			}
			else
			{
				sprintf(toprint,"PM");
				oledPutString(toprint, 5 ,2*6,1);
			}
		}	
 /*
		sprintf(timeprint, "%2d", Date.month);
		oledPutString(timeprint, 2 ,2*30,1);
		sprintf(timeprint, "%2d", Date.day);
		oledPutString(timeprint, 2 ,2*20,1);  */
}
void setClock()
{
	time timetemp;
    int i , z;
	int currChoice=1;
	int c =1;
	timetemp.hour = Time.hour;
	timetemp.minute = Time.minute;
	timetemp.second = Time.second;
	clearScreen();
	DelayMs(60);
	while(1)
	{
    	sprintf(toprint,"Set clock");
    	oledPutString(toprint, 0, 0,1);  
   		menuClock(Time);
 		digClock(timetemp);		
		switch(c)
		{
			case 1:
			{
				if( CheckUDVolt(mTouchReadButton(RA1),mTouchReadButton(RA2))==1 &&  timetemp.hour < 24)    //Pressed up           
					timetemp.hour++;	

    			if( CheckUDVolt(mTouchReadButton(RA1),mTouchReadButton(RA2))==2 && timetemp.hour > 0) //Pressed down
 					timetemp.hour--;
				break;
			}
			case 2:
			{
				if( CheckUDVolt(mTouchReadButton(RA1),mTouchReadButton(RA2))==1 &&  timetemp.minute < 59)    //Pressed up           
					timetemp.minute++;	

    			if( CheckUDVolt(mTouchReadButton(RA1),mTouchReadButton(RA2))==2 && timetemp.minute > 0) //Pressed down
 					timetemp.minute--;	
				break;
			}

			case 3:
			{
				if( CheckUDVolt(mTouchReadButton(RA1),mTouchReadButton(RA2))==1 &&  timetemp.second < 59)    //Pressed up           
					timetemp.second++;	

    			if( CheckUDVolt(mTouchReadButton(RA1),mTouchReadButton(RA2))==2 && timetemp.second > 0) //Pressed down
 					timetemp.second--;

				break;
			}
		}



		
		if( CheckLRVolt(mTouchReadButton(RA3)) ) // L to return to main menu
			c--;
		
		if( CheckLRVolt(mTouchReadButton(RA0)) ) // R to choose
			c++;

		if(c ==4)
		{
			Time.hour = timetemp.hour;
			Time.minute = timetemp.minute;
			Time.second = timetemp.second;
			clearScreen0();
			return 0;
		}
 		DelayMs(60);
	}
}
void setTraverse(int c){
	switch(c){
		//case 1: subMenu1();break;
		case 2: IntervalMenu();break;
		case 3: setClock();break;
		//case 4: subMenu4();break;
		//case 5: subMenu5();break;
		default: break;
	}
}
void setMenu() //potenciometer
{
    int i , z, pot;
	int currChoice=1;
	clearScreen();
	while(1)
	{
    	sprintf(toprint,"Settings");
    	oledPutString(toprint, 0, 0,1);  
 		menuClock(Time);
		

		pot = GetA2D();
		if(pot < 204)
			currChoice=1;
		else if(pot > 204 && pot < 409)
			currChoice=2;
		else if(pot > 409 && pot < 613)
			currChoice=3;
		else if(pot > 613 && pot < 818)
			currChoice=4;
		else if(pot > 818 && pot < 1023)
			currChoice=5;

		sprintf(toprint, "Display Mode");
		if(currChoice == 1)oledPutString(toprint, 1 ,2*6,0);
	   		else oledPutString(toprint, 1 ,2*6,1);
		sprintf(toprint, "12/24H Interval");
		if(currChoice == 2)oledPutString(toprint, 2 ,2*6,0);
	   		else oledPutString(toprint, 2 ,2*6,1);
		sprintf(toprint, "Set Time");
		if(currChoice == 3)oledPutString(toprint, 3 ,2*6,0);
	   		else oledPutString(toprint, 3 ,2*6,1);
		sprintf(toprint, "Set Date");
		if(currChoice == 4)oledPutString(toprint, 4 ,2*6,0);
	   		else oledPutString(toprint, 4 ,2*6,1);
		sprintf(toprint, "Alarm");
		if(currChoice == 5)oledPutString(toprint, 5 ,2*6,0);
	   		else oledPutString(toprint, 5 ,2*6,1);

		if( CheckLRVolt(mTouchReadButton(RA3)) ) // L to return to main menu
		{
		clearScreen0();
		return 0;
		}
		if( CheckLRVolt(mTouchReadButton(RA0)) ) // R to choose
			setTraverse(currChoice);
		
 		DelayMs(60);
	}
}

void clockScreen()
{  
    int i, up, down;	
	int currChoice=1;
	clearScreen0();
	while(1)
	{
		digClock(Time);
		if(CheckButtonPressed())
			DelayMs(200);
			if(CheckButtonPressed())
				setMenu();

	}
}

void main(void)
{
    InitializeSystem();

	Date.day=1;
	Date.month=1;
	Time.second=0;
	Time.minute=0;
	Time.hour=0;

/*	T0CONbits.T08BIT = 0 ;			//Timer0 16BIT COUNTER
	T0CONbits.T0CS = 0 ;			//Clock Source -- Internal
	T0CONbits.PSA = 0 ;				//Use Pre-Scaler
	T0CONbits.T0PS = 7 ;			//Prescale 1:256
	T0CONbits.TMR0ON = 1 ;			//Set Timer to ON

	RCONbits.IPEN = 1 ;				//Use Priority Interrutps
	INTCON2bits.T0IP = 1 ;			//Timer0 High-Priority

	INTCONbits.GIE = 1 ;			//Enable Interrupts
	INTCONbits.PEIE = 1 ;
	INTCONbits.T0IE = 1 ;			//Timer0 Overflow Interrupt Enable
*/

	
	T0CON = 0x07 ;
	// Initialize Timer Interrupt
	RCONbits.IPEN = 1 ;			//Prio Enable
	INTCON2bits.TMR0IP = 1 ;	//Use Hi-Prio
	INTCON = 0xE0 ;				//Enable Timer Interrupt

	T0CON |= 0x80;				//Start the Timer
	
	clockScreen();
    //mainMenu();
	
    while(1)                            //Main is Usualy an Endless Loop
    {}      
}//end main
 
 
/** EOF main.c *************************************************/