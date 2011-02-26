#include <stdio.h>
#include <LM3Sxxxx.H>
						
// definition of fputc for debugging over virtual COM port11
int fputc (int ch, FILE *f)  {
  /* Debug output to serial port. */

  if (ch == '\n')  {
    UARTCharPut (UART0_BASE, '\r');          /* output CR                    */
  }
  UARTCharPut (UART0_BASE, ch);
  return (ch);
}

