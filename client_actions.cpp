#include <stdio.h>
#include <iostream>
#include <string.h>
#include <fstream>

using namespace std;

int client_add(char* client_id)
{
	char cmd[250];
	string path = parser("setting.conf","directories","asterisk_etc") + "clients/" + (const char*) client_id;

	if(ifstream(path.c_str()).good() == true)
	{
		cout<<"Client exists! Exiting..."<<endl;
		return 1;
	}
	sprintf(cmd, "mkdir %s", path.c_str());
	if(popen(cmd, "r")==NULL)
	{
		printf("Can't run command\n");
		return 1;
	}
	return 0;
}

int client_del(char* client_id)
{
	char cmd[250];
	string path = parser("setting.conf","directories","asterisk_etc") + "clients/" + (const char*) client_id;

	if(ifstream(path.c_str()).good() == false)
	{
		cout<<"Client not exists! Exiting..."<<endl;
		return 1;
	}
	sprintf(cmd, "rm -Rf %s", path.c_str());
	if(popen(cmd, "r")==NULL)
	{
		printf("Can't run command\n");
		return 1;
	}
	return 0;
}
