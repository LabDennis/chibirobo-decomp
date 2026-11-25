extern void (*__stdio_exit)(void);

extern void __close_all(void);

/**
 * @note Address: 0x80156878
 * @note Size: 0x10
 */
void __stdio_atexit(void)
{
	__stdio_exit = __close_all;
}
