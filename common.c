#include "common.h"
						
// definition of fputc for debugging over virtual COM port11
int fputc (int ch, FILE *f)  {
  /* Debug output to serial port. */

  if (ch == '\n')  {
    UARTCharPut (UART0_BASE, '\r');          /* output CR */
  }
  UARTCharPut (UART0_BASE, ch);
  return (ch);
}

void __aeabi_assert(const char *expr, const char *file, int line) {
	// failed assertion
	printf("\n\n - assertion failed: %s\n   - file: %s\n   - line: %u\n\n", expr, file, line);
	abort();
}	 

void abort(void) {
	printf("abort() called\n");
}

char *strdup (const char *s) {
    char *d = (char *)(malloc (strlen (s) + 1)); // Allocate memory
    if (d != NULL)
        strcpy (d,s);                            // Copy string if okay
    return d;                                    // Return new memory
}
