#ifndef _CTYPE_H
#define _CTYPE_H

#include "types.h"
#include "locale.h"

#ifdef __cplusplus
extern "C" {
#endif

int tolower(int __c);

#define ctype_alpha  0x0001
#define ctype_blank  0x0002
#define ctype_cntrl  0x0004
#define ctype_digit  0x0008
#define ctype_graph  0x0010
#define ctype_lower  0x0020
#define ctype_print  0x0040
#define ctype_punct  0x0080
#define ctype_space  0x0100
#define ctype_upper  0x0200
#define ctype_xdigit 0x0400

inline int isalpha(int c)
{
	return (c < 0 || c >= 256) ? 0 : (int)(_current_locale.ctype_cmpt_ptr->ctype_map_ptr[c] & ctype_alpha);
}
inline int isdigit(int c)
{
	return (c < 0 || c >= 256) ? 0 : (int)(_current_locale.ctype_cmpt_ptr->ctype_map_ptr[c] & ctype_digit);
}
inline int isspace(int c)
{
	return (c < 0 || c >= 256) ? 0 : (int)(_current_locale.ctype_cmpt_ptr->ctype_map_ptr[c] & ctype_space);
}
inline int isupper(int c)
{
	return (c < 0 || c >= 256) ? 0 : (int)(_current_locale.ctype_cmpt_ptr->ctype_map_ptr[c] & ctype_upper);
}
inline int isxdigit(int c)
{
	return (c < 0 || c >= 256) ? 0 : (int)(_current_locale.ctype_cmpt_ptr->ctype_map_ptr[c] & ctype_xdigit);
}
inline int isprintable(int c)
{
	return (c < 0 || c >= 256) ? 0 : (int)(_current_locale.ctype_cmpt_ptr->ctype_map_ptr[c] & ctype_print);
}
// added underscore to avoid naming conflicts
inline int _tolower(int c)
{
	return ((c < 0) || (c >= 0x100)) ? c : (int)(_current_locale.ctype_cmpt_ptr->lower_map_ptr[c]);
}
inline int toupper(int c)
{
	return (c < 0 || c >= 256) ? c : (int)(_current_locale.ctype_cmpt_ptr->upper_map_ptr[c]);
}

#ifdef __cplusplus
}
#endif
#endif
