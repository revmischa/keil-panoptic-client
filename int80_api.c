#include "int80_api.h"

short int80_parse_message(json_t *json, int80_message *msg) {
	json_t *command, *params;

	if (! msg) return 0;

	if (! json_is_object(json)) return 0;

	printf("json=%X\n", json);
	command = json_object_get(json, "command");
	if (command == NULL || ! json_is_string(command))
		return 0;

	printf("command=%X\n", command);

	params = json_object_get(json, "params");
	printf("params=%X\n", params);
	
	// cool

	int80_set_message_command_copy(msg, json_string_value(command));
                          
	if (params) {
		printf("1: params refcnt: %d\n", params->refcount);
		int80_set_message_params_ref(msg, params);
		printf("2: params refcnt: %d\n", params->refcount);
    }

	return 1;	
}						

int80_message* int80_alloc_message(void) {
	int80_message *msg = malloc(sizeof(struct int80_message));
    if (! msg) {
        printf("failed to alloc msg!\n");
        return NULL;
    }

    printf("alloc msg=%X size=%u\n", msg, sizeof(struct int80_message));
													 
	msg->command = NULL;
	msg->params  = NULL;
	msg->params_copied  = 0;
	msg->command_copied = 0;
	msg->params_ref     = 0;

	return msg;	
}

void int80_free_message(int80_message *msg) {
	printf("msg=%X, params copy: %d, params ref: %d, params: %X\n",
		msg, msg->params_copied, msg->params_ref, msg->params);

	if (msg->command_copied && msg->command)
		free(msg->command);

    printf("freeing params\n");
	int80_message_free_params(msg);

    printf("freeing msg=%X\n", msg);
	free(msg);
}
		
void int80_set_message_command(int80_message *msg, char *command) {
	if (msg->command_copied && msg->command)
		free(msg->command);

	msg->command = command;
	msg->command_copied = 0;
} 
		
void int80_set_message_command_copy(int80_message *msg, const char *command) {
	//if (msg->command == command) return;

	if (msg->command_copied && msg->command)
		free(msg->command);
	
	msg->command = strdup(command);
	printf("copied %X to %X, s1=%s, s2=%s\n", command, msg->command, command, msg->command);

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
	msg->params_ref = 1;

	msg->params = params;	
}

void int80_message_free_params(int80_message *msg) {
	if (msg->params_copied && msg->params)
		free(msg->params);
	else if (msg->params_ref && msg->params)
		json_decref(msg->params);

	msg->params_copied = 0;
	msg->params_ref    = 0;
    msg->params        = NULL;
}
