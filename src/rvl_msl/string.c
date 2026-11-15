#include "types.h"
#include "errno.h"
#include "stdio.h"

char* __strerror(int errnum, char* str);
#define K1 0x80808080
#define K2 0xFEFEFEFF

#define set_char_map(map, ch) map[(u8)(ch) >> 3] |= (u8)(1 << ((ch) & 7))
#define tst_char_map(map, ch) (map[(u8)(ch) >> 3] & (u8)(1 << ((ch) & 7)))

/**
 * @note Address: 0x80158F54
 * @note Size: 0x1C
 */
size_t(strlen)(const char* str)
{
	size_t len = -1;
	u8* p      = (u8*)str - 1;

	do
		len++;
	while (*++p);
	return (len);
}

/**
 * @note Address: 0x80158E9C
 * @note Size: 0xB8
 */
char*(strcpy)(char* dst, const char* src)
{
	register u8 *destb, *fromb;
	register u32 w, t, align;

	fromb = (u8*)src;
	destb = (u8*)dst;

	if ((align = ((int)fromb & 3)) != ((int)destb & 3)) {
		goto bytecopy;
	}

	if (align) {
		if ((*destb = *fromb) == 0)
			return (dst);
		for (align = 3 - align; align; align--) {
			if ((*(++destb) = *(++fromb)) == 0)
				return (dst);
		}
		++destb;
		++fromb;
	}

	w = *((int*)(fromb));

	t = w + K2;
	t &= ~w;

	t &= K1;
	if (t)
		goto bytecopy;
	--((int*)(destb));

	do {
		*(++((int*)(destb))) = w;
		w                    = *(++((int*)(fromb)));

		t = w + K2;
		t &= ~w;
		t &= K1;
		if (t)
			goto adjust;
	} while (1);

adjust:
	++((int*)(destb));
bytecopy:
	if ((*destb = *fromb) == 0)
		return dst;
	do {
		if ((*(++destb) = *(++fromb)) == 0)
			return dst;
	} while (1);

	return dst;
}

/**
 * @note Address: 0x80158E58
 * @note Size: 0x44
 */
char* strncpy(char* dst, const char* src, size_t n)
{
	const u8* p = (const u8*)src - 1;
	u8* q       = (u8*)dst - 1;
	u8 zero     = 0;

	n++;

	while (--n)
		if (!(*++q = *++p)) {
			while (--n)
				*++q = 0;
			break;
		}
	return (dst);
}

/**
 * @note Address: 0x8015A9F8 (Japan; stripped in USA and Europe)
 * @note Size: 0x2C
 */
char* strcat(char* dst, const char* src)
{
	const u8* p = (u8*)src - 1;
	u8* q       = (u8*)dst - 1;

	while (*++q)
		;

	q--;

	while (*++q = *++p)
		;

	return (dst);
}

/**
 * @note Address: N/A
 * @note Size: 0x4C
 */
char* strncat(char* dst, const char* src, size_t n)
{
	const u8* p = (u8*)src - 1;
	u8* q       = (u8*)dst - 1;

	while (*++q)
		;

	q--;
	n++;
	while (--n) {
		if (!(*++q = *++p)) {
			q--;
			break;
		}
	}
	*++q = 0;

	return dst;
}

/**
 * @note Address: 0x80158D30
 * @note Size: 0x128
 */
int strcmp(const char* str1, const char* str2)
{
	// bless metrowerks for this implementation

	register u8* left  = (u8*)str1;
	register u8* right = (u8*)str2;
	u32 align, l1, r1, x;

	l1 = *left;
	r1 = *right;
	if (l1 - r1) {
		return (l1 - r1);
	}

	if ((align = ((int)left & 3)) != ((int)right & 3)) {
		goto bytecopy;
	}
	if (align) {
		if (l1 == 0) {
			return 0;
		}
		for (align = 3 - align; align; align--) {
			l1 = *(++left);
			r1 = *(++right);
			if (l1 - r1) {
				return (l1 - r1);
			}
			if (l1 == 0) {
				return 0;
			}
		}
		left++;
		right++;
	}

	l1 = *(int*)left;
	r1 = *(int*)right;
	x  = l1 + K2;
	x &= ~l1;
	if (x & K1) {
		goto adjust;
	}
	while (l1 == r1) {
		l1 = *(++((int*)(left)));
		r1 = *(++((int*)(right)));
		x  = l1 + K2;
		if (x & K1) {
			goto adjust;
		}
	}

adjust:
	l1 = *left;
	r1 = *right;
	if (l1 - r1) {
		return (l1 - r1);
	}
bytecopy:
	if (l1 == 0) {
		return 0;
	}
	do {
		l1 = *(++left);
		r1 = *(++right);
		if (l1 - r1) {
			return (l1 - r1);
		}
		if (l1 == 0) {
			return 0;
		}
	} while (1);
}

/**
 * @note Address: 0x80158CF0
 * @note Size: 0x40
 */
int strncmp(const char* str1, const char* str2, size_t n)
{
	const u8* p1 = (u8*)str1 - 1;
	const u8* p2 = (u8*)str2 - 1;
	u32 c1, c2;

	n++;

	while (--n)
		if ((c1 = *++p1) != (c2 = *++p2))
			return (c1 - c2);
		else if (!c1)
			break;
	return 0;
}

/**
 * @note Address: 0x80158CC0
 * @note Size: 0x30
 */
char* strchr(const char* str, int chr)
{
	const u8* p = (u8*)str - 1;
	u32 c       = (chr & 0xFF);
	u32 ch;

	while (ch = *++p)
		if (ch == c)
			return ((char*)p);

	return (c ? 0 : (char*)p);
}

/**
 * @note Address: N/A
 * @note Size: 0x20
 */
void strcoll(void)
{
	// UNUSED FUNCTION
}

/**
 * @note Address: N/A
 * @note Size: 0x80
 */
void strxfrm(void)
{
	// UNUSED FUNCTION
}

/**
 * @note Address: N/A
 * @note Size: 0x48
 */
char* strrchr(const char* str, int chr)
{
	const u8* p = (u8*)str - 1;
	const u8* q = 0;
	u32 c       = (chr & 0xFF);
	u32 ch;

	while (ch = *++p)
		if (ch == c)
			q = p;

	if (q)
		return ((char*)q);

	return (c ? 0 : (char*)p);
}

/**
 * @note Address: N/A
 * @note Size: 0xD4
 */
void strpbrk(void)
{
	// UNUSED FUNCTION
}

/**
 * @note Address: N/A
 * @note Size: 0xD0
 */
void strspn(void)
{
	// UNUSED FUNCTION
}

/**
 * @note Address: N/A
 * @note Size: 0xD0
 */
void strcspn(void)
{
	// UNUSED FUNCTION
}

/**
 * @note Address: 0x80158B84
 * @note Size: 0x13C
 */
char* strtok(char* str, const char* set)
{
	u8 *p, *n;
	int c;
	u8 map[32]       = { 0 };
	static u8* empty = (u8*)"";
	static u8* s     = (u8*)"";

	if (str) {
		s = (u8*)str;
	}

	n = (u8*)set - 1;
	while ((c = *++n)) {
		set_char_map(map, c);
	}

	n = s - 1;
	while ((c = *++n)) {
		if (!tst_char_map(map, c)) {
			break;
		}
	}

	if (!c) {
		s = empty;
		return NULL;
	}

	p = n;
	while ((c = *++n)) {
		if (tst_char_map(map, c)) {
			break;
		}
	}

	if (!c) {
		s = empty;
		return (char*)p;
	}

	s  = n + 1;
	*n = 0;
	return (char*)p;
}

/**
 * @note Address: N/A
 * @note Size: 0x68
 */
void strstr(void)
{
	// UNUSED FUNCTION
}

/**
 * @note Address: N/A
 * @note Size: 0x28
 */
char* strerror(int errnum)
{
	static char errstr[100];
	return __strerror(errnum, errstr);
}

/**
 * @note Address: N/A
 * @note Size: 0x458
 */
// stripped from the game, but its messages stay behind in rodata
char* __strerror(int errnum, char* str)
{
	switch (errnum) {
	case E2BIG:
		strcpy(str, "Argument list too long");
		break;
	case EACCES:
		strcpy(str, "Permission denied");
		break;
	case EAGAIN:
		strcpy(str, "Resource temporarily unavailable");
		break;
	case EBADF:
		strcpy(str, "Bad file descriptor");
		break;
	case EBUSY:
		strcpy(str, "Device busy");
		break;
	case ECHILD:
		strcpy(str, "No child processes");
		break;
	case EDEADLK:
		strcpy(str, "Resource deadlock avoided");
		break;
	case EDOM:
		strcpy(str, "Numerical argument out of domain");
		break;
	case EEXIST:
		strcpy(str, "File exists");
		break;
	case EFAULT:
		strcpy(str, "Bad address");
		break;
	case EFBIG:
		strcpy(str, "File too large");
		break;
	case EFPOS:
		strcpy(str, "File Position Error");
		break;
	case EILSEQ:
		strcpy(str, "Wide character encoding error");
		break;
	case EINTR:
		strcpy(str, "Interrupted system call");
		break;
	case EINVAL:
		strcpy(str, "Invalid argument");
		break;
	case EIO:
		strcpy(str, "Input/output error");
		break;
	case EISDIR:
		strcpy(str, "Is a directory");
		break;
	case EMFILE:
		strcpy(str, "Too many open files");
		break;
	case EMLINK:
		strcpy(str, "Too many links");
		break;
	case ENAMETOOLONG:
		strcpy(str, "File name too long");
		break;
	case ENFILE:
		strcpy(str, "Too many open files in system");
		break;
	case ENODEV:
		strcpy(str, "Operation not supported by device");
		break;
	case ENOENT:
		strcpy(str, "No such file or directory");
		break;
	case ENOERR:
		strcpy(str, "No error detected");
		break;
	case ENOEXEC:
		strcpy(str, "Exec format error");
		break;
	case ENOLCK:
		strcpy(str, "No locks available");
		break;
	case ENOMEM:
		strcpy(str, "Cannot allocate memory");
		break;
	case ENOSPC:
		strcpy(str, "No space left on device");
		break;
	case ENOSYS:
		strcpy(str, "Function not implemented");
		break;
	case ENOTDIR:
		strcpy(str, "Not a directory");
		break;
	case ENOTEMPTY:
		strcpy(str, "Directory not empty");
		break;
	case ENOTTY:
		strcpy(str, "Inappropriate ioctl for device");
		break;
	case ENXIO:
		strcpy(str, "Device not configured");
		break;
	case EPERM:
		strcpy(str, "Operation not permitted");
		break;
	case EPIPE:
		strcpy(str, "Broken pipe");
		break;
	case ERANGE:
		strcpy(str, "Result too large");
		break;
	case EROFS:
		strcpy(str, "Read-only file system");
		break;
	case ESIGPARM:
		strcpy(str, "Signal error");
		break;
	case ESPIPE:
		strcpy(str, "Illegal seek");
		break;
	case ESRCH:
		strcpy(str, "No such process");
		break;
	case EUNKNOWN:
		strcpy(str, "Unknown error");
		break;
	case EXDEV:
		strcpy(str, "Cross-device link");
		break;
	default:
		sprintf(str, "Unknown Error (%d)", errnum);
		break;
	}
	return str;
}
