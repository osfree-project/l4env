#define TOKEN_EMPTY		0
#define TOKEN_IDENT		1		/* identifier */
#define TOKEN_STRUCT	2		/* structure element (brackets etc.) */
#define TOKEN_STRING	3		/* string */
#define TOKEN_WEIRD		4		/* weird */
#define TOKEN_NUMBER	5		/* number */
#define TOKEN_EOS		99		/* end of string */

struct tokenizer_services {
	s32 (*parse)  (char *str,s32 max_tok,s32 *offbuf,s32 *lenbuf);
	s32 (*toktype)(char *str,s32 offset);
};
