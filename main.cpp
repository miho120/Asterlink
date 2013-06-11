#include <stdio.h>
#include <iostream>
#include <string.h>
#include <cstring>
#include <unistd.h>
#include <stdlib.h>
#include <libconfig.h>
#include "sip_actions.h"
#include "configreader.h"

using namespace std;

int main(int argc, char **argv)
{
	int i, code;
	char host [60];
	char user [40];
        char action [20];
	const char *lol;
	for (int i = 1; i != argc; ++i)
	{
	   	int size = 0;
		sscanf(argv[i], "--host=%s", host, &size);
        	sscanf(argv[i], "--user=%s", user, &size);
		sscanf(argv[i], "--action=%s", action, &size);
    	}

	if( ! strcmp(action, "sip_user_gen") )
	{
		code = sip_user_gen(user);
	}
	lol = parser("setting.conf","database","user");
	cout << lol << endl;
	
	return 0;
}
