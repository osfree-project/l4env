/*
 * \brief	DOpE command interpreter module
 * \date	2002-11-13
 * \author	Norman Feske <nf2@inf.tu-dresden.de>
 */

#include <stdio.h>

#include "dope-config.h"
#include "memory.h"
#include "widget.h"
#include "appman.h"
#include "hashtab.h"
#include "tokenizer.h"
#include "script.h"

#define WIDTYPE_HASHTAB_SIZE	32
#define WIDTYPE_HASH_CHARS		5

#define METHODS_HASHTAB_SIZE	32
#define METHODS_HASH_CHARS		5

#define ATTRIBS_HASHTAB_SIZE	32
#define ATTRIBS_HASH_CHARS		5

#define MAX_TOKENS 256

static struct memory_services		*mem;
static struct hashtab_services 		*hashtab;
static struct appman_services		*appman;
static struct tokenizer_services	*tokenizer;

static HASHTAB *widtypes;

static u32 tok_off[MAX_TOKENS];
static u32 tok_len[MAX_TOKENS];

/*** INTERNAL WIDGET TYPE REPRESENTATION ***/
struct widtype_struct {
	char	*ident;			/* name of widget type */
	void * (*create)(void);	/* widget creation routine */
	HASHTAB *methods;		/* widget methods information */
	HASHTAB *attribs;		/* widget attributs information */
};

/*** INTERNAL METHOD ARGUMENT REPRESENTATION ***/
struct methodarg_struct;
struct methodarg_struct {
	char	*arg_name;				/* argument name */
	char	*arg_type;				/* argument type identifier string */
	char	*arg_default;			/* argument default value */
	struct methodarg_struct	*next;	/* next argument in argument list */
};

/*** INTERNAL METHOD REPRESENTATION ***/
struct method_struct {
	char	*method_name;			/* method name string */
	char	*return_type;			/* identifier string of return type */
	void  *(*routine)(void *,...);
	struct methodarg_struct *args;	/* list of arguments */
};


/*** INTERNAL ATTRIBUTE REPRESENTATION ***/
struct attrib_struct {
	char	*name;					/* name of attribute */
	char	*type;					/* type of attribute */
	void  *(*get)(void *);			/* get function to request the attibute */
	void   (*set)(void *,void *);	/* set function to set the attribute */
	void   (*update)(void *,u16);/* function to be called after attribute changes */
};


/*** INTERNAL VARIABLE REPRESENTATION ***/

#define VAR_BASECLASS_WIDGET 	1
#define VAR_BASECLASS_LONG		2
#define VAR_BASECLASS_FLOAT		3
#define VAR_BASECLASS_STRING	4
#define VAR_BASECLASS_BOOLEAN	5


struct variable_struct {
	char	*name;		/* variable name */
	char	*type;		/* variable type identifier */
	void	*value;		/* variable value */
};

int init_script(struct dope_services *d);



/**********************************/
/*** FUNCTIONS FOR INTERNAL USE ***/
/**********************************/

static char *new_symbol(char *s,u32 length) {
	u32 i;
	char *new = mem->alloc(length+1);
	if (!new) {
		DOPEDEBUG(printf("Script(new_symbol): out of memory\n");)
		return NULL;
	}
	for (i=0;i<length;i++) {
		new[i]=s[i];
	}
	new[length]=0;
	return new;
}


static char *get_tok_str(char *s,u32 tok_num) {
	static char tok_str[256];
	u32 i;
	s+=tok_off[tok_num];
	for (i=0;i<tok_len[tok_num];i++) {
		tok_str[i] = *(s++);
	}
	tok_str[tok_len[tok_num]]=0;
	return tok_str;
}


static u16 streq(char *s1,char *s2) {
	int i;
	for (i=0;i<256;i++) {
		if (*(s1) != *(s2++)) return 0;
		if (*(s1++) == 0) return 1;
	}
	return 1;
}


static u32 get_baseclass(char *vartype) {
	if (!vartype) return 0;
	if (streq(vartype,"long")) 	 return VAR_BASECLASS_LONG;
	if (streq(vartype,"float"))  return VAR_BASECLASS_FLOAT;
	if (streq(vartype,"string")) return VAR_BASECLASS_STRING;
	if (streq(vartype,"boolean")) 	return VAR_BASECLASS_BOOLEAN;
	if (hashtab->get_elem(widtypes,vartype)) return VAR_BASECLASS_WIDGET;
	if (streq(vartype,"Widget")) return VAR_BASECLASS_WIDGET;
	return 0;
}


static struct methodarg_struct *new_methodarg(void) {
	struct methodarg_struct *m_arg = mem->alloc(sizeof(struct methodarg_struct));

	if (!m_arg) {
		DOPEDEBUG(printf("Script(register_method): out of memory");)
		return NULL;
	}
	m_arg->arg_name=NULL;
	m_arg->arg_type=NULL;
	m_arg->arg_default=NULL;
	m_arg->next=NULL;
	return m_arg;
}


static s32 str2long(char *s) {
	s32 result=0;
	s32 sign=1;
	if (*s == '-')  {
		sign=-1;
		s++;
	}
	while (*s) result = result*10 + (*(s++) & 0xf);
	return sign*result;
}


static char *long2str(long value,char *dststr) {
	sprintf(dststr,"%lu",value);
	return dststr;
}


static s32 strlength(char *s) {
	s32 result=0;
	while (*(s++)) result++;
	return result;
}


static u8 *extract_string(char *str_token) {
	s32 strlen;
	u8 *s;
	u8 *result;
	str_token++;
	strlen = strlength(str_token);
	result=mem->alloc(strlen);
	s=result;
	if (!s) {
		ERROR(printf("Script(extract_string): out of memory!\n");)
		return NULL;
	}
	strlen--;
		
	while (strlen--) {
		if (*str_token == '\\') {
			switch (*(str_token+1)) {
			case '\\':	*(s++)='\\';str_token+=2;strlen--;break;
			case '"':	*(s++)='"'; str_token+=2;strlen--;break;
			case 'n':	*(s++)=0x0a;str_token+=2;strlen--;break;
			default:
				*(s++)=*(str_token++);
			}
		} else {
			*(s++)=*(str_token++);
		}
	}
	*s=0;

	return result;
}


static void *convert_arg(char *type,char *value,HASHTAB *app_vars) {
	struct variable_struct *var;

	switch (get_baseclass(type)) {
		case VAR_BASECLASS_LONG: 	return (void *)str2long(value);
		case VAR_BASECLASS_BOOLEAN:
			if (streq(value,"yes")) return (void *)1;
			if (streq(value,"true"))return (void *)1;
			if (streq(value,"on")) 	return (void *)1;
			if (streq(value,"1")) 	return (void *)1;
			return (void *)0;
		case VAR_BASECLASS_WIDGET:	
			var = hashtab->get_elem(app_vars,value);
			if (var) return var->value;
			return NULL;
		case VAR_BASECLASS_STRING:
			return (void *)extract_string(value);
	}
	return NULL;
}


static char return_str[256];

static char *convert_result(char *type,void *value) {
	switch (get_baseclass(type)) {
		case VAR_BASECLASS_BOOLEAN: 
		case VAR_BASECLASS_LONG: 	
			return long2str((long)value,return_str);
		case VAR_BASECLASS_STRING:  
			if (!value) return "<undefined>";
			return value;
	}
	return "ok";
}


static s32 argtypes_match_toktypes(char **arg_types,s16 *arg_toktypes,s16 num_args) {
	s16 i;
	for (i=0;i<num_args;i++) {
		switch (get_baseclass(arg_types[i])) {
		case VAR_BASECLASS_WIDGET:
			if (arg_toktypes[i]!=TOKEN_IDENT) return 0;
			break;
		case VAR_BASECLASS_STRING:
			if (arg_toktypes[i]!=TOKEN_STRING) return 0;
			break;
		}
	}
	return 1;
}



/*************************/
/*** SERVICE FUNCTIONS ***/
/*************************/

static void *register_widget_type(char *widtype_name,void *(*create_func)(void)) {
	struct widtype_struct *new = (struct widtype_struct *)mem->alloc(sizeof(struct widtype_struct));

	if (hashtab->get_elem(widtypes,widtype_name)) {
		DOPEDEBUG(printf("Script(register_widget_type): widget type already exists\n");)
		return NULL;
	}
	
	new->create  = create_func;
	new->methods = hashtab->create(METHODS_HASHTAB_SIZE,METHODS_HASH_CHARS);
	new->attribs = hashtab->create(ATTRIBS_HASHTAB_SIZE,ATTRIBS_HASH_CHARS);
	new->ident   = widtype_name;
	hashtab->add_elem(widtypes,widtype_name,new);

	/* return pointer to widget type structure */
	return new;
}


static void register_widget_method(struct widtype_struct *widtype,char *desc,void *methadr) {
	struct method_struct  *method;
	struct methodarg_struct *m_arg,**cl;
	u32 num_tokens;
	u32 i;
	
	num_tokens = tokenizer->parse(desc,MAX_TOKENS,tok_off,tok_len);
	
	method = (struct method_struct *)mem->alloc(sizeof(struct method_struct));
	method->routine = methadr;
	method->method_name = new_symbol(desc+tok_off[1],tok_len[1]);
	method->return_type = new_symbol(desc+tok_off[0],tok_len[0]);
	method->args = NULL;
	cl=(struct methodarg_struct **)&method->args;
	
	/* scan arguments and build argument list */
	if (!streq("void",get_tok_str(desc,3))) {
		for (i=3;i<num_tokens;) {
			m_arg=new_methodarg();
			
			/* read argument type */
			m_arg->arg_type = new_symbol(desc+tok_off[i],tok_len[i]);
			if (++i>=num_tokens) break;
			
			/* read argument name */
			m_arg->arg_name = new_symbol(desc+tok_off[i],tok_len[i]);
			if (++i>=num_tokens) break;
			
			/* check if a default value is specified */
			if ((*(desc+tok_off[i])) == '=') {
				if (++i>=num_tokens) break;
				m_arg->arg_default=new_symbol(desc+tok_off[i],tok_len[i]);
				if (++i>=num_tokens) break;
			} else {
				m_arg->arg_default=NULL;
			}
			
			m_arg->next=NULL;
			*cl=m_arg;
			cl=(struct methodarg_struct **)&m_arg->next;
			
			/* skip comma */
			if (++i>=num_tokens) break;
		}
	}

	hashtab->add_elem(widtype->methods,method->method_name,method);
}


static void register_widget_attrib(struct widtype_struct *widtype,char *desc,void *get,void *set,void *update) {
	struct attrib_struct  *attrib;
	u32 num_tokens;
	
	num_tokens = tokenizer->parse(desc,MAX_TOKENS,tok_off,tok_len);
	
	attrib = (struct attrib_struct *)mem->alloc(sizeof(struct attrib_struct));
	attrib->name   = new_symbol(desc+tok_off[1],tok_len[1]);
	attrib->type   = new_symbol(desc+tok_off[0],tok_len[0]);
	attrib->get    = get;
	attrib->set    = set;
	attrib->update = update;
	
	hashtab->add_elem(widtype->attribs,attrib->name,attrib);
}


static void *call_routine(void *(*rout)(void *,...),u16 ac,void **av) {

	switch (ac) {
	case 1:	return rout(av[0]);
	case 2:	return rout(av[0],av[1]);
	case 3:	return rout(av[0],av[1],av[2]);
	case 4:	return rout(av[0],av[1],av[2],av[3]);
	case 5:	return rout(av[0],av[1],av[2],av[3],av[4]);
	case 6:	return rout(av[0],av[1],av[2],av[3],av[4],av[5]);
	case 7:	return rout(av[0],av[1],av[2],av[3],av[4],av[5],av[6]);
	case 8:	return rout(av[0],av[1],av[2],av[3],av[4],av[5],av[6],av[7]);
	case 9:	return rout(av[0],av[1],av[2],av[3],av[4],av[5],av[6],av[7],av[8]);
	}
	return NULL;
}


static char *exec_command(u32 app_id,char *cmd) {
	s32 num_tokens;
	s32 i;
	HASHTAB *app_vars;
	WIDGET *w;
	struct widtype_struct *w_type;
	struct attrib_struct *attrib;
	struct method_struct *meth;
	s32 assign_flag=0;
	s32 method_tok=0;
	s32 num_args;
	char	*meth_name;
	void	*res_value=NULL;
	char	*res_type="typeless";
	char	*tag;
	char	*value;
	void	(*update)(void *,u16);
	struct variable_struct *var;
	struct methodarg_struct *m_arg;
	static void 	*args[20];
	static char 	*arg_types[20];
	static s16 	 arg_toktypes[20];

	num_tokens = tokenizer->parse(cmd,MAX_TOKENS,tok_off,tok_len);
	app_vars = appman->get_variables(app_id);

	if (!app_vars) return "Error: invalid application id";

	/* check, if there is an assignment */
	if (*(cmd+tok_off[1]) == '=') {
	
		assign_flag=1;		/* set assignment flag */
		method_tok=2;		/* method name has token index 2 */
	}

	res_type=NULL;

	/* execute method */
	if (*(cmd+tok_off[1+method_tok]) == '.') {	

		/* determine widget with the given variable symbol */
		var = hashtab->get_elem(app_vars,get_tok_str(cmd,0+method_tok));
		if (!var) return "Error: variable does not exists";
		w = var->value;
		
		/* get widget type information */
		w_type = hashtab->get_elem(widtypes,var->type);
		if (!w_type) {
			return "Error: invalid data type";
		}

		/* get information structure of the method to call */
		meth_name=get_tok_str(cmd,2+method_tok);
		meth = hashtab->get_elem(w_type->methods,meth_name);
		if (meth) {

			args[0]=w;
			num_args=1;
			m_arg=meth->args;
			i=4+method_tok;	/* index of first parameter */
			
			/* set needed parameters */
			while (m_arg) {
				if (m_arg->arg_default) break;
				
				value = get_tok_str(cmd,i);
				
				arg_types[num_args]=m_arg->arg_type;
				arg_toktypes[num_args]=tokenizer->toktype(value,0);
				args[num_args++]=convert_arg(m_arg->arg_type,value,app_vars);
				
				m_arg=m_arg->next;
				if (++i>=num_tokens) break;
				if (++i>=num_tokens) break;	/* skip comma */
			}
			
			/* set optional parameters */
			while (m_arg) {
				tag = get_tok_str(cmd,i)+1;
				value = m_arg->arg_default;
				
				/* parameter specified ? */
				if (streq(tag,m_arg->arg_name)) {
					if (++i>=num_tokens) break;	/* skip tag */
					value = get_tok_str(cmd,i);
					if (++i>=num_tokens) break;	/* skip value */
				}

				arg_types[num_args]=m_arg->arg_type;
				arg_toktypes[num_args]=tokenizer->toktype(value,0);
				args[num_args++]=convert_arg(m_arg->arg_type,value,app_vars);
				m_arg=m_arg->next;
			}

			/* argument type checking */
			if (!argtypes_match_toktypes(arg_types,arg_toktypes,num_args)) {
				return "argument type conflict";
			}
			
			res_value = call_routine(meth->routine,num_args,args);
			res_type  = meth->return_type;
			
			/* deallocate string arguments */
			for (i=1;i<num_args;i++) {
				if (get_baseclass(arg_types[i])==VAR_BASECLASS_STRING) {
					mem->free(args[i]);
				}
			}	
		
		} else if (streq(meth_name,"set")) {
			update=NULL;
			for (i=method_tok+4;i<num_tokens;) {
				
				tag=get_tok_str(cmd,i)+1;
				attrib=hashtab->get_elem(w_type->attribs,tag);

				if (++i>=num_tokens) break;
				value = get_tok_str(cmd,i);
				if (++i>=num_tokens) break;

				if (attrib) {
					arg_types[0]=attrib->type;
					arg_toktypes[0]=tokenizer->toktype(value,0);
					if (argtypes_match_toktypes(arg_types,arg_toktypes,1)) {
				
						if (attrib->update) update=attrib->update;
					
						args[0]=convert_arg(attrib->type,value,app_vars);
						if (attrib->set) attrib->set(w,args[0]);
						if (get_baseclass(attrib->type)==VAR_BASECLASS_STRING) {
							mem->free(args[0]);
						}
					}
				}
			}
			
			/* update widget */
			if (update) update(w,1);
		
		} else if ((attrib = hashtab->get_elem(w_type->attribs,meth_name))) {
		
			res_type = attrib->type;
			if (attrib->get) res_value = attrib->get(w);
			else return "Error: attribute value is not requestable";
		
		} else {
			return "Error: such a method or attribute does not exists";
		}

		
	/* is there a keyword 'new' ? */
	} else if (streq("new",get_tok_str(cmd,2))) {
	
		w_type=hashtab->get_elem(widtypes,get_tok_str(cmd,3));
		if (!w_type) {
			return "Error: such a datatype does not exists";
		}
		res_type = w_type->ident;
		res_value = w_type->create();
		if (res_value) {
			((WIDGET *)res_value)->gen->set_app_id((WIDGET *)res_value,app_id);
		}
		
	}
//	printf("result = %lu, type is %s\n",(long)res_value,res_type);

	/* assign method result to variable */
	if (assign_flag && res_type) {

		var = hashtab->get_elem(app_vars,new_symbol(cmd,tok_len[0]));
		if (var) {
			/* do something with the old variable contents */
		} else {
			var = mem->alloc(sizeof(struct variable_struct));
		}
		
		var->name		= new_symbol(cmd,tok_len[0]);
		var->value 		= res_value;
		var->type		= res_type;
		if (streq(var->type,"Widget")) {
			var->type = ((WIDGET *)var->value)->gen->get_type(var->value);
		}
//		printf("var: name=%s, value=%lu, type=%s\n",var->name,var->value,var->type);		
//		printf("assign new value %lu to variable %s\n",(long)res_value,get_tok_str(cmd,0));
		hashtab->add_elem(app_vars,var->name,var);
//		hashtab_print_info(app_vars);	
	}
	return convert_result(res_type,res_value);
} 


/****************************************/
/*** SERVICE STRUCTURE OF THIS MODULE ***/
/****************************************/

static struct script_services services = {
	register_widget_type,
	(void (*)(void *,char *,void *))register_widget_method,
	(void (*)(void *,char *,void *,void *,void *))register_widget_attrib,
	exec_command,
};


/**************************/
/*** MODULE ENTRY POINT ***/
/**************************/

int init_script(struct dope_services *d) {

	mem			= d->get_module("Memory 1.0");	
	hashtab 	= d->get_module("HashTable 1.0");
	appman		= d->get_module("ApplicationManager 1.0");
	tokenizer	= d->get_module("Tokenizer 1.0");
	
	widtypes = hashtab->create(WIDTYPE_HASHTAB_SIZE,WIDTYPE_HASH_CHARS);
	
	d->register_module("Script 1.0",&services);
	return 1;
}
