#ifndef __MAIN_H__
#define __MAIN_H__

#define STATUS_MAX 19

extern U8 lcd_text[6][STATUS_MAX+1];

void status(U8 lineNum, char *status);
void update_display(void);

#endif
