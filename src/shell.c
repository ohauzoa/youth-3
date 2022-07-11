

#include <Arduino.h>
#include "shell.h"
#include "my_lv_ports.h"
#include <ctype.h>
#include <stdio.h>
#include "main.h"

typedef uint8_t (EMPTY)(uint8_t);
typedef uint8_t (GETCHAR)();
typedef uint8_t (PUTCHAR)(uint8_t, uint8_t, uint8_t);

#define CR 0x0d 
#define BS 0x08 

#define MAX_ARGV			64
#define MAX_ARGC			6
#define HIS_MAX				4

char ARG_Buff[MAX_ARGC][16] = { {0,}, };

char HisBuff[HIS_MAX][64] = { {0,}, };
int32_t  HisCount               = 0;
int32_t  HisIndex               = 0;


GETCHAR *SHELL_GET;

TaskHandle_t task_shell;











struct cmd_struct {
	char *string;
	int (*func)(int argc, char **argv);
	char *comment;
	char *usage;
};


static int cmd_help(int argc, char **argv)
{
    printf("test ok\n");
}

struct cmd_struct cmd_list[] = {
	{"?",		cmd_help,					"this message print.",				"?"},
	{"tt",		cmd_help,					"this message print.",				"?"},

};







unsigned long simple_strtoul(const char *cp,char **endp,unsigned int base)
{
	unsigned long result = 0,value;

	if (*cp == '0') {
		cp++;
		if ((*cp == 'x') && isxdigit(cp[1])) {
			base = 16;
			cp++;
		}
		if (!base) {
			base = 8;
		}
	}
	if (!base) {
		base = 10;
	}
	while (isxdigit(*cp) && (value = isdigit(*cp) ? *cp-'0' : (islower(*cp)
	    ? toupper(*cp) : *cp)-'A'+10) < base) {
		result = result*base + value;
		cp++;
	}
	if (endp)
		*endp = (char *)cp;
	return result;
}

long simple_strtol(const char *cp,char **endp,unsigned int base)
{
	if(*cp=='-')
		return -simple_strtoul(cp+1,endp,base);
	return simple_strtoul(cp,endp,base);
}


void shell_get_init(void)
{
	SHELL_GET = (GETCHAR *)UartGet;
}

int shell_get(char *s)
{
	uint32_t cnt = strlen(s);
	uint8_t err;
	char  c;

	s += cnt;

	while((c = SHELL_GET()) != CR) {
		if (c == BS) {
			if(cnt > 0) {
				cnt--; 
				*s-- = ' ';
				printf("\b \b");
			}
		} else if (c == 0x1b) {
			cnt += 3;
			*s++ = c;
			*s++ = SHELL_GET();
			*s++ = SHELL_GET();

			break;
		} else {
			cnt++;
			*s++ = c;
			printf("%c",c);
		}


	}
	*s = 0;

	return(cnt);
}

static
int his_append(char *s)
{
	int loop;

	for( loop = 0; loop < HIS_MAX; loop++ )
	{
		if( strcmp( s, HisBuff[loop] ) == 0 ) 
		{
			HisIndex = 0;
			if( HisCount ) HisIndex = HisCount-1;
			return HisCount;
		} 
	} 

	if( HisCount < HIS_MAX )
	{
		strcpy( HisBuff[HisCount], s );
		HisCount++;
	}
	else
	{   
		for( loop = 1; loop < HIS_MAX ; loop++ )
		{
			strcpy( HisBuff[loop-1], HisBuff[loop] );
		}
		strcpy( HisBuff[HIS_MAX-1], s );
		HisIndex = HisCount-1;
	}

	HisIndex = 0; 
	if( HisCount ) HisIndex = HisCount-1;

	return HisCount;
}

static int his_set(char *s, int index )
{
	int loop;
	int len;

	len = strlen( s );

	for( loop = 0; loop < len; loop++ )
	{
		printf("\b \b");
	}
     
	strcpy( s, HisBuff[index] ); 
	printf( "%s", s );

	return index;
}

static int gets_his(char *s)
{
	int cnt = 0;
	char  *fp;
	char  c;
	uint8_t err;

     fp = s;
     
     while( 1 )
     {
		c = SHELL_GET();

		if( c == CR )
		{
			*s = 0;
			if( strlen( fp ) ) his_append( fp );
			break;
		}

		switch( c )
		{
		case 0x1a  : // ^Z
			if( HisIndex >= 0 )
			{ 
				his_set( fp, HisIndex );
				if( HisIndex ) HisIndex--;
				if( HisIndex >= HisCount ) HisIndex = HisCount -1;
				cnt = strlen( fp );
				s = fp + cnt; 
			} 
			break;
		case 0x18  : // ^X
			if( HisIndex < HisCount )
			{ 
				his_set( fp, HisIndex );
				HisIndex++;
				cnt = strlen( fp );
				s = fp + cnt; 
			}  
			break;
      
		case BS    :
			if( cnt > 0 )
			{ 
				cnt--; *s-- = ' '; 
				printf("\b \b");  
			} 
			break;

		default    :
			cnt++;
			*s++ = c;
			printf("%c", c );
			break;
		}
	}
   
	return(cnt);
}


static
int shell_mkargv(char *s, char *argv[]) 
{
	char *sp;
	int i, j;
	int len = strlen(s);

	for (i=0, j=0, sp=s; i < MAX_ARGV && j <= len; j++) {
		if (!*(s+j) && *(s+j-1))
			argv[i++] = sp;
		
		if (*(s+j) == ' ') {
			*(s+j) = '\0';
			if (j != 0 && *(s+j-1))
				argv[i++] = sp;
			sp = s + j + 1;
		}
	}
	argv[i] = 0;
	
	return i;
}

static
void shell_prompt( void )
{
	//char time[30];
	//rtc_fmt_ts(2, rtcTS, time);
	//printf("\rSHELL %s > ", time);
	printf("\rSHELL > ");
}

static
void shell_task(void *arg) 
{
	struct cmd_struct *cp;
	char *argv[MAX_ARGC];
	char str  [MAX_ARGV];
	int argc = 0;
	int idx =  0;
	int pos =  0;

//	printf("start shell!\n");

	for(idx = 0; idx < MAX_ARGC; idx++) argv[idx] = ARG_Buff[idx];

	shell_prompt();


	while (1) {
//		shell_prompt();
		memset(str, 0, sizeof(str));
//		memset(argv, 0, sizeof(argv));
		
		gets_his(str);

		argc = shell_mkargv(str, argv);
		cp = cmd_list;
		
		do {
			//upper_str( argv[0] );
			if (!strcmp(*argv, cp->string)) {
				cp->func(argc, argv);
				
				break;
			}
			cp++;
		} while (cp->string);

		pos = idx - 1;
		
		if (*str && !cp->string)
			printf("\r\n[shell message]: '%s' command not found!  use 'help' command. \n", argv[0]);

		printf("\n");
		shell_prompt();
		
		delay(10);
	}
}


void shell_init(void)
{
	uint8_t err;
	shell_get_init();

    //create a task that will be executed in the Task2code() function, with priority 1 and executed on core 1
    xTaskCreatePinnedToCore(
        shell_task,   /* Task function. */
        "shell",     /* name of task. */
        4096,       /* Stack size of task */
        NULL,        /* parameter of the task */
        19,           /* priority of the task */
        &task_shell,      /* Task handle to keep track of created task */
        APP_CPU_NUM);          /* pin task to core 1 */
    
    delay(500); 

}


