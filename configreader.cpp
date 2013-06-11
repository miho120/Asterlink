#include <stdio.h>
#include <libconfig.h>
#include <string.h>

string parser(const char* file, const char* block, const char* key)
{
    config_t cfg;               /*Returns all parameters in this structure */
    config_setting_t *setting;
    const char *str1;
    const char *str2;
    int tmp;
 
    const char *config_file_name = file;

    string value;

    /*Initialization */
    config_init(&cfg);
 
    /* Read the file. If there is an error, report it and exit. */
    if (!config_read_file(&cfg, config_file_name))
    {
        printf("\n%s:%d - %s", config_error_file(&cfg), config_error_line(&cfg), config_error_text(&cfg));
	config_destroy(&cfg);
        return "Error";
    }
 
    /*Read the parameter group*/
    setting = config_lookup(&cfg, block);
    if (setting != NULL){
        /*Read the string*/
        if (config_setting_lookup_string(setting, key, &str2))
        	value = (const char*) str2;
        else{
            printf("\nNo setting in configuration file.");
	    value = "";
	}
    }
    config_destroy(&cfg);
    return value;
}
