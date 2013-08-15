#include <stdio.h>
#include <iostream>
#include <string.h>
#include <mysql/mysql.h>
//#include "configreader.h"

using namespace std;

int queues_gen(char* client_id)
{
	char cmd[250];
	MYSQL *connection, mysql;
        MYSQL_RES *result, *result1;
        MYSQL_ROW row, row1;
        int query_state;
	string tmp, query, query1, conf_dir;
	FILE *f;
	
	cout << "Generate queues config for client \"" << client_id <<"\"... ";
	
	// Generate patch to client sip user file
	conf_dir = parser("setting.conf","directories","asterisk_etc");        
	tmp = conf_dir + "clients/" + (const char*) client_id + "/queues.conf";
	
	//Open file to write
	f=fopen(tmp.c_str(), "w+t");
	// Start mysql query
	mysql_init(&mysql);
        connection = mysql_real_connect(&mysql,parser("setting.conf","database","host").c_str(),parser("setting.conf","database","user").c_str(),parser("setting.conf","database","password").c_str(),parser("setting.conf","database","base").c_str(),3306,0,0);
	if (connection == NULL) { // Test connection
        	return 1;
        }

	//Generate SQL query
	query = "SELECT name, strategy, id FROM queues WHERE client_id = ";
	query+= (const char*) client_id;
	mysql_query(connection, query.c_str());
	result = mysql_store_result(connection);
	// Generate internal number config to each number in base
	while (( row = mysql_fetch_row(result)) != NULL) {
		fprintf(f,"\n[%s-%s]\n",client_id,row[0]);
		fprintf(f,"strategy = %s\n",row[1]);

		//Generate members
		query1 = "SELECT member FROM queues_members WHERE queue_id = " + string(row[2]);
		mysql_query(connection, query1.c_str());
		result1 = mysql_store_result(connection);
		// Generate internal number config to each number in base
		while (( row1 = mysql_fetch_row(result1)) != NULL)
			fprintf(f,"member => SIP/%s-%s\n",client_id,row1[0]);

        	mysql_free_result(result1);

 	}
	//After generate close file and connections
	fclose(f);
        mysql_free_result(result);
        mysql_close(connection);

	//sprintf(cmd, "asterisk -rx 'sip reload'");
	if(popen(cmd, "r")==NULL)
	{
		printf("Can't run command\n");
		return 1;
	}

	cout << "Done!" << endl;
	return 0;
}
