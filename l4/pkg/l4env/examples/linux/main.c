#include <stdio.h>
#include <l4/env/errno.h>

extern char* strerror(int);

static char *own_strerror(int num){
    if(num==201) return "twohundredandone";
    if(num==202) return "twohundredandtwo";
    return strerror(num);
}

static char *own_strerror2(int num){
    if(num==1000) return "thousand";
    return 0;
}

enum own_errcode_t{
    OWN_EFANCY = 500,
    OWN_EHAPPY,
    OWN_EGLAD,
    OWN_EMAD,
    OWN_ERAGING,
    OWN_ESTONED,
    OWN_EIGNORE,
    OWN_EBORED,
};

L4ENV_ERR_DESC_STATIC(err_desc, 
    { OWN_EFANCY,	"fancy error, yeah" },
    { OWN_EHAPPY,	"i feel happy despite the errors" },
    { OWN_EGLAD,	"be glad nothing worse happened" },
    { OWN_EMAD,		"oingoing, got the cow" },
    { OWN_ERAGING,	"I hate these fucked up errors, run for your life" },
    { OWN_ESTONED,	"someone reported an error, who cares" },
    { OWN_EIGNORE,	"no error" },
    { OWN_EBORED,	"errors, errors, nothing than errors" }
);

L4ENV_ERR_FN_DESC_STATIC(fn_desc, own_strerror, "Unknown error");

int main(int argc, char**argv){
    int i, err;
    const char *e;

    /* the description definition can be placed inside a function too.
       Use the static version then. */
    L4ENV_ERR_FN_DESC_STATIC(fn_desc2, own_strerror2, "Unknown error");

    l4env_err_register_fn(&fn_desc);
    l4env_err_register_fn(&fn_desc2);

    err = l4env_err_register_desc(&err_desc);
    if(err!=0) printf("l4env_err_register_array() returned %d (%s)\n",
		      err, l4env_strerror(err));

    for(i=0; i< 10000; i++){
	e = l4env_strerror(i);
	if(e != l4env_err_unknown)
	    printf("%4d - %s\n", i, e);
    }
    return 0;
}
