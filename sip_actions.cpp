#include <stdio.h>
#include <iostream>
#include <string.h>
#include <mysql/mysql.h>
#include "configreader.h"

using namespace std;

int sip_user_gen(char* user)
{
	MYSQL *connection, mysql;
        MYSQL_RES *result;
        MYSQL_ROW row;
        int query_state;
	char *secret;
	string tmp, conf_dir;
	FILE *f;
	conf_dir = parser("setting.conf","directories","asterisk_etc");        
	tmp = conf_dir + "clients/" + (const char*) user + "/sip_users.conf";
	f=fopen(tmp.c_str(), "w+t");
	mysql_init(&mysql);
        connection = mysql_real_connect(&mysql,parser("setting.conf","database","host").c_str(),parser("setting.conf","database","user").c_str(),parser("setting.conf","database","password").c_str(),parser("setting.conf","database","base").c_str(),3306,0,0);
	if (connection == NULL) {
        	//cout << mysql_error(sql) << endl;
        	return 1;
        }
	tmp = "select md5secret,number FROM user_numbers WHERE client_id = ";
	tmp = tmp + (const char*) user;
	mysql_query(connection, tmp.c_str());
	result = mysql_store_result(connection);
	while (( row = mysql_fetch_row(result)) != NULL) {
          secret = row[0];

	fprintf(f,"[%s-%s]\n",user,row[1]);
	fprintf(f,"type=friend\n");
	fprintf(f,"context=%s-out\n",user);
	fprintf(f,"md5secret=%s\n",secret);
	fprintf(f,"dtmfmode=RFC2833\n");
	fprintf(f,"disallow=all\n");
	fprintf(f,"allow=ulaw\n");
	fprintf(f,"allow=alaw\n");
	fprintf(f,"nat=force_rport,comedia\n");
	fprintf(f,"qualify=yes\n");
	fprintf(f,"host=dynamic\n\n");
 }
	fclose(f);

        mysql_free_result(result);
         mysql_close(connection);
	return 0;


//[miho-100]
//type=friend
//context=miho-out
//md5secret=f841cbe00bd758b24013db7d905d4b21
//dtmfmode=RFC2833
//disallow=all
//allow=ulaw
//allow=alaw
//nat=force_rport,comedia
//qualify=yes
//host=dynamic
}
