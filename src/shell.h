#ifndef __SHELL_H__
#define __SHELL_H__

#ifdef __cplusplus
extern "C" {
#endif

#define SHELL_TASK_PRIO		49
#define SHELL_STK_SIZE		4096

//typedef u8 (EMPTY)(u8);
//typedef u8 (GETCHAR)(u8, u8, u8 *);
//typedef u8 (PUTCHAR)(u8, u8, u8);
//extern GETCHAR *SHELL_GET;

int shell_get(char *s);
void shell_init(void);



#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* __SHELL_H__ */
