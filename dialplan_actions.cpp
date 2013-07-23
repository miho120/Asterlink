#include <stdio.h>
#include <iostream>
#include <string.h>
#include <mysql/mysql.h>
//#include "configreader.h"

using namespace std;

int dialplan_incoming_gen(char* client_id)
{
	char cmd[250];
	MYSQL *connection, mysql;
        MYSQL_RES *result;
        MYSQL_ROW row;
        int query_state;
	string ext_file, conf_dir, query;
	FILE *f;

	// Generate patch to client dialplan file
	conf_dir = parser("setting.conf","directories","asterisk_etc");        
	ext_file = conf_dir + "clients/" + (const char*) client_id + "/extensions.conf";

	// Start mysql query
	mysql_init(&mysql);
        connection = mysql_real_connect(&mysql,parser("setting.conf","database","host").c_str(),parser("setting.conf","database","user").c_str(),parser("setting.conf","database","password").c_str(),parser("setting.conf","database","base").c_str(),3306,0,0);
	if (connection == NULL) { // Test connection
		cout << "Error connect to database! Exiting..." << endl;
        	return 1;
        }
	//Open file to write
	f=fopen(ext_file.c_str(), "w+t");

//------Generate [client-incoming] context
	cout << "Generate [client-incoming] context for client \"" << client_id <<"\"... ";
	//Generate SQL query
	query = "SELECT value FROM dialplan_config WHERE route_type = 'in' AND type = 'main_time' AND client_id = ";
	query+= (const char*) client_id;
	query+= " ORDER BY priority ASC;";
	mysql_query(connection, query.c_str());
	result = mysql_store_result(connection);
	fprintf(f,"[%s-incoming]\n",client_id);
	fprintf(f,"exten => _X.,1,NoOp(Received incoming call...)\n");
	// Generate incoming route
	while (( row = mysql_fetch_row(result)) != NULL) {
		fprintf(f,"exten => _X.,n,GotoIfTime(%s?%s-incoming-day,${EXTEN},1)\n",row[0],client_id);
 	}
	fprintf(f,"exten => _X.,n,Goto(%s-incoming-night,${EXTEN},1)\n\n",client_id);
	cout << "Done!" << endl;
	mysql_free_result(result);

//------Generate [client-ivr] context
	cout << "Generate [client-ivr] context for client \"" << client_id <<"\"... ";
	//Generate SQL query
	query = "SELECT value FROM dialplan_config WHERE route_type = 'in' AND type = 'ivr_main' AND client_id = ";
	query+= (const char*) client_id;
	query+= " ORDER BY priority ASC;";
	mysql_query(connection, query.c_str());
	result = mysql_store_result(connection);
	fprintf(f,"[%s-ivr]\n",client_id);
	fprintf(f,"exten => s,1,NoOp(Starting IVR...)\n");
	// Generate s IVR
	while (( row = mysql_fetch_row(result)) != NULL) {
		fprintf(f,"exten => s,n,%s\n",row[0]);
 	}
	fprintf(f,"exten => s,n,WaitExten()\n\n");
	fprintf(f,"exten => _.,1,NoOp()\n");
	mysql_free_result(result);

	//Generate items for IVR
	query = "SELECT value,value2 FROM dialplan_config WHERE route_type = 'in' AND type = 'ivr_item' AND client_id = ";
	query+= (const char*) client_id;
	query+= " ORDER BY value2, priority ASC;";
	mysql_query(connection, query.c_str());
	result = mysql_store_result(connection);
	// Generate item IVR
	while (( row = mysql_fetch_row(result)) != NULL) {
		fprintf(f,"exten => %s,n,%s\n",row[1],row[0]);
 	}
	mysql_free_result(result);
	cout << "Done!" << endl;

//------Generate [client-incoming-day] context
	cout << "Generate [client-incoming-day] context for client \"" << client_id <<"\"... ";
	fprintf(f,"\n[%s-incoming-day]\n",client_id);
	fprintf(f,"exten => _X.,1,NoOp(Received incoming call...)\n");

	//Play greeting?
	query = "SELECT value2 FROM dialplan_config WHERE route_type = 'in' AND type = 'incoming-day' AND value = 'greeting_play' AND client_id = ";
	query+= (const char*) client_id;
	mysql_query(connection, query.c_str());
	result = mysql_store_result(connection);
	while (( row = mysql_fetch_row(result)) != NULL)
		if ( string(row[0]) != "0" )
			fprintf(f,"exten => _X.,n,Playback(%s)\n",row[0]);
	mysql_free_result(result);

	fprintf(f,"exten => _X.,n,Set(CDR(clientfield)=%s)\n",client_id);

	//Record call?
	query = "SELECT value2 FROM dialplan_config WHERE route_type = 'in' AND type = 'incoming-day' AND value = 'record_call' AND client_id = ";
	query+= (const char*) client_id;
	mysql_query(connection, query.c_str());
	result = mysql_store_result(connection);
	while (( row = mysql_fetch_row(result)) != NULL)
		if ( string(row[0]) == "1" ){
			fprintf(f,"exten => _X.,n,Set(fname=${STRFTIME(${EPOCH},,%%Y%%m%%d%%H%%M)}-%s-${CALLERID(num)}-${MACRO_EXTEN})\n",client_id);
			fprintf(f,"exten => _X.,n,MixMonitor(/var/spool/asterisk/monitor/${fname}.wav)\n",client_id);
		}
	mysql_free_result(result);

	//Ringing to operators
	query = "SELECT value,value2 FROM dialplan_config WHERE route_type = 'in' AND type = 'ring_group' AND client_id = ";
	query+= (const char*) client_id;
	query+= " ORDER BY value2, priority ASC;";
	mysql_query(connection, query.c_str());
	result = mysql_store_result(connection);
	// Generate item IVR
	while (( row = mysql_fetch_row(result)) != NULL) {
		fprintf(f,"exten => _%s,n,%s\n",row[1],row[0]);
		fprintf(f,"exten => _%s,n,GotoIf($[\"${DIALSTATUS}\" = \"ANSWER\"]?hang:)\n",row[1]);
 	}
	mysql_free_result(result);

	//Leave the voicemail?
	query = "SELECT value FROM dialplan_config WHERE route_type = 'in' AND type = 'vm' AND client_id = ";
	query+= (const char*) client_id;
	mysql_query(connection, query.c_str());
	result = mysql_store_result(connection);
	while (( row = mysql_fetch_row(result)) != NULL)
		if ( string(row[0]) != "0" )
			fprintf(f,"exten => _X.,n,Goto(%s-vm,${EXTEN},1)\n",client_id);
	mysql_free_result(result);

	fprintf(f,"exten => _X.,n(hang),Hangup\n");
	cout << "Done!" << endl;

//------Generate [client-vm] context
	cout << "Generate [client-vm] context for client \"" << client_id <<"\"... ";
	fprintf(f,"\n[%s-vm]\n",client_id);
	fprintf(f,"exten => _X.,1,NoOp(Received vm...)\n");

	//Play greeting?
	query = "SELECT value, value2 FROM dialplan_config WHERE route_type = 'in' AND type = 'vm' AND client_id = ";
	query+= (const char*) client_id;
	mysql_query(connection, query.c_str());
	result = mysql_store_result(connection);
	while (( row = mysql_fetch_row(result)) != NULL)
		if ( string(row[0]) != "0" ){
			fprintf(f,"exten => _X.,n,PlayBack(noanswer)\n",row[1]);
			fprintf(f,"exten => _X.,n,Voicemail(%s@%s,s)\n",row[0],client_id);
		}
	fprintf(f,"exten => _X.,n,Hangup\n");
	mysql_free_result(result);

	cout << "Done!" << endl;

	//After generate close file and connections
	fclose(f);
        mysql_close(connection);	
	return 0;
}
