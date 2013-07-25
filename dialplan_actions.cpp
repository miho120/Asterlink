#include <stdio.h>
#include <iostream>
#include <string.h>
#include <mysql/mysql.h>
//#include "configreader.h"

using namespace std;

int dialplan_incoming(char* client_id, MYSQL *connection, FILE *f);
int dialplan_ivr(char* client_id, MYSQL *connection, FILE *f);
int dialplan_incoming_day(char* client_id, MYSQL *connection, FILE *f);
int dialplan_vm(char* client_id, MYSQL *connection, FILE *f);
int dialplan_internal(char* client_id, MYSQL *connection, FILE *f);
int dialplan_external(char* client_id, MYSQL *connection, FILE *f);

int dialplan_gen(char* client_id){
	MYSQL *connection, mysql;
        int query_state;
	string ext_file, conf_dir;
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

	dialplan_incoming(client_id,connection,f);
	dialplan_ivr(client_id,connection,f);
	dialplan_incoming_day(client_id,connection,f);
	dialplan_vm(client_id,connection,f);
	dialplan_internal(client_id,connection,f);
	dialplan_external(client_id,connection,f);

	//After generate close file and connections
	fclose(f);
        mysql_close(connection);	
	return 0;
}

int dialplan_incoming(char* client_id, MYSQL *connection, FILE *f){
	string query;
	MYSQL_RES *result;
        MYSQL_ROW row;
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
}

int dialplan_ivr(char* client_id, MYSQL *connection, FILE *f){
	string query;
	MYSQL_RES *result;
        MYSQL_ROW row;
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
}

int dialplan_incoming_day(char* client_id, MYSQL *connection, FILE *f){
	string query;
	MYSQL_RES *result;
        MYSQL_ROW row;
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
	query = "SELECT value FROM dialplan_config WHERE route_type = 'in' AND type = 'record_call' AND client_id = ";
	query+= (const char*) client_id;
	mysql_query(connection, query.c_str());
	result = mysql_store_result(connection);
	while (( row = mysql_fetch_row(result)) != NULL)
		if ( string(row[0]) == "1" ){
			fprintf(f,"exten => _X.,n,Set(fname=${STRFTIME(${EPOCH},,%%Y%%m%%d%%H%%M)}-%s-${CALLERID(num)}-${EXTEN})\n",client_id);
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
}

int dialplan_vm(char* client_id, MYSQL *connection, FILE *f){
	string query;
	MYSQL_RES *result;
    MYSQL_ROW row;
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
}

int dialplan_internal(char* client_id, MYSQL *connection, FILE *f){
	string query;
	MYSQL_RES *result;
    MYSQL_ROW row;
//------Generate [client-internal] context
	cout << "Generate [client-internal] context for client \"" << client_id <<"\"... ";
	fprintf(f,"\n[%s-internal]\n",client_id);
	fprintf(f,"include => %s-pickup\n",client_id);
	fprintf(f,"include => %s-external\n",client_id);
	
	//Generate macro to call user
	query = "SELECT number, vm, fm FROM user_numbers WHERE client_id = ";
	query+= (const char*) client_id;
	mysql_query(connection, query.c_str());
	result = mysql_store_result(connection);
	while (( row = mysql_fetch_row(result)) != NULL)
			fprintf(f,"exten => %s,1,Macro(%s-internal,%s-%s,60,rTt,%s,20,%s)\n",row[0],client_id,client_id,row[0],row[2],row[1]);
	mysql_free_result(result);

	fprintf(f,"\n[macro-%s-internal]\n",client_id);
	fprintf(f,"exten => s,1,Set(SIPFROMDOMAIN=sip.asterlink.com.ua)\n");
	fprintf(f,"same => n,Dial(SIP/${ARG1}@%s-kamailio,${ARG2},${ARG3})\n",client_id);
	fprintf(f,"same => n,NoOp(DIALSTATUS=${DIALSTATUS}, ARG4=${ARG4}, ARG6=${ARG6})\n");
	fprintf(f,"same => n,GotoIf($[$[\"${DIALSTATUS}\" != \"ANSWER\"] & $[\"${ARG4}\" != \"nofm\"]]?FM)\n");
	fprintf(f,"same => n,GotoIf($[$[\"${DIALSTATUS}\" != \"ANSWER\"] & $[\"${ARG6}\" != \"novm\"]]?VM)\n");
	fprintf(f,"same => n,Hangup\n");
	fprintf(f,"same => n(FM),Playback(ru/followme/status)\n");
	fprintf(f,"same => n,Macro(%s-outgoing,${ARG4},${ARG5},tr)\n",client_id);
	fprintf(f,"same => n,GotoIf($[\"${ARG6}\" != \"novm\"]?VM)");
	fprintf(f,"same => n,Hangup\n");
	fprintf(f,"same => n(VM),VoiceMail(${ARG1}@%s)\n",client_id);
	fprintf(f,"same => n,Hangup\n");

	cout << "Done!" << endl;
}

int dialplan_external(char* client_id, MYSQL *connection, FILE *f){
	string query;
	MYSQL_RES *result;
	MYSQL_ROW row;
//------Generate [client-external] context
	cout << "Generate [client-external] context for client \"" << client_id <<"\"... ";
	fprintf(f,"\n[%s-external]\n",client_id);
	fprintf(f,"exten => _X.,1,NoOp(Calling to ${EXTEN}...)\n");

	//Record call?
	query = "SELECT value FROM dialplan_config WHERE route_type = 'out' AND type = 'record_call' AND client_id = ";
	query+= (const char*) client_id;
	mysql_query(connection, query.c_str());
	result = mysql_store_result(connection);
	while (( row = mysql_fetch_row(result)) != NULL)
		if ( string(row[0]) == "1" ){
			fprintf(f,"exten => _X.,n,Set(fname=${STRFTIME(${EPOCH},,%%Y%%m%%d%%H%%M)}-%s-${CALLERID(num)}-${EXTEN})\n",client_id);
			fprintf(f,"exten => _X.,n,MixMonitor(/var/spool/asterisk/monitor/${fname}.wav)\n",client_id);
		}
	mysql_free_result(result);
	
	//Generate macro to call user
	query = "SELECT route_out.prefix, route_out.template, trunks.number FROM route_out LEFT JOIN trunks ON route_out.trunk = trunks.id WHERE route_out.client_id = ";
	query+= (const char*) client_id;
	query+= " ORDER BY route_out.template, route_out.priority ASC;";
	mysql_query(connection, query.c_str());
	result = mysql_store_result(connection);
	while (( row = mysql_fetch_row(result)) != NULL)
			fprintf(f,"exten => _%s,n,Macro(%s-external,${EXTEN:%s},60,rTt,%s-%s)\n",row[1],client_id,row[0],client_id,row[2]);
	mysql_free_result(result);
	fprintf(f,"exten => _X.,n,Hangup\n");

	fprintf(f,"\n[macro-%s-external]\n",client_id);
	fprintf(f,"exten => s,1,NoOp(Calling to ${ARG1}...)\n");
	fprintf(f,"same => n,Dial(SIP/${ARG1}@${ARG4},${ARG2},${ARG3})\n");


	cout << "Done!" << endl;
}
