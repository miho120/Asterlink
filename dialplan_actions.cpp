#include <stdio.h>
#include <iostream>
#include <string.h>
#include <mysql/mysql.h>
#include <sstream>
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
	fprintf(f,"exten => _X.,n,Set(CDR(clientfield)=%s)\n",client_id);
	fprintf(f,"exten => _X.,n,Set(blacklist=${ODBC_BLACKLIST(4,${CALLERID(num)})})\n");
	fprintf(f,"exten => _X.,n,GotoIf($[\"${blacklist}\" != \"0\"]?black)\n");
	fprintf(f,"exten => _X.,n,Set(CALLERID(name)=${ODBC_PHONEBOOK(4,${CALLERID(num)})})\n");
	fprintf(f,"exten => _X.,n,ExecIf($[\"${CALLERID(name)}\" = \"\"]?Set(CALLERID(name)=${CALLERID(num)}))\n");
	// Generate incoming route
	while (( row = mysql_fetch_row(result)) != NULL) {
		fprintf(f,"exten => _X.,n,GotoIfTime(%s?%s-incoming-day,${EXTEN},1)\n",row[0],client_id);
 	}
	fprintf(f,"exten => _X.,n,Goto(%s-incoming-night,${EXTEN},1)\n",client_id);
	fprintf(f,"exten => _X.,n(black),Noop(Number ${CALLERID(num)} in blacklist!!!)\n",client_id);
	fprintf(f,"exten => _X.,n,Hangup\n\n",client_id);
	cout << "Done!" << endl;
	mysql_free_result(result);
}

int dialplan_ivr(char* client_id, MYSQL *connection, FILE *f){
	string query, query_sub;
	MYSQL_RES *result, *result_sub;
        MYSQL_ROW row, row_sub;
	//------Generate [client-ivr] context
	cout << "Generate [client-ivr] context for client \"" << client_id <<"\"... ";

	//Calculate count of sub ivr
	query = "SELECT count(*) FROM ivr WHERE client_id = ";
	query+= (const char*) client_id;
	query+= " GROUP BY name ORDER BY name ASC;";
	mysql_query(connection, query.c_str());
	result = mysql_store_result(connection);
	row = mysql_fetch_row(result);
	string input(row[0]);
	stringstream SS(input);
	int sub_ivr_count;
	SS >> sub_ivr_count;

	//Generate sub IVR
	query = "SELECT name FROM ivr WHERE client_id = ";
	query+= (const char*) client_id;
	query+= " GROUP BY name ORDER BY name ASC;";
	mysql_query(connection, query.c_str());
	result = mysql_store_result(connection);
	while (( row = mysql_fetch_row(result)) != NULL) {
		fprintf(f,"\n[%s-ivr-%s]\n",client_id,row[0]);
		query_sub = "SELECT item, priority, app FROM ivr WHERE name = '" + string(row[0]) + "' AND client_id = ";
		query_sub+= (const char*) client_id;
		query_sub+= " ORDER BY name, item, priority ASC;";
		mysql_query(connection, query_sub.c_str());
		result_sub = mysql_store_result(connection);
		// Generate item for sub IVR
		while (( row_sub = mysql_fetch_row(result_sub)) != NULL) {
			fprintf(f,"exten => %s,%s,%s\n",row_sub[0],row_sub[1],row_sub[2]);
 		}
	}
	mysql_free_result(result);
	mysql_free_result(result_sub);
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

	//Record call?
	query = "SELECT value FROM dialplan_config WHERE route_type = 'in' AND type = 'record_call' AND client_id = ";
	query+= (const char*) client_id;
	mysql_query(connection, query.c_str());
	result = mysql_store_result(connection);
	while (( row = mysql_fetch_row(result)) != NULL)
		if ( string(row[0]) == "1" ){
			fprintf(f,"exten => _X.,n,Set(fname=${STRFTIME(${EPOCH},,%%Y%%m%%d%%H%%M)}-%s-${CALLERID(num)}-${EXTEN})\n",client_id);
			fprintf(f,"exten => _X.,n,MixMonitor(/var/spool/asterisk/monitor/%s/${fname}.wav)\n",client_id);
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
	fprintf(f,"same => n,GotoIf($[\"${ARG6}\" != \"novm\"]?VM)\n");
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
			fprintf(f,"exten => _X.,n,MixMonitor(/var/spool/asterisk/monitor/%s/${fname}.wav)\n",client_id);
		}
	mysql_free_result(result);
	
	//Generate lcr routes
	query = "SELECT prefix, template FROM route_out WHERE use_lcr = 1 AND client_id = ";
	query+= (const char*) client_id;
	query+= " GROUP BY template ORDER BY template ASC;";
	mysql_query(connection, query.c_str());
	result = mysql_store_result(connection);
	while (( row = mysql_fetch_row(result)) != NULL){
		fprintf(f,"exten => _%s,n,Set(i=0)\n",row[1]);
		fprintf(f,"exten => _%s,n,Set(call_count=${ODBC_LCRCOUNT(%s,'%s')})\n",row[1],client_id,row[1]);			
		fprintf(f,"exten => _%s,n,While($[${i} < ${call_count}])\n",row[1]);
		fprintf(f,"exten => _%s,n,Set(trunk=${ODBC_LCRNUM(%s,'%s',${i})})\n",row[1],client_id,row[1]);
		fprintf(f,"exten => _%s,n,Macro(%s-external,${EXTEN:%s},60,rTtS(${ODBC_LCRSEC(%s,'%s',${i})}),%s-${trunk})\n",row[1],client_id,row[0],client_id,row[1],client_id);
		fprintf(f,"exten => _%s,n,Set(i=$[${i} + 1])\n",row[1]);
		fprintf(f,"exten => _%s,n,EndWhile\n",row[1]);
	}
	mysql_free_result(result);

	//Generate no lcr routes
	query = "SELECT route_out.prefix, route_out.template, trunks.number FROM route_out LEFT JOIN trunks ON route_out.trunk = trunks.id WHERE route_out.use_lcr = 0 AND route_out.client_id = ";
	query+= (const char*) client_id;
	query+= " ORDER BY route_out.template, route_out.priority ASC;";
	mysql_query(connection, query.c_str());
	result = mysql_store_result(connection);
	while (( row = mysql_fetch_row(result)) != NULL)
			fprintf(f,"exten => _%s,n,Macro(%s-external,${EXTEN:%s},60,rTt,%s-%s)\n",row[1],client_id,row[0],client_id,row[2]);
	mysql_free_result(result);

	fprintf(f,"exten => _X.,n,Hangup\n\n");

	fprintf(f,"exten => h,1,Set(ODBC_LCRSEC(${trunk},%s)=${CDR(billsec)})\n",client_id);
	fprintf(f,"exten => h,n,Hangup\n\n");

	fprintf(f,"\n[macro-%s-external]\n",client_id);
	fprintf(f,"exten => s,1,NoOp(Calling to ${ARG1}...)\n");
	fprintf(f,"same => n,ChanIsAvail(SIP/${ARG4})\n");
	fprintf(f,"same => n,GotoIf($[\"${AVAILCHAN}\" != \"\"]?exit:)\n");
	fprintf(f,"same => n,Dial(SIP/${ARG1}@${ARG4},${ARG2},${ARG3})\n");
	fprintf(f,"same => n(exit),Noop(Macros call done)\n");


	cout << "Done!" << endl;
}
