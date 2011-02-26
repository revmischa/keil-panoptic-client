#include "int80_api.h"

short int80_parse_message(json_t *json, int80_message *msg) {
	json_t *command, *params;

	if (! msg) return 0;

	command = json_object_get(json, "command");
	if (command == NULL)
		return 0;
		
	// TODO: validate json commands		

	params = json_object_get(json, "params");
	
	// cool
	// TODO: use json val directly, incrementing ref count? save a little ram
	int80_set_message_command_copy(msg, (char *)json_string_value(command));
	int80_set_message_params_ref(msg, params);

	return 1;	
}

int80_message* int80_alloc_message(void) {
	int80_message *msg = malloc(sizeof(int80_message));
													 
	msg->command = NULL;
	msg->params = NULL;
	msg->params_copied = 0;
	msg->command_copied = 0;
	msg->params_ref    = 0;

	return msg;	
}

void int80_free_message(int80_message *msg) {
	if (msg->command_copied && msg->command)
		free(msg->command);

	if (msg->params_copied && msg->params)
		free(msg->params);

	free(msg);
}
		
void int80_set_message_command(int80_message *msg, char *command) {
	if (msg->command_copied && msg->command)
		free(msg->command);

	msg->command = command;
	msg->command_copied = 0;
} 
		
void int80_set_message_command_copy(int80_message *msg, char *command) {
	if (msg->command == command) return;

	if (msg->command_copied && msg->command)
		free(msg->command);
	
	msg->command = malloc(strlen(command) + 1);
	memcpy(msg->command, command, strlen(command) + 1);

	msg->command_copied = 1;
}

void int80_set_message_params(int80_message *msg, json_t *params) {
	int80_message_free_params(msg);

	msg->params = params;
	msg->params_copied = 0;
} 

void int80_set_message_params_copy(int80_message *msg, json_t *params) {
	int80_message_free_params(msg);

	msg->params = json_deep_copy(params);
	msg->params_copied = 1;
}

void int80_set_message_params_ref(int80_message *msg, json_t *params) {
    int80_message_free_params(msg);

	json_incref(params);
	msg->params = params;

	msg->params_ref = 1;
}

void int80_message_free_params(int80_message *msg) {
	if (msg->params_copied && msg->params)
		free(msg->params);
	else if (msg->params_ref && msg->params)
		json_decref(msg->params);

	msg->params_copied = 0;
	msg->params_ref    = 0;
}
