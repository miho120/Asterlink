#include <stdio.h>
#include <libconfig.h>

const char *parser(const char* file, const char* block, const char* key)
{
    config_t cfg;               /*Returns all parameters in this structure */
    config_setting_t *setting;
    const char *str1, *str2;
    int tmp;
 
    const char *config_file_name = file;
 
    const char *value;

    /*Initialization */
    config_init(&cfg);
 
    /* Read the file. If there is an error, report it and exit. */
    if (!config_read_file(&cfg, config_file_name))
    {
        printf("\n%s:%d - %s", config_error_file(&cfg), config_error_line(&cfg), config_error_text(&cfg));
	config_destroy(&cfg);
        return "Error";
    }
 
    /* Get the configuration file name. */
    //if (config_lookup_string(&cfg, "filename", &str1))
    //    printf("\nFile Type: %s", str1);
    //else
    //    printf("\nNo 'filename' setting in configuration file.");
 
    /*Read the parameter group*/
    setting = config_lookup(&cfg, block);
    if (setting != NULL)
    {
        /*Read the string*/
        if (config_setting_lookup_string(setting, key, &str2))
        {
            printf("\n%s: %s", key, str2);
	    value = str2;
        }
        else
            printf("\nNo setting in configuration file.");
 
        /*Read the integer*/
//        if (config_setting_lookup_int(setting, "param2", &tmp))
//      {
//            printf("\nParam2: %d", tmp);
//        }
//        else
//            printf("\nNo 'param2' setting in configuration file.");
 
        printf("\n");
    }
 
    config_destroy(&cfg);
    return value;
}
 
 
