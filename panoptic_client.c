#include "panoptic_client.h"

//U8 poc_server_hostname[] = "ps.int80.biz";
U8 poc_server_hostname[] = "eastberkeley.int80";
U8 poc_server_ip[4];
U8 client_sock = 0;
BOOL have_server_ip = 0;

U64 poc_task_stack[8000/8];

// declare messagebox to hold queue of messages to parse
// holds a max of 32 messages
os_mbx_declare(raw_message_queue, POC_RAW_MESSAGE_QUEUE_MAX);
// memory pool to use for messages
U32 raw_message_queue_pool[POC_RAW_MESSAGE_QUEUE_MAX * (sizeof(raw_message_t))/4 + 3];  // I don't know what the +3 is for.

static void poc_client_main(void);						
U16 static poc_tcp_callback (U8 soc, U8 evt, U8 *ptr, U16 par);
static void poc_create_client_socket(void);
static void poc_client_tcp_connect(void);
void poc_parse_message(const char *msg_str, unsigned int msg_len);
void poc_send_string (const char *sendbuf, U16 maxlen);
void poc_send_command(const char *command, const json_t *params);
void poc_handle_command(message *msg);

void init_poc(void) {
	// create main panoptic client task
	os_tsk_create_user(poc_task, 0, &poc_task_stack, sizeof(poc_task_stack));

	// initialize mailbox to receive network messages waiting to be parsed
	// and processed
	_init_box(raw_message_queue_pool, sizeof(raw_message_queue_pool), sizeof(raw_message_t));
}

static void poc_server_dns_handler(unsigned char event, unsigned char *ip) {
	char dnsStatus[20] = "Unknown";

	switch (event) {
	    case DNS_EVT_SUCCESS:
			status(2, (char*)poc_server_hostname);
			sprintf(dnsStatus, "ip: %d.%d.%d.%d", ip[0], ip[1], ip[2], ip[3]);
			memcpy(poc_server_ip, ip, sizeof(poc_server_ip));
			have_server_ip = 1;
		    break;

		case DNS_EVT_NONAME:
			sprintf(dnsStatus, "DNS: NONAME");
		    break;

		case DNS_EVT_TIMEOUT:
			sprintf(dnsStatus, "DNS: TIMEOUT");
		    break;

		case DNS_EVT_ERROR:
			sprintf(dnsStatus, "DNS: ERROR");
		    break;
	}
	status(3, dnsStatus);
}

__task void poc_task(void) {
    // start by looking up the server IP address
	get_host_by_name(poc_server_hostname, poc_server_dns_handler);

	printf("Panoptic client started\n");

	while (1) {
		poc_client_main();
		//os_tsk_pass();
	}
}		  

static void poc_client_main(void) {
	U8 state;
	void *msg;
	raw_message_t *raw_message;

	if (! have_server_ip)
		return; 

	if (! client_sock)
	    poc_create_client_socket();

  	if (client_sock) {
		state = tcp_get_state(client_sock);
		//sprintf(lcd_text[5], "status=%i", state);

		switch(state) {
			case TCP_STATE_CLOSED:
				// recreate socket, reconnect
	  			if (tcp_release_socket(client_sock)) {
					client_sock = 0;

					poc_create_client_socket();
					if (client_sock)
						poc_client_tcp_connect();
					else
						printf("Unable to create client socket\n");
				} else {
					printf("Failed to release socket\n");
				}
				break;

			case TCP_STATE_CONNECT:
			    // we are chill
				//printf("Client socket is connected\n");
				break;

			case TCP_STATE_FINW2:
			    // still waiting to close down?
				//printf("Client socket is FINW2\n");
				break;

			default:
				//printf("Got unknown socket state: %u\n", state);
				break;
		}
  	} else {
		printf("No client socket!\n");
	}
	
	// check to see if we have any raw messages to parse and process
	if (os_mbx_wait(raw_message_queue, &msg, 0xFFFF) == OS_R_TMO) {
    	printf ("Wait message timeout!\n");
    } else {
    	// got a raw message to process
		if (msg == NULL) {
			printf("Got NULL message from queue\n");
		} else {
			raw_message = (raw_message_t *)msg; 
			poc_parse_message(raw_message->msg, raw_message->msg_len);
			free(raw_message->msg);
    		free(raw_message);
		}
  	} 
}

static void poc_client_tcp_connect(void) {
    if (tcp_connect(client_sock, poc_server_ip, POC_PORT_NUM, 0)) {
		status(4, "Connecting...");
		printf("Connecting...\n");
	} else {
		status(4, "tcp_connect() error");
		printf("Got tcl_connect() error\n");
	}
}

static void poc_create_client_socket(void) {
	client_sock = tcp_get_socket(TCP_TYPE_CLIENT, 0, POC_IDLE_TIMEOUT, poc_tcp_callback);
}

U16 static poc_tcp_callback (U8 soc, U8 evt, U8 *ptr, U16 par) {
  /* This function is called by the TCP module on TCP event */
  /* Check the 'Net_Config.h' for possible events.          */
			   
  U8 *recv_data;

  raw_message_t *raw_message;

  //printf("got event %u\n", evt);

  if (soc != client_sock) {
    return (0);
  }

  switch (evt) {
    case TCP_EVT_DATA:
      /* TCP data frame has arrived, data is located at *ptr, */
      /* data length is par. */
	  recv_data = malloc(par+1);
	  memcpy(recv_data, ptr, par);
	  recv_data[par] = '\0';
	  strncpy((char *)lcd_text[5], (char*)recv_data, STATUS_MAX+1);
	  lcd_text[5][STATUS_MAX] = '\0';
	  update_display();
	  //printf("Got data: %s\n", recv_data);

	  // add this message to the poc message queue
	  raw_message = _alloc_box(raw_message_queue_pool);
	  raw_message->msg = (char *)recv_data;
	  raw_message->msg_len = par;
	  os_mbx_send(raw_message_queue, raw_message, 0xffff);

	  //free(recv_data);    // recv_data should get freed once it's processed by the poc task
      break;

    case TCP_EVT_CONREQ:
      /* Remote peer requested connect, accept it */
      return (1);

    case TCP_EVT_CONNECT:
	  status(4, "Connected to server");
	  printf("Connected\n");
      return (1);

	case TCP_EVT_ABORT:
	  status(4, "Connection aborted");
	  //poc_connect_to_server();
      return (1);

	case TCP_EVT_CLOSE:
	  status(4, "Connection lost");
	  //poc_connect_to_server();
      return (1);
  }
  return (0);
}

void poc_parse_message(const char *msg_str, unsigned int msg_len) {
	json_t *msg_json;
	json_error_t err;
	message *msg;

	printf("got raw message len=%u, '%s'\n", msg_len, msg_str);

	// parse json into object
	msg_json = json_loads((char *)msg_str, 0, &err);
	if (! msg_json) {
		printf("Failed to parse message");
		if (err.text != NULL) {
			printf(": error: %s", err.text); 
		}
		printf("\n");

		return;
	}

	// create message object
	msg = int80_alloc_message();
	printf("msg 1 %X\n", msg);	 
	if (int80_parse_message(msg_json, msg)) {
		// got a message we can use!
		printf("parse success!!!! &command=%lX, command='%s' len=%u, json refcount=%d\n", (char*)msg->command, (char*)msg->command, strlen(msg->command), msg_json->refcount);			  
		status(5, msg->command);
		poc_handle_command(msg);
	} else {											  
		printf("Did not receive a message we understood\n");
	}		
	
	int80_free_message(msg);
	json_decref(msg_json);								
}

void poc_handle_command(message *msg) {
	if (strcmp(msg->command, "ping") == 0) {
		poc_send_command("pong", NULL);
	} else {
		printf("Got unknown command (%lX): %s\n", msg->command, msg->command);
		return;
	}
}

// TODO: make sure socket is in proper state
void poc_send_command(const char *command, const json_t *params) {
	char *msg_str;
	json_t *msg;

	ASSERT(msg);

	// construct json object
	msg = json_object();
	if (json_object_set_new(msg, "command", json_string(command)) != 0) {
		printf("error in json_object_set_new for command\n");
		json_decref(msg);
		return;
	}
	
	// dump json object to string
	msg_str = json_dumps(msg, 0);

	json_decref(msg);

	if (msg_str && strlen(msg_str)) {					
		poc_send_string(msg_str, strlen(msg_str));
		free(msg_str);
	} else {
		printf("Error serializing JSON. Command=%s\n", command); 
	}
}

void poc_send_string(const char *databuf, U16 datalen) {
	U8 *sendbuf;
	U16 maxlen = tcp_max_dsize(client_sock);

	if (datalen > maxlen) {
		// TODO: fragment message
	}

	//printf("sending message size=%u msg=%s\n\n", datalen, databuf);

    sendbuf = tcp_get_buf(datalen);
	memcpy(sendbuf, databuf, datalen);
	tcp_send(client_sock, sendbuf, datalen);
	//printf("message sent\n");
}
