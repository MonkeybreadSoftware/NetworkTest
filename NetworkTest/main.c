//
//  main.c
//  NetworkTest
//
// Trying out connection with Network.framework on macOS 10.15 or newer synchronously in command line tool
//
//  Created by Christian Schmitz on 07.04.24.
//

#include <stdio.h>
#include <Network/Network.h>

void ConnectSync(nw_connection_t connection)
{
	__block bool done = false;
	
	nw_connection_set_state_changed_handler(connection, ^(nw_connection_state_t state, nw_error_t _Nullable error) {
		printf("%s: %d\n", __FUNCTION__, state);
		if (error)
		{
			printf("error code: %d in domain: %d\n", nw_error_get_error_code(error), nw_error_get_error_domain(error));
			abort();
		}
		
		if (state == nw_connection_state_preparing)
		{
			// ignore
		}
		else if (state == nw_connection_state_ready)
		{
			done = true;
		}
	});
	
	// Start the connection
	dispatch_queue_t queue = dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0);
	nw_connection_set_queue(connection, queue);
	nw_connection_start(connection);
	
	while (!done)
	{
		sleep(1);
	}
}

void SendSync(nw_connection_t connection, const char* text)
{
	__block bool done = false;
	
	dispatch_queue_t queue = NULL;
	dispatch_data_t data = dispatch_data_create(text, strlen(text), queue, ^{
		printf("%s\n", __FUNCTION__);
	});
	
	nw_content_context_t context = nw_content_context_create("test");
	nw_connection_send(connection, data, context, true, ^(nw_error_t  error) {
		printf("%s\n", __FUNCTION__);
		if (error)
		{
			printf("error code: %d in domain: %d\n", nw_error_get_error_code(error), nw_error_get_error_domain(error));
			abort();
		}
		
		done = true;
	});
	
	while (!done)
	{
		sleep(1);
	}
}

void CloseSync(nw_connection_t connection)
{
	__block bool done = false;
	
	nw_connection_set_state_changed_handler(connection, ^(nw_connection_state_t state, nw_error_t _Nullable error) {
		printf("%s: %d\n", __FUNCTION__, state);
		if (error)
		{
			printf("error code: %d in domain: %d\n", nw_error_get_error_code(error), nw_error_get_error_domain(error));
		}
		
		if (state == nw_connection_state_cancelled)
		{
			done = true;
		}
	});
	
	nw_connection_cancel(connection);
	
	while (!done)
	{
		sleep(1);
	}
}

void ReceiveSync(nw_connection_t connection)
{
	uint32_t minimum_incomplete_length = 1;
	uint32_t maximum_length = 1024 * 1024;
	__block bool done = false;
	nw_connection_receive(connection, minimum_incomplete_length, maximum_length, ^(dispatch_data_t  _Nullable content, nw_content_context_t  _Nullable context, bool is_complete, nw_error_t  _Nullable error) {
		printf("%s: %d\n", __FUNCTION__, is_complete);
		if (error)
		{
			printf("error code: %d in domain: %d\n", nw_error_get_error_code(error), nw_error_get_error_domain(error));
		}
		
		size_t size = dispatch_data_get_size(content);
		printf("size: %lu\n", size);
		char* buffer = calloc(1, size);
		if (buffer)
		{
			__block char *buf = buffer;
			__block size_t sizeRead = 0;
			dispatch_data_apply(content, ^bool(dispatch_data_t  _Nonnull region, size_t offset, const void * _Nonnull buffer, size_t size) {
				memcpy(buf, buffer, size);
				buf += size;
				sizeRead += size;
				return true;
			});

			printf("size read: %lu\n", sizeRead);
			
			printf("%s\n", buffer);
			free(buffer);
		}
		
		done = true;
	});
	
	while (!done)
	{
		sleep(1);
	}
}

int main(int argc, const char * argv[]) {
	// insert code here...
	printf("Hello, World!\n");
	
	// Create an endpoint for the website
	nw_endpoint_t endpoint = nw_endpoint_create_host("curl.se", "https");
	
	// Create a connection to the endpoint
	nw_parameters_t param = nw_parameters_create_secure_tcp(NW_PARAMETERS_DEFAULT_CONFIGURATION, NW_PARAMETERS_DEFAULT_CONFIGURATION);
	nw_connection_t connection = nw_connection_create(endpoint, param);
	
	printf("connect...\n");
	ConnectSync(connection);
	printf("connected.\n");
	
	
	SendSync(connection, "GET / HTTP/1.1\r\nHost: curl.se\r\n\r\n");
	printf("sent.\n");
	
	ReceiveSync(connection);
	
	CloseSync(connection);
	
	return 0;
}
