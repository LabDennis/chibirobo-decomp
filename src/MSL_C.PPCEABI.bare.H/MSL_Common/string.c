#include "types.h"
#include "errno.h"
#include "stdio.h"

char* __strerror(int errnum, char* str);
#define BYTE_HIGH_BITS 0x80808080u
#define ZERO_BYTE_BIAS 0xFEFEFEFFu

#define set_char_map(map, ch) map[(u8)(ch) >> 3] |= (1 << ((ch) & 7))
#define tst_char_map(map, ch) (map[(u8)(ch) >> 3] & (1 << ((ch) & 7)))

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
	u8* destBytes;
	const u8* sourceBytes;
	u32* destWords;
	const u32* sourceWords;
	u32 word, zeroProbe, align;

	sourceBytes = (const u8*)src;
	destBytes = (u8*)dst;

	if ((align = ((int)sourceBytes & 3)) != ((int)destBytes & 3)) {
		goto bytecopy;
	}

	if (align) {
		if ((*destBytes = *sourceBytes) == 0)
			return (dst);
		for (align = 3 - align; align; align--) {
			if ((*(++destBytes) = *(++sourceBytes)) == 0)
				return (dst);
		}
		++destBytes;
		++sourceBytes;
	}

	// Test for a possible NUL before copying an entire word.
	sourceWords = (const u32*)sourceBytes;
	word = *sourceWords;

	zeroProbe = word + ZERO_BYTE_BIAS;

	zeroProbe &= BYTE_HIGH_BITS;
	if (zeroProbe)
		goto bytecopy;
	destWords = (u32*)destBytes - 1;

	do {
		*++destWords = word;
		word = *++sourceWords;

		zeroProbe = word + ZERO_BYTE_BIAS;
		zeroProbe &= BYTE_HIGH_BITS;
		if (zeroProbe)
			goto adjust;
	} while (1);

adjust:
	destBytes = (u8*)(destWords + 1);
	sourceBytes = (const u8*)sourceWords;
bytecopy:
	if ((*destBytes = *sourceBytes) == 0)
		return dst;
	do {
		if ((*(++destBytes) = *(++sourceBytes)) == 0)
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
 * @note Address: 0x80158D30
 * @note Size: 0x128
 */
int strcmp(const char* str1, const char* str2)
{
	const u8* left = (const u8*)str1;
	const u8* right = (const u8*)str2;
	const u32* leftWords;
	const u32* rightWords;
	u32 align, leftValue, rightValue, zeroProbe;

	leftValue = *left;
	rightValue = *right;
	if (leftValue - rightValue) {
		return (leftValue - rightValue);
	}

	if ((align = ((int)left & 3)) != ((int)right & 3)) {
		goto bytecopy;
	}
	if (align) {
		if (leftValue == 0) {
			return 0;
		}
		for (align = 3 - align; align; align--) {
			leftValue = *(++left);
			rightValue = *(++right);
			if (leftValue - rightValue) {
				return (leftValue - rightValue);
			}
			if (leftValue == 0) {
				return 0;
			}
		}
		left++;
		right++;
	}

	leftWords = (const u32*)left;
	rightWords = (const u32*)right;
	leftValue = *leftWords;
	rightValue = *rightWords;
	zeroProbe = leftValue + ZERO_BYTE_BIAS;
	if (zeroProbe & BYTE_HIGH_BITS) {
		goto adjust;
	}
	while (leftValue == rightValue) {
		leftValue = *++leftWords;
		rightValue = *++rightWords;
		zeroProbe = leftValue + ZERO_BYTE_BIAS;
		if (zeroProbe & BYTE_HIGH_BITS) {
			goto adjust;
		}
	}
	if (leftValue > rightValue)
		return 1;
	return -1;

adjust:
	left = (const u8*)leftWords;
	right = (const u8*)rightWords;
	leftValue = *left;
	rightValue = *right;
	if (leftValue - rightValue) {
		return (leftValue - rightValue);
	}
bytecopy:
	if (leftValue == 0) {
		return 0;
	}
	do {
		leftValue = *(++left);
		rightValue = *(++right);
		if (leftValue - rightValue) {
			return (leftValue - rightValue);
		}
		if (leftValue == 0) {
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
