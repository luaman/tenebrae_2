#ifdef _WIN32
# include <windows.h>
#else
#include <stdarg.h>
#define strnicmp strncasecmp
#endif

#include <stdio.h>
#include <GL/glu.h>

#include <string>
#include <vector>

#include "nvparse_errors.h"
#include "nvparse.h"


//void yyinit(char*);
//int yyparse(void);


// RC1.0  -- register combiners 1.0 configuration
bool rc10_init(char *);
int  rc10_parse();
bool is_rc10(const char *);

nvparse_errors errors;
int line_number;
char * myin = 0;

void nvparse(const char * input_string, int argc /* = 0 */,...)
{
    if (NULL == input_string)
    {
        errors.set("NULL string passed to nvparse");
        return;
    }
    
    char * instring = strdup(input_string);


	// register combiners (1 and 2)
	if(is_rc10(instring))
	{
		if(rc10_init(instring))
		{
			rc10_parse();
		}
	}
    else
    {
        errors.set("invalid string.\n \
                    first characters must be: !!VP1.0 or !!VSP1.0 or !!RC1.0 or !!TS1.0\n \
                    or it must be a valid DirectX 8.0 Vertex Shader");
    }
    free(instring);
}

char * const * const nvparse_get_errors()
{
    return errors.get_errors();
}

char * const * const nvparse_print_errors(FILE * errfp)
{
    for (char * const *  ep = nvparse_get_errors(); *ep; ep++)
    {
        const char * errstr = *ep;
        fprintf(errfp, "%s\n", errstr);
    }
    return nvparse_get_errors();
}

const int* nvparse_get_info(const char* input_string, int* pcount)
{
	return 0;
}
