#ifndef __INT80_API_H__
#define __INT80_API_H__

#include "jansson/jansson.h"
#include <string.h>	// memcpy
#include <stdlib.h>	// malloc

struct int80_message {
    char *command;
	json_t *params;

	short params_copied;
	short command_copied;
	short params_ref;
};

typedef struct int80_message int80_message;

int80_message* int80_alloc_message(void);
void int80_free_message(int80_message*);

short int80_parse_message(json_t *json, int80_message *msg);

void int80_set_message_command(int80_message *msg, char *command);
void int80_set_message_command_copy(int80_message *msg, char *command);
void int80_set_message_params(int80_message *msg, json_t *);
void int80_set_message_params_copy(int80_message *msg, json_t *);
void int80_set_message_params_ref(int80_message *msg, json_t *);
void int80_message_free_params(int80_message *msg);

#endif
