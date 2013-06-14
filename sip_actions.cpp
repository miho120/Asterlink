#include <stdio.h>
#include <iostream>
#include <string.h>
#include <mysql/mysql.h>
#include "configreader.h"

using namespace std;

int sip_internal_number_gen(char* client_id)
{
	char cmd[250];
	MYSQL *connection, mysql;
        MYSQL_RES *result;
        MYSQL_ROW row;
        int query_state;
	string tmp, conf_dir;
	FILE *f;
	
	// Generate patch to client sip user file
	conf_dir = parser("setting.conf","directories","asterisk_etc");        
	tmp = conf_dir + "clients/" + (const char*) client_id + "/sip_users.conf";
	
	//Open file to write
	f=fopen(tmp.c_str(), "w+t");
	// Start mysql query
	mysql_init(&mysql);
        connection = mysql_real_connect(&mysql,parser("setting.conf","database","host").c_str(),parser("setting.conf","database","user").c_str(),parser("setting.conf","database","password").c_str(),parser("setting.conf","database","base").c_str(),3306,0,0);
	if (connection == NULL) { // Test connection
        	return 1;
        }
	//Generate SQL query
	tmp = "select md5secret,number FROM user_numbers WHERE client_id = ";
	tmp = tmp + (const char*) client_id;
	mysql_query(connection, tmp.c_str());
	result = mysql_store_result(connection);
	// Generate internal number config to each number in base
	while (( row = mysql_fetch_row(result)) != NULL) {
		fprintf(f,"[%s-%s]\n",client_id,row[1]);
		fprintf(f,"type=friend\n");
		fprintf(f,"context=%s-out\n",client_id);
		fprintf(f,"md5secret=%s\n",row[0]);
		fprintf(f,"dtmfmode=RFC2833\n");
		fprintf(f,"disallow=all\n");
		fprintf(f,"allow=ulaw\n");
		fprintf(f,"allow=alaw\n");
		fprintf(f,"nat=force_rport,comedia\n");
		fprintf(f,"qualify=yes\n");
		fprintf(f,"host=dynamic\n\n");
 	}
	//After generate close file and connections
	fclose(f);
        mysql_free_result(result);
        mysql_close(connection);

	sprintf(cmd, "asterisk -rx 'sip reload'");
	if(popen(cmd, "r")==NULL)
	{
		printf("Can't run command\n");
		return 1;
	}

	return 0;
}
