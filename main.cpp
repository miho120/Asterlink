#include <stdio.h>
#include <iostream>
#include <string.h>
#include <cstring>
#include <libconfig.h>
#include "sip_actions.h"
#include "client_actions.h"

using namespace std;


int main(int argc, char **argv)
{
	int i, code;
	char host [60], client_id[40], action [20];

	// Read console arguments
	for (int i = 1; i != argc; ++i)
	{
	   	int size = 0;
		sscanf(argv[i], "--host=%s", host, &size);
        	sscanf(argv[i], "--client_id=%s", client_id, &size);
		sscanf(argv[i], "--action=%s", action, &size);
    	}

	// Run actions
	if( ! strcmp(action, "sip_user_gen") )
		code = sip_internal_number_gen(client_id);
	else if( ! strcmp(action, "client_add") )
		code = client_add(client_id);
	else if( ! strcmp(action, "client_del") )
		code = client_del(client_id);
	
	return 0;
}
