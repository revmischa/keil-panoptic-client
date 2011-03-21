/*
    panoptic client
*/

#include <stdio.h>
#include <RTL.h>
#include "Net_Config.h"
#include <LM3Sxxxx.H>
#include <string.h>
#include "drivers/rit128x96x4.h"
#include "panoptic_client.h"
#include "common.h"
#include "main.h"

BOOL LEDrun;
BOOL LCDupdate;		 
BOOL tick;
U32  dhcp_tout;

extern LOCALM localm[];                       /* Local Machine Settings      */
#define MY_IP localm[NETIF_ETH].IpAdr
#define DHCP_TOUT   50                        /* DHCP timeout 5 seconds      */

U8 lcd_text[6][STATUS_MAX+1] = {"     int80",       /* Buffer for LCD text         */
                          "Panoptic Agent",
                          " ",
                          "Waiting for DHCP",
						  "\0",
						  "\0"};

static void init_io (void);
static void init_display (void);
static void net_ready(void);

 U64 tcp_stack[800/8];                        /* A bigger stack for tcp_task */

 /* Forward references */
 __task void init       (void);
 __task void blink_led  (void);
 __task void timer_task (void);
 __task void tcp_task   (void);

/*--------------------------- init ------------------------------------------*/

__task void init (void) {
  /* Add System initialisation code here */

  init_io();
  init_display();
  init_TcpNet();

  /* Initialize Tasks */
  os_tsk_prio_self (100);
  os_tsk_create (blink_led, 20);
  os_tsk_create (timer_task, 30);
  os_tsk_create_user (tcp_task, 0, &tcp_stack, sizeof(tcp_stack));

  os_tsk_delete_self();
}


/*--------------------------- timer_poll ------------------------------------*/

__task void timer_task (void) {
  /* System tick timer task */
  os_itv_set (10);
  while (1) {
    timer_tick ();
    tick = __TRUE;
    os_itv_wait ();
  }
}

/*--------------------------- init_io ---------------------------------------*/

static void init_io () {

  /* Set the clocking to run from the PLL at 50 MHz */
  SysCtlClockSet(SYSCTL_SYSDIV_4 | SYSCTL_USE_PLL |
                 SYSCTL_OSC_MAIN | SYSCTL_XTAL_8MHZ);

  /* Configure the GPIO for the LED and Push Buttons. */
  SysCtlPeripheralEnable (SYSCTL_PERIPH_GPIOE);
  SysCtlPeripheralEnable (SYSCTL_PERIPH_GPIOF);
  GPIOPadConfigSet (GPIO_PORTE_BASE, GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3,
                    GPIO_STRENGTH_2MA, GPIO_PIN_TYPE_STD_WPU);
  GPIOPadConfigSet (GPIO_PORTF_BASE, GPIO_PIN_0, GPIO_STRENGTH_2MA,
                    GPIO_PIN_TYPE_STD);
  GPIOPadConfigSet (GPIO_PORTF_BASE, GPIO_PIN_1, GPIO_STRENGTH_2MA,
                    GPIO_PIN_TYPE_STD_WPU);
  GPIODirModeSet (GPIO_PORTE_BASE, GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3,
                  GPIO_DIR_MODE_IN);
  GPIODirModeSet (GPIO_PORTF_BASE, GPIO_PIN_0, GPIO_DIR_MODE_OUT);
  GPIODirModeSet (GPIO_PORTF_BASE, GPIO_PIN_1, GPIO_DIR_MODE_IN);

  /* Configure UART0 for 115200 baud. */
  SysCtlPeripheralEnable (SYSCTL_PERIPH_GPIOA);
  SysCtlPeripheralEnable (SYSCTL_PERIPH_UART0);
  GPIOPinTypeUART (GPIO_PORTA_BASE, GPIO_PIN_0 | GPIO_PIN_1);
  UARTConfigSet(UART0_BASE, 115200, (UART_CONFIG_WLEN_8 |
                                     UART_CONFIG_STOP_ONE |
                                     UART_CONFIG_PAR_NONE));
  UARTEnable(UART0_BASE);
}



/*--------------------------- LED_out ---------------------------------------*/

void LED_out (U32 val) {
  GPIOPinWrite (GPIO_PORTF_BASE, GPIO_PIN_0, val & 1);
}


/*--------------------------- get_button ------------------------------------*/

U8 get_button (void) {
  /* Read ARM Digital Input */
  U32 val = 0;

  if (GPIOPinRead (GPIO_PORTE_BASE, GPIO_PIN_0) == 0) {
    val |= 0x01;
  }
  if (GPIOPinRead (GPIO_PORTE_BASE, GPIO_PIN_1) == 0) {
    val |= 0x02;
  }
  if (GPIOPinRead (GPIO_PORTE_BASE, GPIO_PIN_2) == 0) {
    val |= 0x04;
  }
  if (GPIOPinRead (GPIO_PORTE_BASE, GPIO_PIN_3) == 0) {
    val |= 0x08;
  }
  if (GPIOPinRead (GPIO_PORTF_BASE, GPIO_PIN_1) == 0) {
    val |= 0x10;
  }
  return (val);
}


/*--------------------------- upd_display -----------------------------------*/

static void upd_display () {
  /* Update LCD Module display text. */

  RIT128x96x4Clear ();
  RIT128x96x4StringDraw ((const char *)&lcd_text[0], 5, 5, 11);
  RIT128x96x4StringDraw ((const char *)&lcd_text[1], 5, 20, 11);
  RIT128x96x4StringDraw ((const char *)&lcd_text[2], 5, 35, 11);
  RIT128x96x4StringDraw ((const char *)&lcd_text[3], 5, 50, 11);
  RIT128x96x4StringDraw ((const char *)&lcd_text[4], 5, 65, 11);
  RIT128x96x4StringDraw ((const char *)&lcd_text[5], 5, 80, 11);
  LCDupdate =__FALSE;
}


/*--------------------------- init_display ----------------------------------*/

static void init_display () {
  /* OLED Module init */

  RIT128x96x4Init(1000000);
  upd_display ();					 
}


/*--------------------------- dhcp_check ------------------------------------*/

static void dhcp_check () {
  /* Monitor DHCP IP address assignment. */

  if (tick == __FALSE || dhcp_tout == 0) {
    return;
  }

  tick = __FALSE;

  if (mem_test (&MY_IP, 0, IP_ADRLEN) == __FALSE && !(dhcp_tout & 0x80000000)) {
    /* Success, DHCP has already got the IP address. */
    dhcp_tout = 0;
    sprintf((char *)lcd_text[2],"IP address:");
    sprintf((char *)lcd_text[3],"%d.%d.%d.%d", MY_IP[0], MY_IP[1],
                                               MY_IP[2], MY_IP[3]);
    LCDupdate = __TRUE;
	net_ready();
    return;
  }
  if (--dhcp_tout == 0) {
    /* A timeout, disable DHCP and use static IP address. */
    dhcp_disable ();
    sprintf((char *)lcd_text[3]," DHCP failed    " );
    LCDupdate = __TRUE;
    dhcp_tout = 30 | 0x80000000;
	net_ready();
    return;
  }
  if (dhcp_tout == 0x80000000) {
    dhcp_tout = 0;
    sprintf((char *)lcd_text[2],"IP address:");
    sprintf((char *)lcd_text[3],"%d.%d.%d.%d", MY_IP[0], MY_IP[1],
                                               MY_IP[2], MY_IP[3]);
    LCDupdate = __TRUE;
	net_ready();
  }
}

void status(U8 lineNum, char *status) {
	//printf("setting status=%s\n", status);
	strncpy((char *)lcd_text[lineNum], status, STATUS_MAX+1);
	update_display();
}

void update_display(void) {
	LCDupdate = __TRUE;
}

static void net_ready(void) {
  init_poc();
}

/*--------------------------- blink_led -------------------------------------*/

__task void blink_led () {
  /* Blink the LEDs on an eval board */
  U32 LEDstat = 1;

  LEDrun = __TRUE;
  while(1) {
    /* Every 100 ms */
    if (LEDrun == __TRUE) {
      LEDstat = ~LEDstat & 0x01;
      LED_out (LEDstat);
    }
    if (LCDupdate == __TRUE) {
      upd_display ();
    }
    os_dly_wait(10);
  }
}

/*---------------------------------------------------------------------------*/

__task void tcp_task (void) {
  /* Main Thread of the TcpNet. This task should have */
  /* the lowest priority because it is always READY. */
  dhcp_tout = DHCP_TOUT;
  while (1) {
    main_TcpNet();
    dhcp_check ();
    os_tsk_pass();
  }
}


int main (void) {
  os_sys_init(init);
  while(1);
}
