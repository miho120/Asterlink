#include <stdio.h>
#include <iostream>
#include <string.h>
#include <cstring>
#include <libconfig.h>
#include "sip_actions.h"

using namespace std;


int main(int argc, char **argv)
{
	int i, code;
	char host [60], user[40], action [20];

	// Read console arguments
	for (int i = 1; i != argc; ++i)
	{
	   	int size = 0;
		sscanf(argv[i], "--host=%s", host, &size);
        	sscanf(argv[i], "--user=%s", user, &size);
		sscanf(argv[i], "--action=%s", action, &size);
    	}

	// Run actions
	if( ! strcmp(action, "sip_user_gen") )
	{
		code = sip_user_gen(user);
	}

	return 0;
}
