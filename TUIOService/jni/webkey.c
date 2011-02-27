/*
Copyright (C) 2010  Peter Mora, Zoltan Papp

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef _WIN32_WCE /* Some ANSI #includes are not available on Windows CE */
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <signal.h>
#include <fcntl.h>
#endif /* !_WIN32_WCE */

#include <time.h>
#include <stdlib.h>
#include <stdarg.h>
#include <assert.h>
#include <string.h>
#include <ctype.h>
#include <limits.h>
#include <stddef.h>
#include <stdio.h>

#include <vector>
#include <map>
#include <algorithm>


#include <netdb.h>

#include <sys/wait.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <sys/mman.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/time.h>

#include "suinput.h"
#include <stdio.h>
//#include <sys/types.h>
#include <sys/socket.h>
//#include <netinet/in.h>
#include <stdlib.h>
#include <strings.h>
#include "KeycodeLabels.h"
#include "kcm.h"
#include <sys/mman.h>
#include <linux/fb.h>
#include <linux/if.h>
#include "png.h"
#include "jpeglib.h"
#include <errno.h>
#include <signal.h>
#include <sys/wait.h>
#include <netdb.h>
//#include <sys/stat.h>

#include <limits.h>
#include "mongoose.h"
#include "base64.h"

//#include <sys/wait.h>
//#include <unistd.h>
#include <linux/input.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <termios.h>
#include <dirent.h>

#include "minizip/zip.h"

#include <android/log.h>

#undef stderr
FILE *stderr = stderr;

#ifdef MYDEB
#define FB_DEVICE "/android/dev/graphics/fb0"
#else
#define FB_DEVICE "/dev/graphics/fb0"
#endif

#define FILENAME_MAX 4096
//#define LINESIZE 4096

//static HashMap* sessions;


struct SESSION
{
	int ptm;
	int oldwidth;
	int oldheight;
	int alive;
	pid_t pid;
	time_t lastused;
	pthread_mutex_t mutex;
};

struct eqstr
{
	bool operator()(const char* s1, const char* s2) const
	{
		return strcmp(s1, s2) == 0;
	}
};

static std::map<std::string, time_t> access_times;
static std::string logfile;

char modelname[PROP_VALUE_MAX];

char langret[1024];
static char deflanguage[PROP_VALUE_MAX];//PROP_VALUE_MAX == 92

static bool has_ssl = false;

char* humandate(char* buff, long int epoch, int dateformat, int datesep, int datein, int datetimezone);
static pthread_mutex_t logmutex;
static void
access_log(const struct mg_request_info *ri, const char* s)
{
	pthread_mutex_lock(&logmutex);
	std::string log;
	if (ri && ri->remote_user)
	{
		log = ri->remote_user;
		log += ": ";
	}
	log += s;
	time_t last = access_times[log];

	struct timeval tv;
	gettimeofday(&tv,0);
	time_t now = tv.tv_sec;
	if (last == 0 || now > last+600)
	{
		FILE* f = fopen(logfile.c_str(),"a");
		if (f)
		{
			char conv[LINESIZE];
			fprintf(f,"[%s] %s\n",humandate(conv,now,0,0,0,0),log.c_str());
			fclose(f);
		}
		access_times[log] = now;
//		printf("%d\n",now);
//		printf(" %s %d\n",log.c_str(),access_times[log.c_str()]);
	}	
	pthread_mutex_unlock(&logmutex);
}

static void
read_post_data(struct mg_connection *conn,
                const struct mg_request_info *ri, char** post_data, int* post_data_len)
{
	int l = contentlen(conn);
	if (l > 65536)
		l = 65536;
	*post_data = new char[l+1];
	*post_data_len = mg_read(conn,*post_data,l);
	(*post_data)[*post_data_len] = 0;
	//printf("!%s!\n",*post_data);
}




static std::vector<SESSION*> sessions;

static bool samsung = false;
static bool force_240 = false;
static bool is_zte_blade = false;

static int pipeforward[2];
static int pipeback[2];
static FILE* pipein;
static FILE* pipeout;

static char* server_username = NULL;
static char* server_random = NULL;
static bool server;
static int server_changes = 0;

static std::string requested_username;
static std::string requested_password;
static __u32 requested_ip;

static int port=80;
static int sslport=443;
static char upload_random[10];
//static int touchcount=0;
static std::string dir;
static int dirdepth = 0;
static std::string passfile;
//static std::string admin_password;
struct mg_context       *ctx;
static char position_value[512];
static std::string mimetypes[32];
//static std::string contacts;
//static std::string sms;
volatile static bool firstfb = false;

volatile static int position_id = 0;

volatile static bool gps_active = false;
volatile static time_t last_gps_time =0;

volatile static bool recording = false;
volatile static int recordingnum = 0;
volatile static int recordingnumfinished = -1;
static void
signal_handler(int sig_num)
{
        if (sig_num == SIGCHLD)
                while (waitpid(-1, &sig_num, WNOHANG) > 0);
        else
                exit_flag = sig_num;
}

struct MESSAGE
{
	std::string user;
	time_t timestamp;
	std::string message;
};

static time_t chat_random;
static std::vector<MESSAGE> chat_messages;
static std::map<std::string,int> chat_readby;
static std::map<std::string,int> chat_readby_real;
static std::map<std::string,int> chat_readby_count;
static int chat_count;
static pthread_mutex_t chatmutex;
static pthread_cond_t chatcond;


volatile static bool wakelock;
volatile static time_t wakelock_lastused;
static pthread_mutex_t wakelockmutex;

static void lock_wakelock()
{
	pthread_mutex_lock(&wakelockmutex);
	if (!wakelock)
	{
		wakelock = true;
		int fd = open("/sys/power/wake_lock", O_WRONLY);
		if(fd >= 0)
		{
			write(fd, "Webkey\n", 7);
			close(fd);
		}
	}
	struct timeval tv;
	gettimeofday(&tv,0);
	wakelock_lastused = tv.tv_sec;
	pthread_mutex_unlock(&wakelockmutex);
}
static void unlock_wakelock(bool force)
{
	pthread_mutex_lock(&wakelockmutex);
	if (wakelock)
	{
		struct timeval tv;
		gettimeofday(&tv,0);
		if (force || wakelock_lastused +30 < tv.tv_sec)
		{
			int fd = open("/sys/power/wake_unlock", O_WRONLY);
			if(fd >= 0)
			{
				wakelock = false;
				write(fd, "Webkey\n", 7);
				close(fd);
			}
		}
	}
	pthread_mutex_unlock(&wakelockmutex);
}

//from android source tree
static int sendit(int timeout_ms)
{
    int nwr, ret, fd;
    char value[20];

    fd = open("/sys/class/timed_output/vibrator/enable", O_RDWR);
    if(fd < 0)
        return errno;

    nwr = sprintf(value, "%d\n", timeout_ms);
    ret = write(fd, value, nwr);

    close(fd);

    return (ret == nwr) ? 0 : -1;
}

static int vibrator_on(int timeout_ms)
{
    /* constant on, up to maximum allowed time */
    return sendit(timeout_ms);
}

static int vibrator_off()
{
    return sendit(0);
}

static int
write_int(char const* path, int value)
{
    int fd;
    static int already_warned = 0;

    fd = open(path, O_RDWR);
    if (fd >= 0) {
        char buffer[20];
        int bytes = sprintf(buffer, "%d\n", value);
        int amt = write(fd, buffer, bytes);
        close(fd);
        return amt == -1 ? -errno : 0;
    } else {
        if (already_warned == 0) {
            printf("write_int failed to open %s\n", path);
            already_warned = 1;
        }
        return -errno;
    }
}


static void set_blink(bool blink,int onMS,int offMS)
{
	//from android source tree
	int freq, pwm;
	if (onMS > 0 && offMS > 0) {
		int totalMS = onMS + offMS;

		// the LED appears to blink about once per second if freq is 20
		// 1000ms / 20 = 50
		freq = totalMS / 50;
		// pwm specifies the ratio of ON versus OFF
		// pwm = 0 -> always off
		// pwm = 255 => always on
		pwm = (onMS * 255) / totalMS;

		// the low 4 bits are ignored, so round up if necessary
		if (pwm > 0 && pwm < 16)
			pwm = 16;
	} else {
		blink = 0;
		freq = 0;
		pwm = 0;
	}
	write_int("/sys/class/leds/red/device/blink",blink?1:0);
	write_int("/sys/class/leds/red/device/grpfreq",freq);
	write_int("/sys/class/leds/red/device/grppwm",pwm);

}

struct NOTIFY{
	bool smson;
	bool callon;
	int interval;
	bool vibrate;
	std::string vibratepatt;
	bool blink;
	int blinktype;
	int blinkon;
	int blinkoff;
	time_t lastalarm;
	bool smsalarmed;
	bool callalarmed;
	unsigned long lastsmstime;
	unsigned long lastcalltime;
};

static struct NOTIFY notify;

static int fbfd = -1;
static void *fbmmap = MAP_FAILED;
static int fbmmapsize = 0;
static int bytespp = 0;
static int lowest_offset = 0;
static struct fb_var_screeninfo scrinfo;
static png_byte* pict = NULL;
static png_byte** graph = NULL;
unsigned int* copyfb = NULL;
static int* lastpic = NULL;
static bool lastorient = 0;
static bool lastflip = 0;
static bool picchanged = true;
static int touchfd = -1;
static __s32 xmin;
static __s32 xmax;
static __s32 ymin;
static __s32 ymax;
static int newsockfd = -1;

static int max_brightness;

static bool dyndns = false;
static __u32 dyndns_last_updated_ip = 0;
static std::string dyndns_host;
static std::string dyndns_base64;


static pthread_mutex_t pngmutex;
static pthread_mutex_t diffmutex;
static pthread_mutex_t popenmutex;
static pthread_mutex_t uinputmutex;
//static pthread_mutex_t terminalmutex;
static pthread_cond_t diffcond;
static pthread_cond_t diffstartcond;
//static pthread_mutex_t smsmutex;
//static pthread_cond_t smscond;
//static pthread_mutex_t contactsmutex;
//static pthread_cond_t contactscond;

/*
static bool dyndns = true;
static __u32 dyndns_last_updated_ip = 0;
static std::string dyndns_host = "g1.homeip.net";
static std::string dyndns_base64 = "b3BlcmF0Omxha2F0MTIz";
*/

//base
static const char* KCM_BASE[] ={
"A 'A' '2' 'a' 'A'",
"B 'B' '2' 'b' 'B'",
"C 'C' '2' 'c' 'C'",
"D 'D' '3' 'd' 'D'",
"E 'E' '3' 'e' 'E'",
"F 'F' '3' 'f' 'F'",
"G 'G' '4' 'g' 'G'",
"H 'H' '4' 'h' 'H'",
"I 'I' '4' 'i' 'I'",
"J 'J' '5' 'j' 'J'",
"K 'K' '5' 'k' 'K'",
"L 'L' '5' 'l' 'L'",
"M 'M' '6' 'm' 'M'",
"N 'N' '6' 'n' 'N'",
"O 'O' '6' 'o' 'O'",
"P 'P' '7' 'p' 'P'",
"Q 'Q' '7' 'q' 'Q'",
"R 'R' '7' 'r' 'R'",
"S 'S' '7' 's' 'S'",
"T 'T' '8' 't' 'T'",
"U 'U' '8' 'u' 'U'",
"V 'V' '8' 'v' 'V'",
"W 'W' '9' 'w' 'W'",
"X 'X' '9' 'x' 'X'",
"Y 'Y' '9' 'y' 'Y'",
"Z 'Z' '9' 'z' 'Z'",
"COMMA ',' ',' ',' '?'",
"PERIOD '.' '.' '.' '/'",
"AT '@' 0x00 '@' '~'",
"SPACE 0x20 0x20 0x20 0x20",
"ENTER 0xa 0xa 0xa 0xa",
"0 '0' '0' '0' ')'",
"1 '1' '1' '1' '!'",
"2 '2' '2' '2' '@'",
"3 '3' '3' '3' '#'",
"4 '4' '4' '4' '$'",
"5 '5' '5' '5' '%'",
"6 '6' '6' '6' '^'",
"7 '7' '7' '7' '&'",
"8 '8' '8' '8' '*'",
"9 '9' '9' '9' '('",
"TAB 0x9 0x9 0x9 0x9",
"GRAVE '`' '`' '`' '~'",
"MINUS '-' '-' '-' '_'",
"EQUALS '=' '=' '=' '+'",
"LEFT_BRACKET '[' '[' '[' '{'",
"RIGHT_BRACKET ']' ']' ']' '}'",
"BACKSLASH '\\' '\\' '\\' '|'",
"SEMICOLON ';' ';' ';' ':'",
"APOSTROPHE '\'' '\'' '\'' '\"'",
"STAR '*' '*' '*' '<'",
"POUND '#' '#' '#' '>'",
"PLUS '+' '+' '+' '+'",
"SLASH '/' '/' '/' '?'"
};

//  ,!,",#,$,%,&,',(,),*,+,,,-,.,/
int spec1[] = {62,8,75,10,11,12,14,75,16,7,15,70,55,69,56,56,52};                      
int spec1sh[] = {0,1,1,1,1,1,1,0,1,1,1,1,0,0,0,1};
// :,;,<,=,>,?,@
int spec2[] = {74,74,17,70,18,55,77};
int spec2sh[] = {1,0,1,0,1,1,0};
// [,\,],^,_,`
int spec3[] = {71,73,72,13,69,68};
int spec3sh[] = {0,0,0,1,1,0};
// {,|,},~
int spec4[] = {71,73,72,68};
int spec4sh[] = {1,1,1,1,0};


struct BIND{
	int ajax;
	int disp;
	int kcm;
	bool kcm_sh;
	bool sms;
};

struct FAST{
	bool show;
	char* name;
	int  ajax;
};

struct DEVSPEC{
	int dev;
	int type;
	int code;
};

static std::map<std::string, int> device_names;
static std::vector<BIND*> speckeys;
static std::vector<FAST*> fastkeys;
static std::map<int, DEVSPEC> device_specific_buttons;
static int uinput_fd = -1;

bool startswith(const char* st, char* patt)
{
	int i = 0;
	while(patt[i])
	{
		if (st[i] == 0 || st[i]-patt[i])
			return false;
		i++;
	}
	return true;
}
bool urlcompare(const char* url, char* patt)
{
	int i = 0;
	while(true)
	{
		if (patt[i] == '*')
			return true;
		if ((!patt[i] && url[i]) || (patt[i] && !url[i]))
			return false;
		if (!patt[i] && !url[i])
			return true;
		if (url[i]-patt[i])
			return false;
		i++;
	}
}
bool cmp(const char* st, char* patt)
{
	int i = 0;
	while(patt[i] && st[i])
	{
		if (st[i]-patt[i])
			return false;
		i++;
	}
	if (st[i]-patt[i])
		return false;
	return true;
}


int contains(const char* st, const char* patt)
{
	int n = strlen(st);
	int m = strlen(patt);
	int i,j;
	for (i = 0; i<n-m+1; i++)
	{
		for (j=0;j<m;j++)
			if (st[i+j]!=patt[j])
				break;
		if (j==m)
			return i;
	}
	return 0;
}

char* humandur(char* buff, int secs)
{
	if (secs < 60)
		sprintf(buff,"%d secs",secs);
	else
	if (secs < 120)
		sprintf(buff,"1 min %d secs",secs%60);
	else
	if (secs < 3600)
		sprintf(buff,"%d mins %d secs",secs/60,secs%60);
	else
		sprintf(buff,"%d hours %d mins %d secs",secs/3600,(secs/60)%60,secs%60);
	return buff;
};

char* humandate(char* buff, long int epoch, int dateformat, int datesep, int datein, int datetimezone)
{
	if (datein == 1)
	{
		sprintf(buff,"%d",epoch);
		return buff;
	}
	time_t d = epoch + datetimezone*3600;
	struct tm* timeinfo;
	timeinfo = gmtime( &d );
	int a,b,c;
	if (dateformat == 0)
	{
		a = timeinfo->tm_mday; b = timeinfo->tm_mon+1; c = timeinfo->tm_year+1900;
	}
	if (dateformat == 1)
	{
		a = timeinfo->tm_mon+1; b = timeinfo->tm_mday; c = timeinfo->tm_year+1900;
	}
	if (dateformat == 2)
	{
		a = timeinfo->tm_year+1900; b = timeinfo->tm_mon+1; c = timeinfo->tm_mday;
	}
	if (datesep == 0) sprintf(buff,"%.2d/%.2d/%.2d %d:%.2d",a,b,c, timeinfo->tm_hour,timeinfo->tm_min);
	if (datesep == 1) sprintf(buff,"%.2d-%.2d-%.2d %d:%.2d",a,b,c, timeinfo->tm_hour,timeinfo->tm_min);
	if (datesep == 2) sprintf(buff,"%.2d.%.2d.%.2d. %d:%.2d",a,b,c, timeinfo->tm_hour,timeinfo->tm_min);
	return buff;
}


char* itoa(char* buff, int value)
{
	unsigned int i = 0;
	bool neg = false;
	if (value<0)
	{
		neg = true;
		value=-value;
	}
	if (value==0)
		buff[i++] = '0';
	else
		while(value)
		{
			buff[i++] = '0'+(char)(value%10);
			value = value/10;
		}
	if (neg)
		buff[i++] = '-';
	unsigned int j = 0;
	for(j = 0; j < (i>>1); j++)
	{
		char t = buff[j];
		buff[j] = buff[i-j-1];
		buff[i-j-1] = t;
	}
	buff[i++]=0;
	return buff;
}

long int getnum(const char* st)
{
	long int r = 0;
	long int i = 0;
	bool neg = false;
	if (st[i]=='-')
	{
		neg = true;
		i++;
	}
	while ('0'<=st[i] && '9'>=st[i])
	{
		r = r*10+(long int)(st[i]-'0');
		i++;
	}
	if (neg)
		return -r;
	else
		return r;
}
static void syst(const char* cmd)
{
//	pthread_mutex_lock(&popenmutex);
//	fprintf(pipeout,"S%s\n",cmd);
//	pthread_mutex_unlock(&popenmutex);
	system(cmd);
}
int getnum(char* st)
{
	int r = 0;
	int i = 0;
	bool neg = false;
	if (st[i]=='-')
	{
		neg = true;
		i++;
	}
	while ('0'<=st[i] && '9'>=st[i])
	{
		r = r*10+(int)(st[i]-'0');
		i++;
	}
	if (neg)
		return -r;
	else
		return r;
}

char* removesemicolon(char* to, const char* from)
{
	int i = 0;
	while(from[i] && i < LINESIZE - 1)
	{
		if(from[i] == ';')
			to[i] = ',';
		else
			to[i] = from[i];
		i++;
	}
	to[i++] = 0;
	return to;
}
void send_ok(struct mg_connection *conn, const char* extra = NULL,int size = 0)

{
	if (size)
	{
		if (extra)
			mg_printf(conn,"HTTP/1.1 200 OK\r\nCache-Control: no-store, no-cache, must-revalidate\r\nCache-Control: post-check=0, pre-check=0\r\nPragma: no-cache\r\nConnection: close\r\nContent-Length: %d\r\n%s\r\n\r\n",size,extra);
		else
			mg_printf(conn,"HTTP/1.1 200 OK\r\nCache-Control: no-store, no-cache, must-revalidate\r\nCache-Control: post-check=0, pre-check=0\r\nPragma: no-cache\r\nConnection: close\r\nContent-Length: %d\r\n\r\n",size);
	}
	else
	{
		if (extra)
			mg_printf(conn,"HTTP/1.1 200 OK\r\nCache-Control: no-store, no-cache, must-revalidate\r\nCache-Control: post-check=0, pre-check=0\r\nPragma: no-cache\r\nConnection: close\r\n%s\r\n\r\n",extra);
		else
			mg_printf(conn,"HTTP/1.1 200 OK\r\nCache-Control: no-store, no-cache, must-revalidate\r\nCache-Control: post-check=0, pre-check=0\r\nPragma: no-cache\r\nConnection: close\r\n\r\n");
	}
}
void clear(bool exit = true)
{
	int i;
	for(i=0; i < speckeys.size(); i++)
		if (speckeys[i])
			delete speckeys[i];
	speckeys.clear();
	for(i=0; i < fastkeys.size(); i++)
		if (fastkeys[i])
		{
			delete[] fastkeys[i]->name;
			delete fastkeys[i];
		}
	fastkeys.clear();
	if (uinput_fd != -1)
	{
		pthread_mutex_lock(&uinputmutex);
		suinput_close(uinput_fd);
		uinput_fd = -1;
		pthread_mutex_unlock(&uinputmutex);
	}
	if (exit)
	{
		if (pict)
			delete[] pict;
		pict = NULL;
		if (lastpic)
			delete[] lastpic;
		lastpic = NULL;
		if (graph)
			delete[] graph;
		graph = NULL;
		if (copyfb)
			delete[] copyfb;
		copyfb = NULL;
		if (gps_active)
			syst("/system/bin/am broadcast -a \"webkey.intent.action.GPS.STOP\" -n \"com.webkey/.GPS\"");
		if (server_username)
			delete[] server_username;
		server_username = NULL;
		if (server_random)
			delete[] server_random;
		if (fbmmap != MAP_FAILED)
			munmap(fbmmap, fbmmapsize);

		server_random = NULL;
		pthread_mutex_lock(&diffmutex);
		pthread_cond_broadcast(&diffstartcond);
		pthread_cond_broadcast(&diffcond);
		pthread_mutex_unlock(&diffmutex);
		sleep(1);
		pthread_mutex_destroy(&diffmutex);
		pthread_mutex_destroy(&popenmutex);
		pthread_mutex_destroy(&logmutex);
		pthread_mutex_destroy(&uinputmutex);
		pthread_mutex_destroy(&chatmutex);
		pthread_mutex_destroy(&wakelockmutex);
		pthread_cond_destroy(&diffcond);
		pthread_cond_destroy(&chatcond);
		pthread_cond_destroy(&diffstartcond);
		for(i=0; i < sessions.size(); i++)
		{
			pthread_mutex_destroy(&(sessions[i]->mutex));
			if (sessions[i]->alive)
			{
				kill(sessions[i]->pid,SIGKILL);
				close(sessions[i]->ptm);
			}
			delete sessions[i];
		}
		close(fbfd);
//		pthread_mutex_destroy(&smsmutex);
//		pthread_cond_destroy(&smscond);
//		pthread_mutex_destroy(&contactsmutex);
//		pthread_cond_destroy(&contactscond);
	}
}

void error(const char *msg,const char *msg2 = NULL, const char *msg3=NULL, const char * msg4=NULL)
{
    perror(msg);
    __android_log_print(ANDROID_LOG_INFO,"Webkey C++","service stoped");
    access_log(NULL,msg);
    if (msg2)
    {
	    perror(msg2);
	    __android_log_print(ANDROID_LOG_ERROR,"Webkey C++",msg2);
	    access_log(NULL,msg2);
    }
    if (msg3) 
    {
	    perror(msg3);
	    __android_log_print(ANDROID_LOG_ERROR,"Webkey C++",msg3);
	    access_log(NULL,msg3);
    }
    if (msg4)
    {
	perror(msg4);
	    __android_log_print(ANDROID_LOG_ERROR,"Webkey C++",msg4);
	    access_log(NULL,msg4);
    }
    clear();
    exit(1);
}
FILE* fo(const char* filename, const char* mode)
{
	std::string longname = dir + filename;
	FILE* ret = fopen(longname.c_str(),mode);
	if (!ret)
		error("Couldn't open ",longname.c_str(), " with mode ", mode);
//	else
//		printf("%s is opened.\n",longname.c_str());
	return ret;
}



void init_uinput()
{
	clear(false);
	usleep(10000);
	int i;
//	FILE* kl = fopen("/android/system/usr/keylayout/Webkey.kl","w");
#ifdef MYDEB
	FILE* kl = fopen("/android/dev/Webkey.kl","w");
#else
	FILE* kl = fopen("/dev/Webkey.kl","w");
#endif
	if (!kl)
	{
		error("Couldn't open Webkey.kl for writing.\n");
	}
	printf("Webkey.kl is opened.\n");
	i = 0;
	while (KEYCODES[i].value)
	{
		fprintf(kl,"key %d   %s   WAKE\n",KEYCODES[i].value,KEYCODES[i].literal);
		FAST* load = new FAST;
		load->show = 0;
		load->name = new char[strlen(KEYCODES[i].literal)+1];
		strcpy(load->name,KEYCODES[i].literal);
		load->ajax = 0;
		fastkeys.push_back(load);
		i++;
	}
	fclose(kl);

	FILE* fk = fo("fast_keys.txt","r");
	if (!fk)
		return;
	int ajax, show, id;
	while(fscanf(fk,"%d %d %d\n",&id,&show,&ajax) == 3)
	{
		if (id-1 < i)
		{
			fastkeys[id-1]->ajax = ajax;
			fastkeys[id-1]->show = show;
		}
	}
	fclose(fk);

	FILE* sk = fo("spec_keys.txt","r");
	if (!sk)
		return;
	int disp, sms;
	while(fscanf(sk,"%d %d %d\n",&sms,&ajax,&disp) == 3)
	{
		BIND* load = new BIND;
		load->ajax = ajax;
		load->disp = disp;
		load->sms = sms;
		load->kcm = 0;
		speckeys.push_back(load);
	}
	fclose(sk);

	if (samsung) //don't ask, it's not my mess
	{
		BIND* load = new BIND;
		load->ajax = 103;//g
		load->disp = 103;//g
		load->sms = 0;
		load->kcm = 0;
		speckeys.push_back(load);

		load = new BIND;
		load->ajax = 71;//G
		load->disp = 71;//G
		load->sms = 0;
		load->kcm = 0;
		speckeys.push_back(load);

		load = new BIND;
		load->ajax = 100;//d
		load->disp = 100;//d
		load->sms = 0;
		load->kcm = 0;
		speckeys.push_back(load);

		load = new BIND;
		load->ajax = 68;//D
		load->disp = 68;//D
		load->sms = 0;
		load->kcm = 0;
		speckeys.push_back(load);
	}

//	FILE* kcm = fopen("/android/system/usr/keychars/Webkey.kcm","w");
#ifdef MYDEB
	FILE* kcm = fopen("/android/dev/Webkey.kcm","w");
#else
	FILE* kcm = fopen("/dev/Webkey.kcm","w");
#endif
	if (!kcm)
	{
		error("Couldn't open Webkey.kcm for writing.\n");
	}
//	printf("Webkey.kcm is opened.\n");
	fprintf(kcm,"[type=QWERTY]\n# keycode       display number  base    caps    alt     caps_alt\n");
	int ii = 0;
	for(i=0; i<54; i++)
	{
		if (samsung && (i == 3 || i == 6))
		{
			fprintf(kcm,"%s %d %d\n",KCM_BASE[i],0,0);
			continue;
		}
		int k = 0;
		if (2*ii < speckeys.size())
		{
			k = speckeys[2*ii]->disp;
			speckeys[2*ii]->kcm_sh = false;
			if (i<28)
				speckeys[2*ii]->kcm = 29+i; //A-Z, COMMA, PERIOD
			if (28==i)
				speckeys[2*ii]->kcm = 77; //AT
			if (29==i)
				speckeys[2*ii]->kcm = 62; //SPACE
			if (30==i)
				speckeys[2*ii]->kcm = 66; //ENTER
			if (30<i && i<41)
				speckeys[2*ii]->kcm = i-31+7; //0-9
			if (41==i)
				speckeys[2*ii]->kcm = 61; //TAB
			if (42<=i && i < 50)
				speckeys[2*ii]->kcm = i-42+68; //GRAVE, MINUS, EQUALS, LEFT/RIGHT_BRACKET, BACKSLASH, SEMICOLON, APOSTROPHE
			if (50==i)
				speckeys[2*ii]->kcm = 17; // STAR
			if (51==i)
				speckeys[2*ii]->kcm = 18; // POUND 
			if (52==i)
				speckeys[2*ii]->kcm = 81; // PLUS
			if (53==i)
				speckeys[2*ii]->kcm = 76; // SLASH
		}
		int l = 0;
		if (2*ii+1 < speckeys.size())
		{
			l = speckeys[2*ii+1]->disp;
			speckeys[2*ii+1]->kcm_sh = true;
			if (i<28)
				speckeys[2*ii+1]->kcm = 29+i; //A-Z, COMMA, PERIOD
			if (28==i)
				speckeys[2*ii+1]->kcm = 77; //AT
			if (29==i)
				speckeys[2*ii+1]->kcm = 62; //SPACE
			if (30==i)
				speckeys[2*ii+1]->kcm = 66; //ENTER
			if (30<i && i<41)
				speckeys[2*ii+1]->kcm = i-31+7; //0-9
			if (41==i)
				speckeys[2*ii+1]->kcm = 61; //TAB
			if (42<=i && i < 50)
				speckeys[2*ii+1]->kcm = i-42+68; //GRAVE, MINUS, EQUALS, LEFT/RIGHT_BRACKET, BACKSLASH, SEMICOLON, APOSTROPHE
			if (50==i)
				speckeys[2*ii+1]->kcm = 17; // STAR
			if (51==i)
				speckeys[2*ii+1]->kcm = 18; // POUND 
			if (52==i)
				speckeys[2*ii+1]->kcm = 81; // PLUS
			if (53==i)
				speckeys[2*ii+1]->kcm = 76; // SLASH
		}
		fprintf(kcm,"%s %d %d\n",KCM_BASE[i],k,l);
		ii++;
	}
	fclose(kcm);
//	if (compile("/android/system/usr/keychars/Webkey.kcm","/android/system/usr/keychars/Webkey.kcm.bin"))
#ifdef MYDEB
	if (compile("/android/dev/Webkey.kcm","/android/dev/Webkey.kcm.bin"))
#else
	if (compile("/dev/Webkey.kcm","/dev/Webkey.kcm.bin"))
#endif
	{
		error("Couldn't compile kcm to kcm.bin\n");
	}
//	printf("kcm.bin is compiled.\n");

	struct input_id uid = {
		0x06,//BUS_VIRTUAL, /* Bus type. */
		1, /* Vendor id. */
		1, /* Product id. */
		1 /* Version id. */
	};
//	uinput_fd = suinput_open("../../../sdcard/Webkey", &uid);
	printf("suinput init...\n");
	pthread_mutex_lock(&uinputmutex);
	usleep(100000);
	uinput_fd = suinput_open("../../../dev/Webkey", &uid);
	usleep(100000);
	pthread_mutex_unlock(&uinputmutex);



	device_specific_buttons.clear();
	fk = fo("keycodes.txt","r");
	if (!fk)
		return;
	char line[256];
	while (fgets(line, sizeof(line)-1, fk) != NULL) 
	{
		if (line[0] && line[strlen(line)-1]==10)
			line[strlen(line)-1] = 0;
		if (line[0] && line[strlen(line)-1]==13)
			line[strlen(line)-1] = 0;
		if (strcmp(line,modelname)==0) //this is the right phone
		{
			while (fgets(line, sizeof(line)-1, fk) != NULL) 
			{
				if (strlen(line) < 3)
					break;
//				printf("PROCESSING %s\n",line);
				if (line[strlen(line)-1]==10)
					line[strlen(line)-1] = 0;
				if (line[strlen(line)-1]==13)
					line[strlen(line)-1] = 0;
				int n = strlen(line);
				int pos[2]; pos[0] = pos[1] = 0;
				int i = 0;
				int j;
				for (;i<n-1;i++)
					if (line[i] == ' ')
					{
						pos[0] = i;
						break;
					}
				for (i++;i<n-1;i++)
					if (line[i] == ' ')
					{
						pos[1] = i;
						break;
					}
				line[pos[0]]=0;
				DEVSPEC q;
				q.dev = device_names[std::string(line+pos[1]+1)];
				q.type = 1;
				q.code = getnum(line+pos[0]+1);
				j = 0;
				while (KEYCODES[j].value)
				{
					if(strcmp(line,KEYCODES[j].literal)==0)
					{
						break;
					}
					j++;
				}
				device_specific_buttons[KEYCODES[j].value] = q;
				
			}
				
			
//		while(fscanf(fk,"%d %d %d\n",&id,&show,&ajax) == 3)
//		{
//			if (id-1 < i)
//			{
//				fastkeys[id-1]->ajax = ajax;
//				fastkeys[id-1]->show = show;
//			}
//		}
		}
		else
		{
			bool b = true;
			while (b && strlen(line)>2)
			{
				b = fgets(line, sizeof(line)-1, fk);
			}
			if (!b)
				break;
		}

	}
	fclose(fk);
}

//from android-vnc-server

static void init_fb(void)
{
        size_t pixels;

        pixels = scrinfo.xres_virtual * scrinfo.yres_virtual;
//        pixels = scrinfo.xres * scrinfo.yres;
        bytespp = scrinfo.bits_per_pixel / 8;
	if (bytespp == 3)
		bytespp = 4;

        printf("xres=%d, yres=%d, xresv=%d, yresv=%d, xoffs=%d, yoffs=%d, bpp=%d\n",
          (int)scrinfo.xres, (int)scrinfo.yres,
          (int)scrinfo.xres_virtual, (int)scrinfo.yres_virtual,
          (int)scrinfo.xoffset, (int)scrinfo.yoffset,
          (int)scrinfo.bits_per_pixel);

	if (pixels < scrinfo.xres_virtual*scrinfo.yoffset+scrinfo.xoffset+scrinfo.xres_virtual*scrinfo.yres )//for Droid
		pixels = scrinfo.xres_virtual*scrinfo.yoffset+scrinfo.xoffset+scrinfo.xres_virtual*scrinfo.yres;


	//TEMP
	//pixels = 822272;
	//TEMP

	lowest_offset = scrinfo.xres_virtual*scrinfo.yoffset+scrinfo.xoffset;

        fbmmap = mmap(NULL, pixels * bytespp, PROT_READ, MAP_SHARED, fbfd, 0);
	fbmmapsize = pixels * bytespp;

        if (fbmmap == MAP_FAILED)
        {
                perror("mmap");
                exit(EXIT_FAILURE);
        }
	pict  = new png_byte[scrinfo.yres*scrinfo.xres*3];
	lastpic = new int[pixels * bytespp/4/7+1];
	if (scrinfo.yres > scrinfo.xres)	//orientation might be changed
		graph = new png_byte*[scrinfo.yres];
	else
		graph = new png_byte*[scrinfo.xres];
	copyfb = new unsigned int[scrinfo.yres*scrinfo.xres_virtual*bytespp/4];
}

void init_touch()
{
	int i;
#ifdef MYDEB
	char touch_device[26] = "/android/dev/input/event0";
#else
	char touch_device[18] = "/dev/input/event0";
#endif
	for (i=0; i<10; i++)
	{
		char name[256]="Unknown";
		touch_device[sizeof(touch_device)-2] = '0'+(char)i;
		struct input_absinfo info;
		if((touchfd = open(touch_device, O_RDWR)) == -1)
		{
			continue;
		}
		printf("searching for touch device, opening %s ... ",touch_device);
		if (ioctl(touchfd, EVIOCGNAME(sizeof(name)),name) < 0)
		{
			printf("failed, no name\n");
			close(touchfd);
			touchfd = -1;
			continue;
		}
		device_names[std::string(name)] = i;
		printf("%s ",name);
		if (contains(name,"touchscreen"))
		{
			printf("There is touchscreen in its name, it must be the right device!\n");
		}
		else
		{
			printf("\n");
			continue;
		}
		// Get the Range of X and Y
		if(ioctl(touchfd, EVIOCGABS(ABS_X), &info))
		{
			printf("failed, no ABS_X\n");
			close(touchfd);
			touchfd = -1;
			continue;
		}
		xmin = info.minimum;
		xmax = info.maximum;
		if (xmin == 0 && xmax == 0)
		{
			if(ioctl(touchfd, EVIOCGABS(53), &info))
			{
				printf("failed, no ABS_X\n");
				close(touchfd);
				touchfd = -1;
				continue;
			}
			xmin = info.minimum;
			xmax = info.maximum;
		}

		if(ioctl(touchfd, EVIOCGABS(ABS_Y), &info)) {
			printf("failed, no ABS_Y\n");
			close(touchfd);
			touchfd = -1;
			continue;
		}
		ymin = info.minimum;
		ymax = info.maximum;
		if (ymin == 0 && ymax == 0)
		{
			if(ioctl(touchfd, EVIOCGABS(54), &info))
			{
				printf("failed, no ABS_Y\n");
				close(touchfd);
				touchfd = -1;
				continue;
			}
			ymin = info.minimum;
			ymax = info.maximum;
		}
		if (xmin < 0 || xmin == xmax)	// xmin < 0 for the compass
		{
			printf("failed, xmin<0 || xmin==xmax\n");
			close(touchfd);
			touchfd = -1;
			continue;
		}
		printf("success\n");
		__android_log_print(ANDROID_LOG_INFO,"Webkey C++","using touch device: %s",touch_device);
		printf("xmin = %d, xmax = %d, ymin = %d, ymax = %d\n",xmin,xmax,ymin,ymax);
		return;
	}
	for (i=0; i<10; i++)
	{
		char name[256]="Unknown";
		touch_device[sizeof(touch_device)-2] = '0'+(char)i;
		struct input_absinfo info;
		if((touchfd = open(touch_device, O_RDWR)) == -1)
		{
			continue;
		}
		printf("searching for touch device, opening %s ... ",touch_device);
		if (ioctl(touchfd, EVIOCGNAME(sizeof(name)),name) < 0)
		{
			printf("failed, no name\n");
			close(touchfd);
			touchfd = -1;
			continue;
		}
		printf("device name is %s\n",name);
		// Get the Range of X and Y
		if(ioctl(touchfd, EVIOCGABS(ABS_X), &info))
		{
			printf("failed, no ABS_X\n");
			close(touchfd);
			touchfd = -1;
			continue;
		}
		xmin = info.minimum;
		xmax = info.maximum;
		if (xmin == 0 && xmax == 0)
		{
			if(ioctl(touchfd, EVIOCGABS(53), &info))
			{
				printf("failed, no ABS_X\n");
				close(touchfd);
				touchfd = -1;
				continue;
			}
			xmin = info.minimum;
			xmax = info.maximum;
		}

		if(ioctl(touchfd, EVIOCGABS(ABS_Y), &info)) {
			printf("failed, no ABS_Y\n");
			close(touchfd);
			touchfd = -1;
			continue;
		}
		ymin = info.minimum;
		ymax = info.maximum;
		if (ymin == 0 && ymax == 0)
		{
			if(ioctl(touchfd, EVIOCGABS(54), &info))
			{
				printf("failed, no ABS_Y\n");
				close(touchfd);
				touchfd = -1;
				continue;
			}
			ymin = info.minimum;
			ymax = info.maximum;
		}
		bool t = contains(name,"touch");
		bool tk = contains(name,"touchkey");
		if (t && !tk)
			printf("there is \"touch\", but not \"touchkey\" in the name\n");
		if (!(t && !tk) && (xmin < 0 || xmin == xmax))	// xmin < 0 for the compass
		{
			printf("failed, xmin<0 || xmin==xmax\n");
			close(touchfd);
			touchfd = -1;
			continue;
		}
		printf("success\n");
		__android_log_print(ANDROID_LOG_INFO,"Webkey C++","using touch device: %s",touch_device);
		printf("xmin = %d, xmax = %d, ymin = %d, ymax = %d\n",xmin,xmax,ymin,ymax);
		return;
	}
}

static void savechat()
{
	FILE* f = fo("chat.txt","w");
	int i;
	for (i = 0; i < chat_messages.size(); i++)
		fprintf(f,"%s%c%d%c%s%c",chat_messages[i].user.c_str(),0,chat_messages[i].timestamp,0,chat_messages[i].message.c_str(),0);
	fprintf(f,"%c",0);
	for (std::map<std::string,int>::iterator it = chat_readby.begin(); it != chat_readby.end(); it++)
		fprintf(f,"%s%c%d%c",it->first.c_str(),0,it->second,0);
	fprintf(f,"%c",0);
	fclose(f);
}

static void loadchat()
{
	chat_messages.clear();
	chat_readby.clear();
	chat_readby_real.clear();
	std::string longname = dir + "chat.txt";
	FILE* f = fopen(longname.c_str(),"r");
	if (!f)
		return;
	int i;

	fseek (f , 0 , SEEK_END);
	int lSize = ftell (f);
	rewind (f);
	char* buff = new char[lSize+1];
	if (!buff)
	{
		fclose(f);
		return;
	}
	fread(buff,1,lSize,f);

	i = 0;
	while(buff[i] && i < lSize)
	{
		MESSAGE toload;
		toload.user = buff+i;
		while (buff[i] && i < lSize)
			i++;
		if (i < lSize)
			i++;
		toload.timestamp = getnum(buff+i);
		while (buff[i] && i < lSize)
			i++;
		if (i < lSize)
			i++;
		toload.message = buff+i;
		while (buff[i] && i < lSize)
			i++;
		if (i < lSize)
			i++;
		chat_messages.push_back(toload);
	}
	if (i < lSize)
		i++;
	while(buff[i] && i < lSize)
	{
		std::string q;
		int w;
		q = buff+i;
		while (buff[i] && i < lSize)
			i++;
		if (i < lSize)
			i++;
		w = getnum(buff+i);
		while (buff[i] && i < lSize)
			i++;
		if (i < lSize)
			i++;
		chat_readby[q] = w;
		chat_readby_real[q] = w;
	}

	fclose(f);
}

static void
getchatmessage(struct mg_connection *conn,
                const struct mg_request_info *ri, void *data)
{
	if (ri->permissions != PERM_ROOT && (ri->permissions&PERM_CHAT)==0 && ri->remote_ip!=2130706433)
		return;
	if (exit_flag)
		return;
	int n = strlen(ri->uri);
	int i = 0;
	while (i<n && ri->uri[i] != '_') i++;
	i++;
	int random = getnum(ri->uri+i);
	int pos = -1;
	while (i<n && ri->uri[i] != '_') i++;
	i++;
	if (i < n)       	
		pos = getnum(ri->uri+i);
	int countpos = -1;
	while (i<n && ri->uri[i] != '_') i++;
	i++;
	if (i < n)       	
		countpos = getnum(ri->uri+i);
	std::string username;
	if (strcmp(ri->remote_user,"JAVA_CLIENT")==0)
		username = "phone";
	else
		username = ri->remote_user;
	pthread_mutex_lock(&chatmutex);
	int count = chat_readby[username];
	int count_real = chat_readby_real[username];
	if (random != chat_random)
	{
		count = 0;
		count_real = 0;
	}
	if (count != count_real)
	{
		chat_count++;
		chat_readby[username] = count_real;
		count = count_real;
		pthread_cond_broadcast(&chatcond);
		savechat();
	}
	if (pos == -1)
		pos = count;
	if (countpos == -1)
		countpos = chat_readby_count[username];
	if (chat_messages.size() == pos && random == chat_random && chat_count == countpos)
	{
		if (!exit_flag)
			pthread_cond_wait(&chatcond,&chatmutex);
	}
	if (random != chat_random)
		pos = 0;
	char buff[LINESIZE];
	std::map<int,int> sent;
	send_ok(conn,"Content-Type: text/xml; charset=UTF-8");
	mg_printf(conn,"<chat><readbypos>%d</readbypos><id>%d</id>\n",chat_count,chat_random);
	for (i = pos; i < chat_messages.size(); i++)
	{
		MESSAGE* m = &(chat_messages[i]);
		mg_printf(conn,"<message>");
		mg_printf(conn,"<user>%s</user>",m->user.c_str());
		mg_printf(conn,"<time>%d</time>",m->timestamp);
		convertxml(buff,m->message.c_str());
		mg_printf(conn,"<data>%s</data>",buff);
		mg_printf(conn,"</message>\n");
	}
	if (pos < chat_messages.size())
	{
		//chat_count++;
		chat_readby_real[username] = chat_messages.size();
		//pthread_cond_broadcast(&chatcond);
		//savechat();
	}
	chat_readby_count[username] = chat_count;
	for (std::map<std::string,int>::iterator it = chat_readby.begin(); it != chat_readby.end(); it++)
	{
		if (it->first == username && username == "phone")
			continue;
		convertxml(buff,it->first.c_str());
		mg_printf(conn,"<readby name=\"%s\">%d</readby>",buff,it->second);
	}
	mg_printf(conn,"</chat>\n");
	pthread_mutex_unlock(&chatmutex);
}
static void
phonegetchatmessage(struct mg_connection *conn,
                const struct mg_request_info *ri, void *data)
{
	if (ri->remote_ip==2130706433)
		getchatmessage(conn,ri,data);
}
static void
writechatmessage(struct mg_connection *conn,
                const struct mg_request_info *ri, void *data)
{
	if (ri->permissions != PERM_ROOT && (ri->permissions&PERM_CHAT)==0 && ri->remote_ip!=2130706433)
		return;
	lock_wakelock();
	char* post_data;
	int post_data_len;
	read_post_data(conn,ri,&post_data,&post_data_len);
	send_ok(conn);
	if (post_data_len == 0)
		return;
	MESSAGE load;
	if (strcmp(ri->remote_user,"JAVA_CLIENT")!=0)
	{
		load.user = ri->remote_user;
	}
	else
	{
		load.user = "phone";
	}
	load.timestamp = time(NULL);
	load.message = post_data;
	pthread_mutex_lock(&chatmutex);
	chat_messages.push_back(load);
	savechat();
	pthread_cond_broadcast(&chatcond);
	pthread_mutex_unlock(&chatmutex);
	delete[] post_data;
	usleep(500000);
	pthread_mutex_lock(&chatmutex);
	bool intent = false;
	if (chat_messages.size() != chat_readby["phone"])
		intent = true;
	pthread_mutex_unlock(&chatmutex);
	if (intent)
		syst("/system/bin/am broadcast -a \"webkey.intent.action.Chat\" -n \"com.webkey/.ChatReceiver\"");
}
static void
phonewritechatmessage(struct mg_connection *conn,
                const struct mg_request_info *ri, void *data)
{
	if (ri->remote_ip==2130706433)
		writechatmessage(conn,ri,data);
}
static void
clearchatmessage(struct mg_connection *conn,
                const struct mg_request_info *ri, void *data)
{
	if (ri->permissions != PERM_ROOT && (ri->permissions&PERM_CHAT)==0 && ri->remote_ip!=2130706433)
		return;
	lock_wakelock();
//	MESSAGE load;
//	if (ri->remote_user)
//		load.user = ri->remote_user;
//	else
//		load.user = "phone";
//	load.timestamp = time(NULL);
//	load.message = "clear";
	pthread_mutex_lock(&chatmutex);
	chat_messages.clear();
	chat_readby.clear();
	chat_readby_real.clear();
	chat_readby_count.clear();
	struct timeval tv;
	gettimeofday(&tv,0);
	chat_random = time(NULL) + tv.tv_usec;
	chat_count = 1;
//	chat_messages.push_back(load);
	savechat();
	pthread_cond_broadcast(&chatcond);
	pthread_mutex_unlock(&chatmutex);
	send_ok(conn);
}
static void
phoneclearchatmessage(struct mg_connection *conn,
                const struct mg_request_info *ri, void *data)
{
	if (ri->remote_ip==2130706433)
		clearchatmessage(conn,ri,data);
}

static void
adjust_light(struct mg_connection *conn,
                const struct mg_request_info *ri, void *data)
{
    if (ri->permissions != PERM_ROOT)
	    return;
    lock_wakelock();
    send_ok(conn);
    
    int i = getnum(ri->uri+14);
    int fd = open("/sys/class/leds/lcd-backlight/brightness", O_WRONLY);
    if(fd < 0)
        return;

    char value[20];
    int n = sprintf(value, "%d\n", i*max_brightness/256);
    write(fd, value, n);
    close(fd);
}
static void
waitdiff(struct mg_connection *conn,
                const struct mg_request_info *ri, void *data)
{
	if (ri->permissions != PERM_ROOT && (ri->permissions&PERM_SCREENSHOT)==0)
		return;
	send_ok(conn);
//	printf("connected\n");
	if (!picchanged)
	{
//	printf("start\n");
		pthread_mutex_lock(&diffmutex);
		if (!exit_flag)
			pthread_cond_wait(&diffcond,&diffmutex);
		pthread_mutex_unlock(&diffmutex);
//	printf("end\n");
	}
//	printf("respond\n");
	mg_printf(conn,"changed");
}
void update_image(int orient,int lowres, bool png, bool flip)
{
        if (ioctl(fbfd, FBIOGET_VSCREENINFO, &scrinfo) != 0)
	{
		FILE* fp;
		if (png)
			fp = fo("tmp.png", "wb");
		else
			fp = fo("tmp.jpg", "wb");
		fclose(fp);
		return;
	}
	if (scrinfo.xres_virtual == 240) //for x10 mini pro
	       scrinfo.xres_virtual = 256;
	if (force_240)
		scrinfo.xres_virtual = scrinfo.xres = 240;
//	{
//	int i;
//	for (i = 0; i < sizeof(fb_var_screeninfo); i++)
//	{
//		printf("%d, ",((char*)&scrinfo)[i]);	
//	}
//	printf("\n");
//	}

	FILE            *fp;
	png_structp     png_ptr;
	png_infop       info_ptr;
	struct jpeg_compress_struct cinfo;
	struct jpeg_error_mgr jerr;

	std::string path = "tmp";
	bool rec = false;
	if (recording)
	{
		rec = true;
		char num[32];
		path = std::string("/sdcard/webkey_TEMP/screenshot_");
		itoa(num,recordingnum++);
		int l = strlen(num);
		if (l<2)
			path = path+"0";
		if (l<3)
			path = path+"0";
		path = path + num;
	}

	if (png)
	{
		path = path + ".png";
		if (rec)
			fp = fopen(path.c_str(), "wb");
		else
			fp = fo(path.c_str(), "wb");
		if (!fp)
			return;
		png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
		info_ptr = png_create_info_struct(png_ptr);
		png_init_io(png_ptr, fp);
		if (rec)
			png_set_compression_level(png_ptr, 0);
		else
			png_set_compression_level(png_ptr, 1);
		if (orient == 0)
			png_set_IHDR(png_ptr, info_ptr, scrinfo.xres>>lowres, scrinfo.yres>>lowres,
				8,//  scrinfo.bits_per_pixel,
				PNG_COLOR_TYPE_RGB, PNG_INTERLACE_NONE,
				PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);
		else
			png_set_IHDR(png_ptr, info_ptr, scrinfo.yres>>lowres, scrinfo.xres>>lowres,
				8,//  scrinfo.bits_per_pixel,
				PNG_COLOR_TYPE_RGB, PNG_INTERLACE_NONE,
				PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);
	}
	else
	{
		path = path + ".jpg";
		if (rec)
			fp = fopen(path.c_str(), "wb");
		else
			fp = fo(path.c_str(), "wb");
		if (!fp)
			return;
		JSAMPROW row_pointer[1];
		cinfo.err = jpeg_std_error( &jerr );
		jpeg_create_compress(&cinfo);
		jpeg_stdio_dest(&cinfo, fp);
		if (orient == 0)
		{
			cinfo.image_width = scrinfo.xres>>lowres;
			cinfo.image_height = scrinfo.yres>>lowres;
		}
		else
		{
			cinfo.image_width = scrinfo.yres>>lowres;
			cinfo.image_height = scrinfo.xres>>lowres;
		}
		cinfo.input_components = 3;
		cinfo.in_color_space = JCS_RGB;
		jpeg_set_defaults( &cinfo );
		jpeg_set_quality(&cinfo, 25, TRUE);
		jpeg_start_compress( &cinfo, TRUE );
	}



	int rr = scrinfo.red.offset;
	int rl = 8-scrinfo.red.length;
	int gr = scrinfo.green.offset;
	int gl = 8-scrinfo.green.length;
	int br = scrinfo.blue.offset;
	int bl = 8-scrinfo.blue.length;
	int j;
//	printf("rr=%d, rl=%d, gr=%d, gl=%d, br=%d, bl=%d\n",rr,rl,gr,gl,br,bl);
	

	int x = scrinfo.xres_virtual;
	int xpic = scrinfo.xres;
	int y = scrinfo.yres;
	int m = scrinfo.yres*scrinfo.xres_virtual;
	int mpic = scrinfo.yres*scrinfo.xres;
	int k;
	int i = 0;
	int p = 0;
	int offset = scrinfo.yoffset*scrinfo.xres_virtual+scrinfo.xoffset;
	if (lowest_offset > offset)
		lowest_offset = offset;
	if (firstfb)
		offset = lowest_offset;



//	int poffset = (scrinfo.yoffset*x+scrinfo.xoffset);
//	printf("%d\n",poffset);
//	int p = poffset;
//	int i = 0;

	memcpy(copyfb,(char*)fbmmap+offset*bytespp,m*bytespp);
	offset=0;
	{
		int s = m*bytespp/4/157;
		unsigned int* map = ((unsigned int*)copyfb)+(offset*bytespp/4);
		for (i=0; i < s; i++)
		{
			lastpic[22*i+0] = map[157*i+0];
			lastpic[22*i+1] = map[157*i+7];
			lastpic[22*i+2] = map[157*i+14];
			lastpic[22*i+3] = map[157*i+21];
			lastpic[22*i+4] = map[157*i+28];
			lastpic[22*i+5] = map[157*i+35];
			lastpic[22*i+6] = map[157*i+42];
			lastpic[22*i+7] = map[157*i+49];
			lastpic[22*i+8] = map[157*i+56];
			lastpic[22*i+9] = map[157*i+63];
			lastpic[22*i+10] = map[157*i+70];
			lastpic[22*i+11] = map[157*i+77];
			lastpic[22*i+12] = map[157*i+84];
			lastpic[22*i+13] = map[157*i+91];
			lastpic[22*i+14] = map[157*i+98];
			lastpic[22*i+15] = map[157*i+105];
			lastpic[22*i+16] = map[157*i+112];
			lastpic[22*i+17] = map[157*i+119];
			lastpic[22*i+18] = map[157*i+126];
			lastpic[22*i+19] = map[157*i+133];
			lastpic[22*i+20] = map[157*i+140];
			lastpic[22*i+21] = map[157*i+147];
		}
		picchanged = false;
	}
	if (flip == false)
	{
		i=0;
		if (lowres) // I use if outside the for, maybe it's faster this way
		{
			int rm = 0; for (k = 0; k < scrinfo.red.length; k++) rm = rm*2+1;
			int gm = 0; for (k = 0; k < scrinfo.green.length; k++) gm = gm*2+1;
			int bm = 0; for (k = 0; k < scrinfo.blue.length; k++) bm = bm*2+1;
			if (bytespp == 2) //16 bit
			{
				rl -= 2;
				gl -= 2;
				bl -= 2;
				unsigned short int* map = ((unsigned short int*)copyfb)+offset;
				if (orient == 0) //vertical
				{
					for (j = 0; j < y; j+=2)
					{
						for (k = 0; k < xpic; k+=2)
						{
							pict[i++] = (((map[p]>>rr)&rm)+((map[p+1]>>rr)&rm)+((map[p+x]>>rr)&rm)+((map[p+x+1]>>rr)&rm))<<rl;
							pict[i++] = (((map[p]>>gr)&gm)+((map[p+1]>>gr)&gm)+((map[p+x]>>gr)&gm)+((map[p+x+1]>>gr)&gm))<<gl;
							pict[i++] = (((map[p]>>br)&bm)+((map[p+1]>>br)&bm)+((map[p+x]>>br)&bm)+((map[p+x+1]>>br)&bm))<<bl;
							p+=2;
						}
						p+=x-xpic;
						p+=x;
					}
				}
				else //horizontal
				{
					for (j = 0; j < xpic; j+=2)
					{
						p = (xpic-j-1);//+poffset;
						for (k = 0; k < y; k+=2)
						{
							pict[i++] = (((map[p]>>rr)&rm)+((map[p-1]>>rr)&rm)+((map[p+x]>>rr)&rm)+((map[p+x-1]>>rr)&rm))<<rl;
							pict[i++] = (((map[p]>>gr)&gm)+((map[p-1]>>gr)&gm)+((map[p+x]>>gr)&gm)+((map[p+x-1]>>gr)&gm))<<gl;
							pict[i++] = (((map[p]>>br)&bm)+((map[p-1]>>br)&bm)+((map[p+x]>>br)&bm)+((map[p+x-1]>>br)&bm))<<bl;
							p += 2*x;
						}
					}
				}
			}
			if (bytespp == 4) //32 bit
			{
				unsigned int* map = ((unsigned int*)copyfb)+offset;
				if (orient == 0) //vertical
				{
					for (j = 0; j < y; j+=2)
					{
						for (k = 0; k < xpic; k+=2)
						{
							pict[i++] = (((map[p]>>rr)&rm)+((map[p+1]>>rr)&rm)+((map[p+x]>>rr)&rm)+((map[p+x+1]>>rr)&rm))>>2<<rl;
							pict[i++] = (((map[p]>>gr)&gm)+((map[p+1]>>gr)&gm)+((map[p+x]>>gr)&gm)+((map[p+x+1]>>gr)&gm))>>2<<gl;
							pict[i++] = (((map[p]>>br)&bm)+((map[p+1]>>br)&bm)+((map[p+x]>>br)&bm)+((map[p+x+1]>>br)&bm))>>2<<bl;
							p+=2;
						}
						p+=x-xpic;
						p+=x;
					}
				}
				else //horizontal
				{
					for (j = 0; j < xpic; j+=2)
					{
						p = (xpic-j-1);// + poffset;
						for (k = 0; k < y; k+=2)
						{
							pict[i++] = (((map[p]>>rr)&rm)+((map[p-1]>>rr)&rm)+((map[p+x]>>rr)&rm)+((map[p+x-1]>>rr)&rm))>>2<<rl;
							pict[i++] = (((map[p]>>gr)&gm)+((map[p-1]>>gr)&gm)+((map[p+x]>>gr)&gm)+((map[p+x-1]>>gr)&gm))>>2<<gl;
							pict[i++] = (((map[p]>>br)&bm)+((map[p-1]>>br)&bm)+((map[p+x]>>br)&bm)+((map[p+x-1]>>br)&bm))>>2<<bl;
//							pict[i++] = ((map[p]&255)+(map[p-1]&255)+(map[p+x]&255)+(map[p+x-1]&255))>>2;
//							pict[i++] = (((map[p]>>8)&255)+((map[p-1]>>8)&255)+((map[p+x]>>8)&255)+((map[p+x-1]>>8)&255))>>2;
//							pict[i++] = (((map[p]>>16)&255)+((map[p-1]>>16)&255)+((map[p+x]>>16)&255)+((map[p+x-1]>>16)&255))>>2;
							p += 2*x;
						}
					}
				}
			}
		}
		else //hires
		{
			if (bytespp == 2)
			{
				unsigned short int* map = ((unsigned short int*)copyfb)+offset;
				if (orient == 0) //vertical
				{
					for (j = 0; j < y; j++)
					{
						for (k = 0; k < xpic; k++)
						{
							pict[i++] = map[p]>>rr<<rl;
							pict[i++] = map[p]>>gr<<gl;
							pict[i++] = map[p++]>>br<<bl;
						}
						p+=x-xpic;
					}
				}
				else //horizontal
				{
					for (j = 0; j < xpic; j++)
					{
						p = (xpic-j-1);// + poffset;
						for (k = 0; k < y; k++)
						{
							pict[i++] = map[p]>>rr<<rl;
							pict[i++] = map[p]>>gr<<gl;
							pict[i++] = map[p]>>br<<bl;
							p += x;
						}
					}
				}
			}
			if (bytespp == 4)
			{
				unsigned int* map = ((unsigned int*)copyfb)+offset;
				if (orient == 0) //vertical
				{
					for (j = 0; j < y; j++)
					{
						for (k = 0; k < xpic; k++)
						{
							pict[i++] = map[p]>>rr<<rl;
							pict[i++] = map[p]>>gr<<gl;
							pict[i++] = map[p++]>>br<<bl;
						}
						p+=x-xpic;
					}
				}
				else //horizontal
				{
					for (j = 0; j < xpic; j++)
					{
						p = (xpic-j-1);// + poffset;
						for (k = 0; k < y; k++)
						{
							pict[i++] = map[p]>>rr<<rl;
							pict[i++] = map[p]>>gr<<gl;
							pict[i++] = map[p]>>br<<bl;
							p += x;
						}
					}
				}
			}
		}
	}
	else //flip == true
	{
		i=3*(scrinfo.xres>>lowres)*(scrinfo.yres>>lowres)-3;
		if (lowres) // I use if outside the for, maybe it's faster this way
		{
			int rm = 0; for (k = 0; k < scrinfo.red.length; k++) rm = rm*2+1;
			int gm = 0; for (k = 0; k < scrinfo.green.length; k++) gm = gm*2+1;
			int bm = 0; for (k = 0; k < scrinfo.blue.length; k++) bm = bm*2+1;
			if (bytespp == 2) //16 bit
			{
				rl -= 2;
				gl -= 2;
				bl -= 2;
				unsigned short int* map = ((unsigned short int*)copyfb)+offset;
				if (orient == 0) //vertical
				{
					for (j = 0; j < y; j+=2)
					{
						for (k = 0; k < xpic; k+=2)
						{
							pict[i++] = (((map[p]>>rr)&rm)+((map[p+1]>>rr)&rm)+((map[p+x]>>rr)&rm)+((map[p+x+1]>>rr)&rm))<<rl;
							pict[i++] = (((map[p]>>gr)&gm)+((map[p+1]>>gr)&gm)+((map[p+x]>>gr)&gm)+((map[p+x+1]>>gr)&gm))<<gl;
							pict[i++] = (((map[p]>>br)&bm)+((map[p+1]>>br)&bm)+((map[p+x]>>br)&bm)+((map[p+x+1]>>br)&bm))<<bl;
							i -= 6;
							p+=2;
						}
						p+=x-xpic;
						p+=x;
					}
				}
				else //horizontal
				{
					for (j = 0; j < xpic; j+=2)
					{
						p = (xpic-j-1);//+poffset;
						for (k = 0; k < y; k+=2)
						{
							pict[i++] = (((map[p]>>rr)&rm)+((map[p-1]>>rr)&rm)+((map[p+x]>>rr)&rm)+((map[p+x-1]>>rr)&rm))<<rl;
							pict[i++] = (((map[p]>>gr)&gm)+((map[p-1]>>gr)&gm)+((map[p+x]>>gr)&gm)+((map[p+x-1]>>gr)&gm))<<gl;
							pict[i++] = (((map[p]>>br)&bm)+((map[p-1]>>br)&bm)+((map[p+x]>>br)&bm)+((map[p+x-1]>>br)&bm))<<bl;
							i -= 6;
							p += 2*x;
						}
					}
				}
			}
			if (bytespp == 4) //32 bit
			{
				unsigned int* map = ((unsigned int*)copyfb)+offset;
				// I assume that each color have 1 byte.
				if (orient == 0) //vertical
				{
					for (j = 0; j < y; j+=2)
					{
						for (k = 0; k < xpic; k+=2)
						{
							pict[i++] = (((map[p]>>rr)&rm)+((map[p+1]>>rr)&rm)+((map[p+x]>>rr)&rm)+((map[p+x+1]>>rr)&rm))>>2<<rl;
							pict[i++] = (((map[p]>>gr)&gm)+((map[p+1]>>gr)&gm)+((map[p+x]>>gr)&gm)+((map[p+x+1]>>gr)&gm))>>2<<gl;
							pict[i++] = (((map[p]>>br)&bm)+((map[p+1]>>br)&bm)+((map[p+x]>>br)&bm)+((map[p+x+1]>>br)&bm))>>2<<bl;
//							pict[i++] = ((map[p]&255)+(map[p+1]&255)+(map[p+x]&255)+(map[p+x+1]&255))>>2;
//							pict[i++] = (((map[p]>>8)&255)+((map[p+1]>>8)&255)+((map[p+x]>>8)&255)+((map[p+x+1]>>8)&255))>>2;
//							pict[i++] = (((map[p]>>16)&255)+((map[p+1]>>16)&255)+((map[p+x]>>16)&255)+((map[p+x+1]>>16)&255))>>2;
							i -= 6;
							p+=2;
						}
						p+=x-xpic;
						p+=x;
					}
				}
				else //horizontal
				{
				// I assume that each color have 1 byte.
					for (j = 0; j < xpic; j+=2)
					{
						p = (xpic-j-1);// + poffset;
						for (k = 0; k < y; k+=2)
						{
							pict[i++] = (((map[p]>>rr)&rm)+((map[p-1]>>rr)&rm)+((map[p+x]>>rr)&rm)+((map[p+x-1]>>rr)&rm))>>2<<rl;
							pict[i++] = (((map[p]>>gr)&gm)+((map[p-1]>>gr)&gm)+((map[p+x]>>gr)&gm)+((map[p+x-1]>>gr)&gm))>>2<<gl;
							pict[i++] = (((map[p]>>br)&bm)+((map[p-1]>>br)&bm)+((map[p+x]>>br)&bm)+((map[p+x-1]>>br)&bm))>>2<<bl;
//							pict[i++] = ((map[p]&255)+(map[p-1]&255)+(map[p+x]&255)+(map[p+x-1]&255))>>2;
//							pict[i++] = (((map[p]>>8)&255)+((map[p-1]>>8)&255)+((map[p+x]>>8)&255)+((map[p+x-1]>>8)&255))>>2;
//							pict[i++] = (((map[p]>>16)&255)+((map[p-1]>>16)&255)+((map[p+x]>>16)&255)+((map[p+x-1]>>16)&255))>>2;
							i -= 6;
							p += 2*x;
						}
					}
				}
			}
		}
		else //hires
		{
			if (bytespp == 2)
			{
				unsigned short int* map = ((unsigned short int*)copyfb)+offset;
				if (orient == 0) //vertical
				{
					for (j = 0; j < y; j++)
					{
						for (k = 0; k < xpic; k++)
						{
							pict[i++] = map[p]>>rr<<rl;
							pict[i++] = map[p]>>gr<<gl;
							pict[i++] = map[p++]>>br<<bl;
							i -= 6;
						}
						p+=x-xpic;
					}
				}
				else //horizontal
				{
					for (j = 0; j < xpic; j++)
					{
						p = (xpic-j-1);// + poffset;
						for (k = 0; k < y; k++)
						{
							pict[i++] = map[p]>>rr<<rl;
							pict[i++] = map[p]>>gr<<gl;
							pict[i++] = map[p]>>br<<bl;
							i -= 6;
							p += x;
						}
					}
				}
			}
			if (bytespp == 4)
			{
				unsigned int* map = ((unsigned int*)copyfb)+offset;
				if (orient == 0) //vertical
				{
					for (j = 0; j < y; j++)
					{
						for (k = 0; k < xpic; k++)
						{
							pict[i++] = map[p]>>rr<<rl;
							pict[i++] = map[p]>>gr<<gl;
							pict[i++] = map[p++]>>br<<bl;
							i -= 6;
						}
						p+=x-xpic;
					}
				}
				else //horizontal
				{
					for (j = 0; j < xpic; j++)
					{
						p = (xpic-j-1);// + poffset;
						for (k = 0; k < y; k++)
						{
							pict[i++] = map[p]>>rr<<rl;
							pict[i++] = map[p]>>gr<<gl;
							pict[i++] = map[p]>>br<<bl;
							i -= 6;
							p += x;
						}
					}
				}
			}
		}
	}

	if (orient == 0)
	{
		for (i = 0; i < y>>lowres; i++)
			graph[i] = pict+(i*xpic*3>>lowres);
	}	
	else
	{
		for (i = 0; i < x>>lowres; i++)
			graph[i] = pict+(i*y*3>>lowres);
	}

	if (png)
	{
		png_write_info(png_ptr, info_ptr);
		png_write_image(png_ptr, graph);
		png_write_end(png_ptr, info_ptr);
		png_destroy_write_struct(&png_ptr, &info_ptr);
	}
	else
	{
		i = 0;
		while( cinfo.next_scanline < cinfo.image_height )
		{
			jpeg_write_scanlines( &cinfo, graph+i++, 1 );
		}
		jpeg_finish_compress( &cinfo );
		jpeg_destroy_compress( &cinfo );
	}
	fclose(fp);
	if (recording)
		recordingnumfinished = recordingnum-1;
	else
		recordingnumfinished = -1;

//	pthread_mutex_lock(&diffmutex);
//	pthread_cond_broadcast(&diffstartcond);
//	pthread_mutex_unlock(&diffmutex);
}

static
void* watchscreen(void* param)
{
	while (1)
	{
		if (exit_flag)
			return NULL;
		pthread_mutex_lock(&diffmutex);
		pthread_cond_wait(&diffstartcond,&diffmutex);
		pthread_mutex_unlock(&diffmutex);
		int l = 0;
		int i;
		//picchanged = false;
		while(1)
		{
//			printf("%d\n",l);
			if (ioctl(fbfd, FBIOGET_VSCREENINFO, &scrinfo) != 0)
			{
				continue;
			}
			if (scrinfo.xres_virtual == 240) //for x10 mini pro
				scrinfo.xres_virtual = 256;
			if (force_240)
				scrinfo.xres_virtual = scrinfo.xres = 240;
			int m = scrinfo.yres*scrinfo.xres;
			int offset = scrinfo.yoffset*scrinfo.xres_virtual+scrinfo.xoffset;
			if (lowest_offset > offset)
				lowest_offset = offset;
			if (firstfb)
				offset = lowest_offset;
			if (pict && lastpic);
			{
				int s = m*bytespp/4/157;
				unsigned int* map = ((unsigned int*)fbmmap)+(offset*bytespp/4);
				int start = 0;
				int end = s;
				if (lastorient == 0)
				{
					if (lastflip)
						end = s*19/20;
					else
						start = s/20;
				}
				int rowlength = scrinfo.xres*bytespp/4;
				for (i=start; i < end; i++)
				{
					if (lastpic[22*i+0] != map[157*i+0] ||
					lastpic[22*i+1] != map[157*i+7] ||
					lastpic[22*i+2] != map[157*i+14] ||
					lastpic[22*i+3] != map[157*i+21] ||
					lastpic[22*i+4] != map[157*i+28] ||
					lastpic[22*i+5] != map[157*i+35] ||
					lastpic[22*i+6] != map[157*i+42] ||
					lastpic[22*i+7] != map[157*i+49] ||
					lastpic[22*i+8] != map[157*i+56] ||
					lastpic[22*i+9] != map[157*i+63] ||
					lastpic[22*i+10] != map[157*i+70] ||
					lastpic[22*i+11] != map[157*i+77] ||
					lastpic[22*i+12] != map[157*i+84] ||
					lastpic[22*i+13] != map[157*i+91] ||
					lastpic[22*i+14] != map[157*i+98] ||
					lastpic[22*i+15] != map[157*i+105] ||
					lastpic[22*i+16] != map[157*i+112] ||
					lastpic[22*i+17] != map[157*i+119] ||
					lastpic[22*i+18] != map[157*i+126] ||
					lastpic[22*i+19] != map[157*i+133] ||
					lastpic[22*i+20] != map[157*i+140] ||
					lastpic[22*i+21] != map[157*i+147])
					{
						//printf("CHANGED %d\n",i);
						picchanged = true;
						pthread_mutex_lock(&diffmutex);
						pthread_cond_broadcast(&diffcond);
						pthread_mutex_unlock(&diffmutex);
						break;
					}
				}
			}
			if (picchanged)
				break;
			l++;
			if (recording)
			{
				if (l > 3000)
					break;
				usleep(10000);
			}
			else
			{
				if (l > 600)
					break;
				usleep(100000);
			}
		}
		if (picchanged == false)
		{
			pthread_mutex_lock(&diffmutex);
			pthread_cond_broadcast(&diffcond);
			pthread_mutex_unlock(&diffmutex);
		}
	}
}

static void
emptyresponse(struct mg_connection *conn,
                const struct mg_request_info *ri, void *data)
{
	send_ok(conn);
}

static void
touch(struct mg_connection *conn,
                const struct mg_request_info *ri, void *data)
{
	if (ri->permissions != PERM_ROOT)
		return;
	lock_wakelock();
	access_log(ri,"touch inject");
	int n = strlen(ri->uri);
       	if (n<8)
		return;
	int orient = 0;
	if (ri->uri[7]=='h')
		orient = 1;
	int i = 8;
	int x = getnum(ri->uri+i);
	while (i<n && ri->uri[i++]!='_');
	int y = getnum(ri->uri+i);
	while (i<n && ri->uri[i++]!='_');
	int down = getnum(ri->uri+i);
	if(touchfd != -1 && scrinfo.xres && scrinfo.yres)
	{
		struct input_event ev;

		if (orient)
		{
			int t = x;
			x = scrinfo.xres-y;
			y = t;
		}
		if (x < 0) x = 0;
		if (y < 0) y = 0;
		if (x > scrinfo.xres) x = scrinfo.xres;
		if (y > scrinfo.yres) y = scrinfo.yres;
		// Calculate the final x and y
		x = xmin + (x * (xmax - xmin)) / (scrinfo.xres);
		y = ymin + (y * (ymax - ymin)) / (scrinfo.yres);

		memset(&ev, 0, sizeof(ev));

		// Then send a BTN_TOUCH
		if (down != 2)
		{
			gettimeofday(&ev.time,0);
			ev.type = EV_KEY; //1
			ev.code = BTN_TOUCH; //330
			ev.value = down;//>0?1:0;
			if(write(touchfd, &ev, sizeof(ev)) < 0)
				printf("touchfd write failed.\n");
		}
		gettimeofday(&ev.time,0);
		ev.type = EV_ABS; //3
		ev.code = 48;
		ev.value = down>0?100:0;
		if(write(touchfd, &ev, sizeof(ev)) < 0)
			printf("touchfd write failed.\n");
		gettimeofday(&ev.time,0);
		ev.type = EV_ABS; //3
		ev.code = 50;
		ev.value = down>0?1:0;
		if(write(touchfd, &ev, sizeof(ev)) < 0)
			printf("touchfd write failed.\n");
		// Then send the X
		gettimeofday(&ev.time,0);
		ev.type = EV_ABS; //3
		ev.code = ABS_X; //0
		ev.value = x;
		if(write(touchfd, &ev, sizeof(ev)) < 0)
			printf("touchfd write failed.\n");
		// Then send the X
		gettimeofday(&ev.time,0);
		ev.type = EV_ABS; //3
		ev.code = 53;//ABS_X;
		ev.value = x;
		if(write(touchfd, &ev, sizeof(ev)) < 0)
			printf("touchfd write failed.\n");

		// Then send the Y
		gettimeofday(&ev.time,0);
		ev.type = EV_ABS; //3
		ev.code = ABS_Y; //1
		ev.value = y;
		if(write(touchfd, &ev, sizeof(ev)) < 0)
			printf("touchfd write failed.\n");
		// Then send the Y
		gettimeofday(&ev.time,0);
		ev.type = EV_ABS; //3
		ev.code = 54;//ABS_Y;
		ev.value = y;
		if(write(touchfd, &ev, sizeof(ev)) < 0)
			printf("touchfd write failed.\n");

		// Finally send the SYN
		gettimeofday(&ev.time,0);
		ev.type = EV_SYN;//0
		ev.code = 2;
		ev.value = 0;
		if(write(touchfd, &ev, sizeof(ev)) < 0)
			printf("touchfd write failed.\n");
		gettimeofday(&ev.time,0);
		ev.type = EV_SYN;//0
		ev.code = 0;
		ev.value = 0;
		if(write(touchfd, &ev, sizeof(ev)) < 0)
			printf("touchfd write failed.\n");

		//printf("%d. injectTouch x=%d, y=%d, down=%d\n", touchcount++, x, y, down);    

	}
	send_ok(conn);
}
static void
key(struct mg_connection *conn,
                const struct mg_request_info *ri, void *data)
{
	if (ri->permissions != PERM_ROOT)
		return;
	lock_wakelock();
	access_log(ri,"key inject");
	send_ok(conn);
	if (uinput_fd == -1)
		return;
	bool sms_mode = false;
	int key = 0;
	bool old = false;
	int orient = 0;
//	printf("%s\n",ri->uri);
		// /oldkey_hn-22 -> -22 key with normal mode, horisontal
	int n = 0;
	char* post_data;
	int post_data_len;
	read_post_data(conn,ri,&post_data,&post_data_len);
	if (post_data_len == 0)
	{
		delete[] post_data;
		if (startswith(ri->uri,"/oldkey") && strlen(ri->uri)>10) 
		{
			n = 9;
			old = true;
		}
		else
		if (strlen(ri->uri)>9)
		{
			n = 9;
		}
		else
			return;
		if (ri->uri[n-2] == 'h')
			orient = 1;
		if (ri->uri[n-1] == 's')
			sms_mode = true;
	}
	else
	{
		for (int i =0;i < post_data_len;i++)
			printf("%d, ",post_data[i]);
		printf("\n");
	}
//	if (ri->uri[++n] == 0)
//		return;
	
	pthread_mutex_lock(&uinputmutex);
	while(1)
	{
		if (post_data_len == 0)
		{
			while (ri->uri[n] != '_')
			{
			       if (ri->uri[++n] == 0)
			       {
				       pthread_mutex_unlock(&uinputmutex);
				       return;
			       }
			}
			if (ri->uri[++n] == 0)
			{
				pthread_mutex_unlock(&uinputmutex);
				return;
			}
			key = getnum(ri->uri+n);
			if (old && key < 0)
				key = -key;
		}
		else
		{
			if (n>=post_data_len)
			{
				pthread_mutex_unlock(&uinputmutex);
				return;
			}
			int q = 0;
			unsigned int w = post_data[n++];
			while (w&128) {	q++; w = w << 1; }
			key = (w&255)>>q;
			printf("%d\n",key);
			while (q>1 && n < post_data_len)
			{
				key = (key<<6)+(post_data[n++]&63);
				q--;
			}
			printf("%d\n",key);
		}
		if (key == 0) // I don't know how, but it happens
		{
			pthread_mutex_unlock(&uinputmutex);
			return;
		}
//		printf("KEY %d\n",key);
		int i;
		int j = 0;
		if (sms_mode)
			for (i=0; i < speckeys.size(); i++)
				if ((speckeys[i]->ajax == key || (old && speckeys[i]->ajax == -key)) && speckeys[i]->sms)
				{
//					printf("pressed: %d\n",speckeys[i]->kcm);
					suinput_press(uinput_fd, 57); //left alt
					if (speckeys[i]->kcm_sh)
						suinput_press(uinput_fd, 59); //left shift
					if (speckeys[i]->kcm)
						suinput_click(uinput_fd, speckeys[i]->kcm);
					suinput_release(uinput_fd, 57); //left alt
					if (speckeys[i]->kcm_sh)
						suinput_release(uinput_fd, 59); //left shift
					j++;
				}
		if (j)
			continue;
		if (!sms_mode || i == speckeys.size())
		{
			for (i=0; i < speckeys.size(); i++)
				if ((speckeys[i]->ajax == key|| (old && speckeys[i]->ajax == -key) ) && speckeys[i]->sms == false)
				{
//					printf("pressed: %d\n",speckeys[i]->kcm);
					suinput_press(uinput_fd, 57); //left alt
					if (speckeys[i]->kcm_sh)
						suinput_press(uinput_fd, 59); //left shift
					if (speckeys[i]->kcm)
						suinput_click(uinput_fd, speckeys[i]->kcm);
					suinput_release(uinput_fd, 57); //left alt
					if (speckeys[i]->kcm_sh)
						suinput_release(uinput_fd, 59); //left shift
					j++;
				}
		}
		if (j)
			continue;
		for (i=0; i < fastkeys.size(); i++)
			if (fastkeys[i]->ajax == key || (old && fastkeys[i]->ajax == -key))
			{
				int k = i+1;
				if (orient)
				{
					if (k == 19) //up -> right
						k = 22;
					else if (k == 20) //down -> left
						k = 21;
					else if (k == 21) //left -> up
						k = 19;
					else if (k == 22) //right -> down
						k = 20;
				}
				suinput_click(uinput_fd, k);
				j++;
			}
		if (j)
			continue;
		if (48<=key && key < 58)
		{
			suinput_click(uinput_fd, key-48+7);
			j++;
		}
		if (97<=key && key < 123)
		{
			suinput_click(uinput_fd, key-97+29);
			j++;
		}
		if (65<=key && key < 91)
		{
			suinput_press(uinput_fd, 59); //left shift
			suinput_click(uinput_fd, key-65+29);
			suinput_release(uinput_fd, 59); //left shift
			j++;
		}
		if (32<=key && key <= 47)
		{
			if (spec1sh[key-32]) suinput_press(uinput_fd, 59); //left shift
			suinput_click(uinput_fd, spec1[key-32]);
			if (spec1sh[key-32]) suinput_release(uinput_fd, 59); //left shift
			j++;
		}
		if (58<=key && key <= 64)
		{
			if (spec2sh[key-58]) suinput_press(uinput_fd, 59); //left shift
			suinput_click(uinput_fd, spec2[key-58]);
			if (spec2sh[key-58]) suinput_release(uinput_fd, 59); //left shift
			j++;
		}
		if (91<=key && key <= 96)
		{
			if (spec3sh[key-91]) suinput_press(uinput_fd, 59); //left shift
			suinput_click(uinput_fd, spec3[key-91]);
			if (spec3sh[key-91]) suinput_release(uinput_fd, 59); //left shift
			j++;
		}
		if (123<=key && key <= 127)
		{
			if (spec4sh[key-123]) suinput_press(uinput_fd, 59); //left shift
			suinput_click(uinput_fd, spec4[key-123]);
			if (spec4sh[key-123]) suinput_release(uinput_fd, 59); //left shift
			j++;
		}
		if (key == -8 || (old && key == 8)) 
		{
			suinput_click(uinput_fd, 67); //BACKSPACE -> DEL
			j++;
		}
		if (key == -13 || (old && key == 13)) 
		{
			suinput_click(uinput_fd, 66); //ENTER
			j++;
		}
		if (j == 0)
		{
			BIND* load = new BIND;
			if (key > 0)
			{
				load->ajax = key;
				load->disp = key;
			}
			else
			{
				load->ajax = -key;
				load->disp = -key;
			}
			load->sms = 0;
			speckeys.push_back(load);
			FILE* sk = fo("spec_keys.txt","w");
			if (!sk)
				continue;
			int nrem = 0;
			if (speckeys.size() > 2*52)
				nrem = speckeys.size() - 2*52;;
			for(i=0; i <speckeys.size(); i++)
				if (speckeys[i]->ajax || speckeys[i]->disp)
				{
					if (nrem == 0 || speckeys[i]->ajax != speckeys[i]->disp || speckeys[i]->sms)
						fprintf(sk,"%d %d %d\n",speckeys[i]->sms,speckeys[i]->ajax,speckeys[i]->disp);
					else
						nrem -= 1;
				}
			fclose(sk);
			pthread_mutex_unlock(&uinputmutex);
			init_uinput();
			pthread_mutex_lock(&uinputmutex);
			for (i=0; i < speckeys.size(); i++)
				if ((speckeys[i]->ajax == key|| (old && speckeys[i]->ajax == -key) ))
				{
//					printf("pressed: %d\n",speckeys[i]->kcm);
					suinput_press(uinput_fd, 57); //left alt
					if (speckeys[i]->kcm_sh)
						suinput_press(uinput_fd, 59); //left shift
					if (speckeys[i]->kcm)
						suinput_click(uinput_fd, speckeys[i]->kcm);
					suinput_release(uinput_fd, 57); //left alt
					if (speckeys[i]->kcm_sh)
						suinput_release(uinput_fd, 59); //left shift
					j++;
				}
			}
		usleep(1000);
	}
	pthread_mutex_unlock(&uinputmutex);

}


static void
savebuttons(struct mg_connection *conn,
                const struct mg_request_info *ri, void *data)
{
	if (ri->permissions != PERM_ROOT)
		return;
	lock_wakelock();
	access_log(ri,"modify buttons");
	char* post_data;
	int post_data_len;
	read_post_data(conn,ri,&post_data,&post_data_len);
	int i;
	for (i=0; i < fastkeys.size(); i++)
		fastkeys[i]->show = false;
	int n = post_data_len;
	i = 0;
	while(i<n)
	{
		if (startswith(post_data+i,"show_"))
		{
			i+=5;
			int id = getnum(post_data+i);
			if (id-1<fastkeys.size())
				fastkeys[id-1]->show = true;
//			printf("SHOW %d\n",id-1);
		}
		else if (startswith(post_data+i,"keycode_"))
		{
			i+=8;
			int id = getnum(post_data+i);
			while (i<n && post_data[i++]!='=');
			int ajax = getnum(post_data+i);
			if (id-1<fastkeys.size())
				fastkeys[id-1]->ajax = ajax;
		}
		while (i<n && post_data[i++]!='&');
	}

	FILE* fk = fo("fast_keys.txt","w");
	if (!fk)
		return;
	for (i=0;i<fastkeys.size();i++)
	{
		fprintf(fk,"%d %d %d\n",i+1,fastkeys[i]->show,fastkeys[i]->ajax);
	}
	fclose(fk);
	mg_printf(conn,"HTTP/1.1 200 OK\r\nCache-Control: no-store, no-cache, must-revalidate\r\nCache-Control: post-check=0, pre-check=0\r\nPragma: no-cache\r\nContent-Type: text/html; charset=utf-8\r\nConnection: close\r\n\r\n<html><head><meta http-equiv=\"refresh\" content=\"0;url=phone.html\"></head><body>redirecting</body></html>");
	if (post_data)
		delete[] post_data;
}
static void
savekeys(struct mg_connection *conn,
                const struct mg_request_info *ri, void *data)
{
	if (ri->permissions != PERM_ROOT)
		return;
	lock_wakelock();
	access_log(ri,"modify keys");
	char* post_data;
	int post_data_len;
	read_post_data(conn,ri,&post_data,&post_data_len);
	int i;
	int n = post_data_len;
	for (i=0; i < speckeys.size(); i++)
		if (speckeys[i])
			delete speckeys[i];
	speckeys.clear();
	for (i=0; i < 106; i++)
	{
		BIND* load = new BIND;
		load->ajax = load->disp = load->sms = 0;
		speckeys.push_back(load);
	}
	i = 0;
	while(i<n)
	{
		if (startswith(post_data+i,"sms_"))
		{
			i+=4;
			int id = getnum(post_data+i);
			if (id-1<speckeys.size())
				speckeys[id-1]->sms = true;
		}
		else if (startswith(post_data+i,"keycode_"))
		{
			i+=8;
			int id = getnum(post_data+i);
			while (i<n && post_data[i++]!='=');
			int ajax = getnum(post_data+i);
			if (id-1<speckeys.size())
				speckeys[id-1]->ajax = ajax;
		}
		else if (startswith(post_data+i,"tokeycode_"))
		{
			i+=10;
			int id = getnum(post_data+i);
			while (i<n && post_data[i++]!='=');
			int disp = getnum(post_data+i);
			if (id-1<speckeys.size())
				speckeys[id-1]->disp = disp;
		}
		while (i<n && post_data[i++]!='&');
	}

	FILE* sk = fo("spec_keys.txt","w");
	if (!sk)
		return;
	for(i=0; i <speckeys.size(); i++)
		if (speckeys[i]->ajax || speckeys[i]->disp || speckeys[i]->disp)
			fprintf(sk,"%d %d %d\n",speckeys[i]->sms,speckeys[i]->ajax,speckeys[i]->disp);
	fclose(sk);
	init_uinput();
	mg_printf(conn,"HTTP/1.1 200 OK\r\nCache-Control: no-store, no-cache, must-revalidate\r\nCache-Control: post-check=0, pre-check=0\r\nPragma: no-cache\r\nContent-Type: text/html; charset=utf-8\r\nConnection: close\r\n\r\n<html><head><meta http-equiv=\"refresh\" content=\"0;url=phone.html\"></head><body>redirecting</body></html>");
	if (post_data)
		delete[] post_data;
}

char* lang(const mg_request_info* ri, const char *key)
{
	FILE* f;
	if (ri->language[0] == 0 && strlen(deflanguage) == 2)
		f = fopen((dir+"language_"+deflanguage+".txt").c_str(),"r");
	else if (ri->language[0] == 0)
	{
		printf("null\n");
		strncpy(langret,key,1023);
		return langret;
	}
	else
		f = fopen((dir+"language_"+ri->language+".txt").c_str(),"r");
	char line[1024];
	langret[1023] = 0;
	if (f)
	{
		if (strcmp(key,"BEFORETIME")==0)
		{
			fclose(f);
			if (strcmp(ri->language,"es")==0 || (ri->language[0] == 0 && strcmp(deflanguage,"es")==0))
				strcpy(langret,"hace ");
			else
				strcpy(langret,"");
			return langret;
		}
		int n = strlen(key);
		while (fgets(line, sizeof(line)-1, f) != NULL) 
		{
			int l = strlen(line);
			if (l && line[l-1] == 10)
				{line[l-1] = 0; l--;}
			if (l && line[l-1] == 13)
				{line[l-1] = 0; l--;}
			if (line[0] == 239 && line[1] == 187 && line[2] == 191)
			{
				int i;
				for (i=3;i<l+1;i++)
					line[i-3]=line[i];
				l-=3;
			}
			if (strncmp(key,line,n) == 0 && line[n] == ' ' && line[n+1] == '-' && line[n+2] == '>' && line[n+3] == ' ')
			{
				
				strncpy(langret,line+n+4,1023);
				fclose(f);
				return langret;
			}
		}
		strncpy(langret,key,1023);
		fclose(f);
		return langret;
	}
	strncpy(langret,key,1023);
	return langret;
}
static void
sendmenu(struct mg_connection *conn,
                const struct mg_request_info *ri, void *data);
static void
cgi(struct mg_connection *conn,
                const struct mg_request_info *ri, void *data, int lSize, char* filebuffer)
{
	int i = 0;
	int start = 0;
	while (i<lSize)
	{
		if (filebuffer[i] == '`' && i < lSize-2)
		{
			mg_write(conn,filebuffer+start,i-start);
			i += 1;
			start = i;
			while (start < lSize && filebuffer[start] != '`')
				start++;
			filebuffer[start] = 0;
			start += 1;
			mg_printf(conn,"%s",lang(ri,filebuffer+i));
			i = start;
		}
		else
		if (filebuffer[i] == '<' && filebuffer[i+1] == '<' && i < lSize-4)
		{
			mg_write(conn,filebuffer+start,i-start);
			i += 2;
			start = i;
			int deep = 1;
			while (start < lSize)
			{
				if (filebuffer[start] == '>' && filebuffer[start+1] == '>')
					deep--;
				if (filebuffer[start] == '<' && filebuffer[start+1] == '<')
					deep++;
				if (!deep)
					break;
				start++;
			}
			filebuffer[start] = 0;
			start += 2;
			if (strcmp("MENU",filebuffer+i)==0)
			{
				sendmenu(conn,ri,data);
				i = start;
			}
			else
			if (strcmp("USERNAME",filebuffer+i)==0)
			{
				if (ri->remote_user)
					mg_printf(conn,"%s",ri->remote_user);
				i = start;
			}
			else
			if (strcmp("PORT",filebuffer+i)==0)
			{
				mg_printf(conn,"%d",port);
				i = start;
			}
			else
			if (strcmp("SSLPORT",filebuffer+i)==0)
			{
				mg_printf(conn,"%d",sslport);
				i = start;
			}
			else
			if (strcmp("WEBVERSION",filebuffer+i)==0)
			{
				mg_printf(conn,"1.9");
				i = start;
			}
			else
			if (strncmp("ADMIN",filebuffer+i,5)==0)
			{
				if (ri->permissions == PERM_ROOT )
				{
					filebuffer[start-2] = ' ';
					filebuffer[start-1] = ' ';
					i = i+5;
					start = i;
				}
				else
					i = start;
			}
			else
			if (strncmp("IFNOTZTEBLADE",filebuffer+i,13)==0)
			{
				if (!is_zte_blade)
				{
					filebuffer[start-2] = ' ';
					filebuffer[start-1] = ' ';
					i = i+13;
					start = i;
				}
				else
					i = start;
			}
//			else
//			if (strncmp("ONLYADMIN",filebuffer+i,9)==0)
//			{
//				if (ri && ri->remote_user && strcmp(ri->remote_user,"admin") == 0 )
//				{
//					filebuffer[start-2] = ' ';
//					filebuffer[start-1] = ' ';
//					i = i+9;
//					start = i;
//				}
//				else
//					i = start;
//			}
			else
			if (strncmp("CHANGE2SSL",filebuffer+i,10)==0)
			{
				if (ri->remote_ip && has_ssl && ri->is_ssl == false)
				{
					filebuffer[start-2] = ' ';
					filebuffer[start-1] = ' ';
					i = i+10;
					start = i;
				}
				else
					i = start;
			}
			else
			if (strncmp("CHANGE2NORMAL",filebuffer+i,13)==0)
			{
				if (ri->remote_ip && has_ssl && ri->is_ssl == true)
				{
					filebuffer[start-2] = ' ';
					filebuffer[start-1] = ' ';
					i = i+13;
					start = i;
				}
				else
					i = start;
			}
			else
			if (strcmp("FRAMEBUFFER_COUNT",filebuffer+i)==0)
			{
				if (scrinfo.yres == scrinfo.yres_virtual)
					mg_printf(conn,"hidden");
				else
					mg_printf(conn,"checkbox");
				i = start;
			}
			else
			if (strncmp("REGISTRATION",filebuffer+i,12)==0)
			{
				bool r = true;
				std::string sharedpref = dir + "../shared_prefs/com.webkey_preferences.xml";
				FILE* sp = fopen(sharedpref.c_str(),"r");
				if (sp)
				{
					char buff[256];
					while (fgets(buff, sizeof(buff)-1, sp) != NULL)
					{
						if (startswith(buff,"<boolean name=\"allowremotereg\" value=\"false\""))
						{
							r = false;
							break;
						}
					}
					fclose(sp);
				}
				if (r)
				{
					filebuffer[start-2] = ' ';
					filebuffer[start-1] = ' ';
					i = i+12;
					start = i;
				}
				else
					i = start;
			}
			else
			if (strncmp("NOREGISTRATION",filebuffer+i,14)==0)
			{
				bool r = true;
				std::string sharedpref = dir + "../shared_prefs/com.webkey_preferences.xml";
				FILE* sp = fopen(sharedpref.c_str(),"r");
				if (sp)
				{
					char buff[256];
					while (fgets(buff, sizeof(buff)-1, sp) != NULL)
					{
						if (startswith(buff,"<boolean name=\"allowremotereg\" value=\"false\""))
						{
							r = false;
							break;
						}
					}
					fclose(sp);
				}
				if (!r)
				{
					filebuffer[start-2] = ' ';
					filebuffer[start-1] = ' ';
					i = i+14;
					start = i;
				}
				else
					i = start;
			}
		}
		else
			i++;
	}
	mg_write(conn,filebuffer+start,i-start);
}
static void
sendmenu(struct mg_connection *conn,
                const struct mg_request_info *ri, void *data)
{
	lock_wakelock();
	mg_printf(conn,"<style type=\"text/css\"> .gray {color: #909090} .stat {background: #FFFFFF;font-size: 10px;width:940px;margin-bottom: 4px;} .red {background: red;width:940px;height:4px;} .green {background: green;width:940px;height:4px;} .yellow {background: yellow;width:940px;height:4px;}.butt {font-size: 9pt;background-color:#404040;color: #FFFFFF;} .menu a{color: #000000;font-size: 16px;text-decoration: none;padding-right: 10px;padding-left: 10px;} .menu a:hover{color: #e00000} .main{margin-right: auto;margin-left: auto;width: 940px;} .kep{clear: right;margin-right: 20px;} .list{margin-bottom:0px;margin-top:10px} .rightmenu{font-size:12px} .gather{font-variant: small-caps;}</style>\n<link rel=\"shortcut icon\" href=\"favicon.ico\">\n</head>\n<body style=\"margin-right: auto; margin-left: auto;\"><div class=\"menu\" style=\"background-color: #eeaa44; padding-top: 8px; padding-bottom: 8px; text-align: center;\">");
	if (ri->permissions == PERM_ROOT || (ri->permissions&PERM_SCREENSHOT))
		mg_printf(conn,"<a href=\"phone.html\" target=\"_top\">%s</a> ",lang(ri,"Phone"));
	if (ri->permissions == PERM_ROOT || (ri->permissions&PERM_GPS))
		mg_printf(conn,"<a href=\"gps.html\" target=\"_top\">%s</a> ",lang(ri,"GPS"));
	if (ri->permissions == PERM_ROOT || (ri->permissions&PERM_SMS_CONTACT))
	{
		mg_printf(conn,"<a href=\"sms.html\" target=\"_top\">%s</a>",lang(ri,"SMS"));
		mg_printf(conn,"<a href=\"calls.html\" target=\"_top\">%s</a> ",lang(ri,"Call list"));
	}
	if (ri->permissions == PERM_ROOT)
	{
		mg_printf(conn,"<a href=\"notify.html\" target=\"_top\">%s</a>",lang(ri,"Notify"));
		mg_printf(conn,"<a href=\"terminal.html\" target=\"_top\">%s</a> ",lang(ri,"Terminal"));
	}
	if (ri->permissions == PERM_ROOT || (ri->permissions&PERM_SMS_CONTACT))
		mg_printf(conn,"<a href=\"export.html\" target=\"_top\">%s</a> ",lang(ri,"Export"));
	//if (ri->permissions == PERM_ROOT)
	mg_printf(conn,"<a href=\"config\" target=\"_top\">%s</a> ",lang(ri,"Config"));
	if (ri->permissions == PERM_ROOT || (ri->permissions&PERM_FILES) || (ri->permissions&PERM_PUBLIC))
		mg_printf(conn,"<a href=\"files.html\" target=\"_top\">%s</a> ",lang(ri,"Files"));
	if (ri->permissions == PERM_ROOT || (ri->permissions&PERM_SDCARD))
		mg_printf(conn,"<a href=\"sdcard\" target=\"_top\">%s</a> ",lang(ri,"Sdcard"));
//	mg_printf(conn,"<a href=\"help.html\" target=\"_top\">help</a> <a href=\"\" onclick=\"document.location.replace(document.location.href.replace(/:\\/\\//,':\\/\\/logout:logout@'))\"target=\"_top\">log out</a></div>");
//	mg_printf(conn,"<a href=\"help.html\" target=\"_top\">help</a> <a href=\"logout\" onclick=\"try {document.execCommand('ClearAuthenticationCache');} catch (exception) {}; document.location=document.location.href.replace(/:\\/\\//,':\\/\\/logout:logout@')\" target=\"_top\">log out</a></div>");
	mg_printf(conn,"<a href=\"help.html\" target=\"_top\">%s</a> ",lang(ri,"Help"));
	mg_printf(conn,"<a href=\"logout\" onclick=\"try {document.execCommand('ClearAuthenticationCache');} catch (exception) {};\" target=\"_top\">%s</a></div>",lang(ri,"Log&nbsp;out"));
	if ((ri->permissions == PERM_ROOT || (ri->permissions&PERM_CHAT)) && strcmp(ri->uri,"/pure_menu_nochat.html"))
	{
		pthread_mutex_lock(&chatmutex);
		pthread_cond_broadcast(&chatcond);
		pthread_mutex_unlock(&chatmutex);
		FILE* f;
		f = fo("chat.html","rb");
		if(!f)
			return;
		fseek (f , 0 , SEEK_END);
		int lSize = ftell (f);
		rewind (f);
		char* filebuffer = new char[lSize+1];
		if (filebuffer)
		{
			fread(filebuffer,1,lSize,f);
			filebuffer[lSize] = 0;
			cgi(conn,ri,data,lSize,filebuffer);
			fclose(f);
			delete[] filebuffer;
		} //what can we do with no memory?
	}
}


static void
getfile(struct mg_connection *conn,
                const struct mg_request_info *ri, void *data)
{
	FILE* f;
	if (strcmp(ri->uri,"/")==0)
		f = fo("index.html","rb");
	else
		f = fo(ri->uri,"rb");
	if(!f)
		return;
	fseek (f , 0 , SEEK_END);
	int lSize = ftell (f);
//	if (saysize)
//		send_ok(conn,NULL,lSize);
//	else
		send_ok(conn,NULL);
	rewind (f);
	char* filebuffer = new char[lSize+1];
	if (filebuffer)
	{
		fread(filebuffer,1,lSize,f);
		filebuffer[lSize] = 0;
		cgi(conn,ri,data,lSize,filebuffer);
		fclose(f);
		delete[] filebuffer;
	} //what can we do with no memory?
}
static void
sendfile(const char* file,struct mg_connection *conn, bool sendok = false, char * extra = NULL)
{
	FILE* f = fopen(file,"rb");
	if(!f)
	{
		if (sendok)
			send_ok(conn);
		return;
	}
	fseek (f , 0 , SEEK_END);
	int lSize = ftell (f);
	if (sendok)
		send_ok(conn,extra,lSize);
	rewind (f);
	char* filebuffer = new char[65536];
	if (filebuffer)
	{
		while(lSize>0)
		{
			int s = min(65536,lSize);
			fread(filebuffer,1,min(65536,lSize),f);
			mg_write(conn,filebuffer,s);
			lSize -= s;
		}
		fclose(f);
		delete[] filebuffer;
	} //what can we do with no memory?
}
static void
index(struct mg_connection *conn,
                const struct mg_request_info *ri, void *data)
{
	if (ri->permissions != PERM_ROOT && (ri->permissions&PERM_SCREENSHOT)==0)
	{
		mg_printf(conn,"HTTP/1.1 200 OK\r\nCache-Control: no-store, no-cache, must-revalidate\r\nCache-Control: post-check=0, pre-check=0\r\nPragma: no-cache\r\nContent-Type: text/html; charset=utf-8\r\nConnection: close\r\n\r\n<html><head><meta http-equiv=\"refresh\" content=\"0;url=pure_menu.html\"></head><body>Redirecting...</body></html>");
		return;
	}
	lock_wakelock();
	getfile(conn,ri,data);
	int i;
	if (ri->permissions == PERM_ROOT)
		for (i=0; i < fastkeys.size(); i++)
			if (fastkeys[i]->show)
			{
				mg_printf(conn,"<input type=\"button\" class=\"butt\" value=\"%s\" onclick=\"makeRequest('button_%d','')\"/>",lang(ri,fastkeys[i]->name),i+1);
			}
	if (ri->permissions == PERM_ROOT)
		mg_printf(conn,"</div><div id=\"res\">%s</div></div><script type=\"text/javascript\" language=\"javascript\">document.images.screenshot.onmousemove=move; document.images.arrows.onmousemove=arrows_move;</script>",lang(ri,"results"));
	mg_printf(conn," <script type=\"text/javascript\" language=\"javascript\">function setWidth(orient,hs){ var s=0; if (hs == 0) { if (orient == 1 ) s=%d; else s=%d;} else {if (orient == 1 ) s=%d; else s=%d;}; document.images.screenshot.width=s; document.getElementById('imgholder').style.width=s+'px'}; function negx(x) { if (document.getElementById('orient').checked==true) return %d-x; return %d-x;}; function negy(y) { if (document.getElementById('orient').checked==true) return %d-y; return %d-y;}; </script></body></html>",scrinfo.yres,scrinfo.xres,scrinfo.yres/2,scrinfo.xres/2,scrinfo.yres,scrinfo.xres,scrinfo.xres,scrinfo.yres);
}
static void
button(struct mg_connection *conn,
                const struct mg_request_info *ri, void *data)
{
//	TEST
//	if (ri->permissions != PERM_ROOT)
//		return;

	send_ok(conn);
	lock_wakelock();
	access_log(ri,"button inject");
	int key = getnum(ri->uri+8);
//	if (samsung && key == 28)
//		key = 63;
	int i = 8;
	while(ri->uri[i] && ri->uri[i] != '_') i++;
	long time = 0;
	if (i < strlen(ri->uri))
		time = getnum(ri->uri+i+1);
//	printf("KEY = %d\n",key);
//	for (std::map<int,DEVSPEC>::const_iterator it = device_specific_buttons.begin(); it != device_specific_buttons.end(); it++)
//	{
//		printf("%d: %d\n",it->first,it->second.dev);
//	}

//	printf("Size = %d -> ",device_specific_buttons.size());
	DEVSPEC d = device_specific_buttons[key];
//	printf("%d\n",device_specific_buttons.size());
	if (d.dev || d.type || d.code)
	{
		int dev;
		char file[20];
		strcpy(file,"/dev/input/eventX");
		file[strlen(file)-1] = 48+d.dev;
//		printf("%s\n",file);
//		printf("%d\n",d.dev);
//		printf("%d\n",d.type);
//		printf("%d\n",d.code);
		if((dev = open(file, O_WRONLY)) == -1)
		{
			return;
		}
		struct input_event event;
		memset(&event, 0, sizeof(event));
		gettimeofday(&event.time, 0);
		event.type = d.type;
		event.code = d.code;
		event.value = 1;
		write(dev, &event, sizeof(event));
		usleep(time*1000000);
		event.value = 0;
		write(dev, &event, sizeof(event));
		//printf("OK\n");
		close(dev);

	}
	else
	{
		if (time)
		{
			struct input_event event;
			memset(&event, 0, sizeof(event));
			gettimeofday(&event.time, 0);
			event.type = EV_KEY;
			event.code = key;
			event.value = 1;
			write(uinput_fd, &event, sizeof(event));
			usleep(time*1000);
			event.value = 0;
			write(uinput_fd, &event, sizeof(event));
		}
		else
			suinput_click(uinput_fd, key, 0);
	}
}
static void
config_buttons(struct mg_connection *conn,
                const struct mg_request_info *ri, void *data)
{
	if (ri->permissions != PERM_ROOT)
		return;
	lock_wakelock();
	getfile(conn,ri,data);
	mg_printf(conn,"<table border=\"1\">\n");
	int i;
	for (i=0; i < fastkeys.size(); i++)
	{
		mg_printf(conn, "<tr><td>%s</td>",lang(ri,fastkeys[i]->name));
		mg_printf(conn, "<td><input type=\"checkbox\" name=\"show_%d\">%s</input></td>",i+1,lang(ri,"Show"));
		mg_printf(conn, "<td>%s: <input type=\"text\" name=\"key_%d\" maxlength=\"1\" size=\"1\" onkeypress=\"var unicode=event.charCode? event.charCode : -event.keyCode;document.config_buttons.keycode_%d.value=unicode; document.config_buttons.key_%d.value=String.fromCharCode(document.config_buttons.keycode_%d.value)\"/></td>",lang(ri,"Key"),i+1,i+1,i+1,i+1);
		mg_printf(conn, "<td>%s: <input type=\"text\" name=\"keycode_%d\" maxlength=\"8\" size=\"8\" onkeyup=\"document.config_buttons.key_%d.value=String.fromCharCode(document.config_buttons.keycode_%d.value)\"/></td></tr>\n",lang(ri,"Keycode"),i+1,i+1,i+1);
	}
	mg_printf(conn,"</table></form>\n<script type=\"text/javascript\" language=\"javascript\">");

	for (i=0; i < fastkeys.size(); i++)
	{
		mg_printf(conn,"document.config_buttons.key_%d.value=String.fromCharCode(%d);document.config_buttons.keycode_%d.value='%d';document.config_buttons.show_%d",i+1,fastkeys[i]->ajax,i+1,fastkeys[i]->ajax,i+1);
		if (fastkeys[i]->show)
			mg_printf(conn,".checked='checked';\n");
		else
			mg_printf(conn,".checked='';\n");
	}
	mg_printf(conn,"</script></body></html>");
}
static void
notify_html(struct mg_connection *conn,
                const struct mg_request_info *ri, void *data)
{
	if (ri->permissions != PERM_ROOT)
		return;
	lock_wakelock();
	getfile(conn,ri,data);
	mg_printf(conn,"<script type=\"text/javascript\" language=\"javascript\">\n");
	mg_printf(conn,"document.getElementById('callon').checked = %s;\n",notify.callon?"true":"false");
	mg_printf(conn,"document.getElementById('smson').checked = %s;\n",notify.smson?"true":"false");
	mg_printf(conn,"document.getElementById('interval').value = %d;\n",notify.interval);
	mg_printf(conn,"document.getElementById('vibrate').checked = %s;\n",notify.vibrate?"true":"false");
	std::string t;
	int i;
	for (i = 0; i < notify.vibratepatt.length(); i++)
	{
		if (notify.vibratepatt[i] != '~')
			t += notify.vibratepatt[i];
		else
			t += ',';
	}
	mg_printf(conn,"document.getElementById('vibratepatt').value = \"%s\";\n",t.c_str());
	mg_printf(conn,"document.getElementById('blink').checked = %s;\n",notify.blink?"true":"false");
	mg_printf(conn,"document.getElementById('blinktype').selectedIndex = \"%d\";\n",notify.blinktype);
	mg_printf(conn,"document.getElementById('blinkon').checked = %d;\n",notify.blinkon);
	mg_printf(conn,"document.getElementById('blinkoff').checked = %d;\n",notify.blinkoff);
	mg_printf(conn,"</script></body></html>");
}
static bool
setupnotify(char * st, bool test = false)
{
	int n = strlen(st);
	if (st[n-1] == '\n')
		st[--n] = 0;
	int pos[8];
	int i;
	i = 0;
	int j;
	j = 0;
	while(st[i] && j < 8 && i < n)
	{
		if (st[i] == '_')
		{
			st[i] = 0;
			pos[j++] = i+1;
		}
		i++;
	}
	if (j<8)
		return false;
	set_blink(0,0,0);
	if (startswith(st,"true") || startswith(st,"checked"))
		notify.callon = true;
	else
		notify.callon = false;
	if (startswith(st+pos[0],"true") || startswith(st+pos[0],"checked"))
		notify.smson = true;
	else
		notify.smson = false;
	notify.interval = getnum(st+pos[1]);
	if (startswith(st+pos[2],"true") || startswith(st+pos[2],"checked"))
		notify.vibrate = true;
	else
		notify.vibrate = false;
	notify.vibratepatt = std::string(st+pos[3]);
	if (startswith(st+pos[4],"true") || startswith(st+pos[4],"checked"))
		notify.blink = true;
	else
		notify.blink = false;
	notify.blinktype = getnum(st+pos[5]);
	notify.blinkon = getnum(st+pos[6]);
	notify.blinkoff = getnum(st+pos[7]);

	notify.lastalarm = -1000000;
	if (test)
	{
		if (notify.blink)
			set_blink(1,notify.blinkon,notify.blinkoff);
		if (notify.vibrate)
		{
			int v, p, i;
			p = 0; v = 0;
			i = -1;
			bool b;
			b = false;
			while (i < notify.vibratepatt.size() || i == -1)
			{
				if (i == -1 || notify.vibratepatt[i] == '~')
				{
					p = v;
					v = getnum(notify.vibratepatt.c_str()+i+1);
					b = !b;
					if (b)
						vibrator_on(v);
					else
						usleep((p+v)*1000);
				}
				i++;
			}
			if (!b && notify.blink)
				usleep(v*1000);
		}
		if (notify.blink)
			set_blink(0,0,0);
	}
	return true;
}
static void
setnotify(struct mg_connection *conn,
                const struct mg_request_info *ri, void *data)
{
	if (ri->permissions != PERM_ROOT)
		return;
	lock_wakelock();
	access_log(ri,"setup notify");
	send_ok(conn);
	if (ri->uri[10] == '_')
	{
		FILE * out = fopen((dir+"notify.txt").c_str(),"w");
		if (out)
		{
			fprintf(out,"%s",ri->uri+11);
			fclose(out);
		}
		setupnotify(ri->uri+11,true);
	}
}
static void
config_keys(struct mg_connection *conn,
                const struct mg_request_info *ri, void *data)
{
	if (ri->permissions != PERM_ROOT)
		return;
	lock_wakelock();
	getfile(conn,ri,data);
	mg_printf(conn,"<table border=\"1\">\n");
	int i;
	for (i=0; i < 106; i++)
	{
		mg_printf(conn,"<tr><td>%s: <input type=\"text\" name=\"key_%d\" maxlength=\"1\" size=\"1\" onkeypress=\"var unicode=event.charCode? event.charCode : event.keyCode;document.config_keys.keycode_%d.value=unicode; document.config_keys.key_%d.value=String.fromCharCode(document.config_keys.keycode_%d.value)\"/></td>",lang(ri,"Char"),i+1,i+1,i+1,i+1);
		mg_printf(conn,"<td>%s: <input type=\"text\" name=\"keycode_%d\" maxlength=\"8\" size=\"8\" onkeyup=\"document.config_keys.key_%d.value=String.fromCharCode(document.config_keys.keycode_%d.value)\"/></td>",lang(ri,"Keycode"),i+1,i+1,i+1);
		mg_printf(conn,"<td> %s </td>",lang(ri,"converts to"));
		mg_printf(conn,"<td>%s: <input type=\"text\" name=\"tokey_%d\" maxlength=\"1\" size=\"1\" onkeypress=\"var unicode=event.charCode? event.charCode : event.keyCode;document.config_keys.tokeycode_%d.value=unicode; document.config_keys.tokey_%d.value=String.fromCharCode(document.config_keys.tokeycode_%d.value)\"/></td>",lang(ri,"Char"),i+1,i+1,i+1,i+1);
		mg_printf(conn,"<td>%s: <input type=\"text\" name=\"tokeycode_%d\" maxlength=\"8\" size=\"8\" onkeyup=\"document.config_keys.tokey_%d.value=String.fromCharCode(document.config_keys.tokeycode_%d.value)\"/></td>",lang(ri,"Keycode"),i+1,i+1,i+1);
		mg_printf(conn,"<td><input type=\"checkbox\" name=\"sms_%d\">%s</input></td></tr>\n",i+1,lang(ri,"works in SMS mode"));
	}
	mg_printf(conn,"</table></form>\n<script type=\"text/javascript\" language=\"javascript\">\n");

	for (i=0; i < speckeys.size(); i++)
	{
		mg_printf(conn,"document.config_keys.key_%d.value=String.fromCharCode(%d);document.config_keys.keycode_%d.value='%d';document.config_keys.tokey_%d.value=String.fromCharCode(%d);document.config_keys.tokeycode_%d.value='%d';document.config_keys.sms_%d",i+1,speckeys[i]->ajax,i+1,speckeys[i]->ajax,i+1,speckeys[i]->disp,i+1,speckeys[i]->disp,i+1);
		if (speckeys[i]->sms)
			mg_printf(conn,".checked='checked';\n");
		else
			mg_printf(conn,".checked='';\n");
	}
	mg_printf(conn,"</script></body></html>");
}
static std::string getoption(char* list, char* option)
{
	std::string ret;
	int n  = strlen(list);
	int i;
	for (i = 0; i < n; i++)
		if (startswith(list+i, option))
		{
			i+=strlen(option);
			while(i<n && list[i] != '&')
				ret += list[i++];
			break;
		}
	return ret;
}

static void
getreg(struct mg_connection *conn,
                const struct mg_request_info *ri, void *data)
{
	if (ri->remote_ip!=2130706433 || strcmp(ri->remote_user,"JAVA_CLIENT") != 0) //localhost
		return;
	send_ok(conn);
	mg_printf(conn,"%s\n%s\n%u",requested_username.c_str(),requested_password.c_str(),requested_ip);
	requested_username = "";
	requested_password = "";
}
static void
reg(struct mg_connection *conn,
                const struct mg_request_info *ri, void *data)
{
	bool r = true;
	std::string sharedpref = dir + "../shared_prefs/com.webkey_preferences.xml";
	FILE* sp = fopen(sharedpref.c_str(),"r");
	if (sp)
	{
		char buff[256];
		while (fgets(buff, sizeof(buff)-1, sp) != NULL)
		{
			if (startswith(buff,"<boolean name=\"allowremotereg\" value=\"false\""))
			{
				r = false;
				break;
			}
		}
		fclose(sp);
	}
	if (!r)
	{
		mg_printf(conn,"HTTP/1.1 200 OK\r\nCache-Control: no-store, no-cache, must-revalidate\r\nCache-Control: post-check=0, pre-check=0\r\nPragma: no-cache\r\nContent-Type: text/html; charset=utf-8\r\nConnection: close\r\n\r\n<html><head><meta http-equiv=\"refresh\" content=\"3;url=index.html\"></head><body>%s",lang(ri,"New user's registration is disabled, click on \"allow user registration in browser\" to enable it."));
		mg_printf(conn,"%s</body></html>",lang(ri,"Reloading..."));
		return;
	}
	char* post_data;
	int post_data_len;
	read_post_data(conn,ri,&post_data,&post_data_len);
	requested_username = "";
	if (strncmp(post_data,"username=",9))
		return;
	int i = 9;
	while (i < post_data_len && post_data[i] != '&')
	{
		requested_username += post_data[i++];
	}
	if (strncmp(post_data+i,"&password=",10))
	{
		requested_username = "";
		return;
	}
	i += 10;
	requested_password = "";
	while (i < post_data_len && post_data[i] != '&')
	{
		requested_password += post_data[i++];
	}
	if (requested_username.size() >= FILENAME_MAX-1 || requested_password.size() >= FILENAME_MAX-1)
	{
		requested_username = "";
		return;
	}
	requested_ip = ri->remote_ip;
	char to[FILENAME_MAX];
	strcpy(to,requested_username.c_str());
	url_decode(to, strlen(to), to, FILENAME_MAX, true);
	requested_username = to;
	strcpy(to,requested_password.c_str());
	url_decode(to, strlen(to), to, FILENAME_MAX, true);
	requested_password = to;
	delete[] post_data;
	mg_printf(conn,"HTTP/1.1 200 OK\r\nCache-Control: no-store, no-cache, must-revalidate\r\nCache-Control: post-check=0, pre-check=0\r\nPragma: no-cache\r\nContent-Type: text/html; charset=utf-8\r\nConnection: close\r\n\r\n<html><head><meta http-equiv=\"refresh\" content=\"10;url=index.html\"></head><body>%s ",lang(ri,"Registration request sent, please wait until it's authorized on the phone."));
	mg_printf(conn,"%s<br/><img alt=\"reganim\" src=\"reganim.gif\"/></body></html>",lang(ri,"Reloading..."));
	syst("am broadcast -a \"webkey.intent.action.remote.registration\"");
}
static void
setpassword(struct mg_connection *conn,
                const struct mg_request_info *ri, void *data)
{
	if (ri->remote_ip!=2130706433 || (ri->remote_user && strcmp(ri->remote_user,"JAVA_CLIENT") != 0)) //localhost
		return;
	char* post_data;
	int post_data_len;
	read_post_data(conn,ri,&post_data,&post_data_len);
	int i = 0;
	int pos[1];
	int j = 0;
	for (;i < post_data_len; i++)
	{
		if (post_data[i] == '\n' && j < 1)
		{
			post_data[i] = 0;
			pos[j++] = i+1;
		}
	}
//	printf("setpassword:\n");
//	printf("username = %s\n",post_data);
//	printf("password = %s\n",post_data+pos[0]);
	mg_modify_passwords_file(ctx, (dir+passfile).c_str(), post_data, post_data+pos[0],-2);
	send_ok(conn);
}
static void
setpermission(struct mg_connection *conn,
                const struct mg_request_info *ri, void *data)
{
	if (ri->remote_ip!=2130706433 || (ri->remote_user && strcmp(ri->remote_user,"JAVA_CLIENT") != 0)) //localhost
		return;
	char* post_data;
	int post_data_len;
	read_post_data(conn,ri,&post_data,&post_data_len);
	int i = 0;
	int pos[2];
	int j = 0;
	for (;i < post_data_len; i++)
	{
		if (post_data[i] == '\n' && j < 2)
		{
			post_data[i] = 0;
			pos[j++] = i+1;
		}
	}
//	printf("setpermission:\n");
//	printf("username = %s\n",post_data);
//	printf("password = %s\n",post_data+pos[0]);
//	printf("permission = %s\n",post_data+pos[1]);
	mg_modify_passwords_file(ctx, (dir+passfile).c_str(), post_data, post_data+pos[0],getnum(post_data+pos[1]));
	send_ok(conn);
}

static void
config(struct mg_connection *conn,
                const struct mg_request_info *ri, void *data)
{
//	if (ri->permissions != PERM_ROOT)
//		return;
	lock_wakelock();
	char* post_data;
	int post_data_len;
	read_post_data(conn,ri,&post_data,&post_data_len);
        int n = post_data_len;
	int i = 0;
	char name[256];
	char pass[256];
	name[0] = pass[0] = 0;
	bool changed = false;
	bool changed_perm = false;
	int permissions = 0;
	int j = 0;
	if (n>256)
		return;
	while(i < n)
	{
//		printf("%s\n",ri->post_data);
		if (!memcmp(post_data+i, "username",8))
		{	
			i+=9;
			j = 0;
			while(i<n && post_data[i] != '&' && j<255)
				name[j++] = post_data[i++];		
			name[j] = 0; i++;
		}
		else if (!memcmp(post_data+i, "permission",10))
		{	
			changed_perm = true;
			changed = true;
			i+=10;
			if (post_data[i] < '8' && post_data[i] >= '0' && permissions != -1)
			{
				int p = post_data[i] - 48;
				if (p == 0)
					permissions = -1;
				else
					permissions = permissions | (1<<(p-1));
			}
			
			while(i<n && post_data[i] != '&')
				i++;		
			i++;
		}
		else if (!memcmp(post_data+i, "password",8))
		{
			i+=9;
			int k = 0;
			while(i<n && post_data[i] != '&' && k<255)
				pass[k++] = post_data[i++];		
			pass[k] = 0; i++;
			if (j && k)
			{
				if (ri->permissions == PERM_ROOT || strcmp(ri->remote_user,name)==0)
				{
					access_log(ri,"modify users");
					url_decode(name, strlen(name), name, FILENAME_MAX, true);
					url_decode(pass, strlen(pass), pass, FILENAME_MAX, true);
					for (int q = 0; q < strlen(name); q++)
						if (name[q] == ':')
							name[q] = ' ';
					mg_modify_passwords_file(ctx, (dir+passfile).c_str(), name, pass,-2);
				}
				changed = true;
			}
		}
		else if (!memcmp(post_data+i, "remove",6))
		{
			i=n;
			if (j)
			{
				if (ri->permissions == PERM_ROOT)
				{
					access_log(ri,"modify users");
					url_decode(name, strlen(name), name, FILENAME_MAX, true);
					for (int q = 0; q < strlen(name); q++)
						if (name[q] == ':')
							name[q] = ' ';
					mg_modify_passwords_file(ctx, (dir+passfile).c_str(), name, "",-2);
				}
				changed = true;
			}
		}
		else if (!memcmp(post_data+i, "dellog",6))
		{
			if (ri->permissions == PERM_ROOT)
			{
				
				pthread_mutex_lock(&logmutex);
				FILE* f = fopen(logfile.c_str(),"w");
				if (f)
				{
					fclose(f);
					access_times.clear();
					pthread_mutex_unlock(&logmutex);
					access_log(ri,"clear log");
				}
				else
					pthread_mutex_unlock(&logmutex);
			}
			changed = true;
			break;
		}
		else
			i++;
	}
	if (changed_perm && ri->permissions == PERM_ROOT)
	{
		if (permissions != PERM_ROOT && (permissions & PERM_PUBLIC) && (permissions & PERM_FILES))
			permissions = (permissions ^ PERM_PUBLIC);
		url_decode(name, strlen(name), name, FILENAME_MAX, true);
		url_decode(pass, strlen(pass), pass, FILENAME_MAX, true);
		for (int q = 0; q < strlen(name); q++)
			if (name[q] == ':')
				name[q] = ' ';
		mg_modify_passwords_file(ctx, (dir+passfile).c_str(), name, pass,permissions);
		access_log(ri,"modify permissions");
	}
	if (changed)
	{
		if (!data)
			mg_printf(conn,"HTTP/1.1 200 OK\r\nCache-Control: no-store, no-cache, must-revalidate\r\nCache-Control: post-check=0, pre-check=0\r\nPragma: no-cache\r\nContent-Type: text/html; charset=utf-8\r\nConnection: close\r\n\r\n<html><head><meta http-equiv=\"refresh\" content=\"1;url=config\"></head><body>%s</body></html>",lang(ri,"Saved, reloading ..."));
		else
			mg_printf(conn,"HTTP/1.1 200 OK\r\nCache-Control: no-store, no-cache, must-revalidate\r\nCache-Control: post-check=0, pre-check=0\r\nPragma: no-cache\r\nContent-Type: text/html; charset=utf-8\r\nConnection: close\r\n\r\n<html><head><meta http-equiv=\"refresh\" content=\"1;url=usersconfig\"></head><body>%s</body></html>",lang(ri,"Saved, reloading ..."));
		if (post_data)
			delete[] post_data;
		return;
	}
	send_ok(conn);
	mg_printf(conn,"<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.01//EN\" \"http://www.w3.org/TR/html4/strict.dtd\">\n<html>\n<head>\n<meta http-equiv=\"Content-Type\" content=\"text/html; charset=utf-8\">\n<title>%s</title>",lang(ri,"Webkey for Android"));
	if (!data)
		sendmenu(conn,ri,NULL);
//	mg_printf(conn,"<br/>Users. The user \'admin\' will have ALL permissions and a random password each time you start the service. Empty passwords are not allowed.<br/>");
	char line[256]; char domain[256];
	FILE* fp = fo(passfile.c_str(),"r");
	if (!fp)
		return;
	while (fgets(line, sizeof(line)-1, fp) != NULL) 
	{
		permissions = -1;
		if (sscanf(line, "%[^:]:%[^:]:%[^:]:%d", name, domain, pass, &permissions) < 3)
			continue;
		if (ri->permissions != PERM_ROOT && strcmp(name,ri->remote_user)!=0)
			continue;
			
		mg_printf(conn,"<hr/>");
		mg_printf(conn,"<form name=\"%s_form\" method=\"post\">%s: <input type=\"text\" readonly=\"readonly\" value=\"%s\" name=\"username\">",name,lang(ri,"username"),name);
		mg_printf(conn,"%s: <input type=\"password\" name=\"password\"></input>",lang(ri,"password"));
		mg_printf(conn,"<input type=\"submit\" value=\"%s\"></input>",lang(ri,"Change password"));
		if (ri->permissions == PERM_ROOT)
			mg_printf(conn,"<input name=\"remove\" type=\"submit\" value=\"%s\"></input></form>",lang(ri,"Remove user"));
		else
			mg_printf(conn,"<br/>");
		mg_printf(conn,"<form name=\"%s_form\" method=\"post\"><input type=\"hidden\" name=\"username\" value=\"%s\">",name,name);
		mg_printf(conn,"<input type=\"checkbox\" name=\"permission0\" %s %s>%s<input>",permissions == -1? "checked=\"yes\"":"",ri->permissions == -1?"":"disabled=false",lang(ri,"ALL"));
		mg_printf(conn,"<input type=\"checkbox\" name=\"permission1\" %s %s>%s</input>",(permissions != -1) && (permissions & PERM_SCREENSHOT)? "checked=\"yes\"":"",ri->permissions == -1?"":"disabled=false",lang(ri,"Screenshot"));
		mg_printf(conn,"<input type=\"checkbox\" name=\"permission2\" %s %s>%s</input>",(permissions != -1) && (permissions & PERM_GPS)? "checked=\"yes\"":"",ri->permissions == -1?"":"disabled=false",lang(ri,"Location"));
		mg_printf(conn,"<input type=\"checkbox\" name=\"permission3\" %s %s>%s</input>",(permissions != -1) && (permissions & PERM_CHAT)? "checked=\"yes\"":"",ri->permissions == -1?"":"disabled=false",lang(ri,"Chat"));
		mg_printf(conn,"<input type=\"checkbox\" name=\"permission4\" %s %s>%s</input>",(permissions != -1) && (permissions & PERM_PUBLIC)? "checked=\"yes\"":"",ri->permissions == -1?"":"disabled=false",lang(ri,"Read /sdcard/public/"));
		mg_printf(conn,"<input type=\"checkbox\" name=\"permission5\" %s %s>%s</input>",(permissions != -1) && (permissions & PERM_SMS_CONTACT)? "checked=\"yes\"":"",ri->permissions == -1?"":"disabled=false",lang(ri,"Contacts, Sms, Calls"));
		mg_printf(conn,"<input type=\"checkbox\" name=\"permission6\" %s %s>%s</input>",(permissions != -1) && (permissions & PERM_FILES)? "checked=\"yes\"":"",ri->permissions == -1?"":"disabled=false",lang(ri,"Read files"));
		mg_printf(conn,"<input type=\"checkbox\" name=\"permission7\" %s %s>%s</input>",(permissions != -1) && (permissions & PERM_SDCARD)? "checked=\"yes\"":"",ri->permissions == -1?"":"disabled=false",lang(ri,"Sdcard"));
		if (ri->permissions == PERM_ROOT)
			mg_printf(conn,"<input type=\"submit\" value=\"%s\"></input>",lang(ri,"Save permissions"));
		mg_printf(conn,"<br/>");
//		if (!strcmp(name,"admin"))
//			mg_printf(conn,"This password will change on every restart\n");
		mg_printf(conn,"</form>\n");
	}
	mg_printf(conn,"<hr/>");
	if (ri->permissions == PERM_ROOT)
	{
		mg_printf(conn,"<form name=\"newuser\" method=\"post\">%s:<input type=\"text\" name=\"username\">",lang(ri,"New user"));
		mg_printf(conn,"%s: <input type=\"password\" name=\"password\"></input>",lang(ri,"password"));
		mg_printf(conn,"<input type=\"submit\" value=\"%s\"></input></form>\n",lang(ri,"Create"));
	}
	fclose(fp);
	mg_printf(conn,"<h3>%s</h3>",lang(ri,"Details about permissions"));
	mg_printf(conn,"<h4 class=\"list\">%s</h4>",lang(ri,"ALL"));
	mg_printf(conn,"%s",lang(ri,"All other permissions. Additionally, user can inject keys, inject touch events, run commands, run commands in terminal, view log. These functions are not accessible by any of the other permissions."));
	mg_printf(conn,"<h4 class=\"list\">%s</h4>",lang(ri,"Screenshot"));
	mg_printf(conn,"%s",lang(ri,"View screenshot of the phone."));
	mg_printf(conn,"<h4 class=\"list\">%s</h4>",lang(ri,"Location"));
	mg_printf(conn,"%s",lang(ri,"View GPS and network location of the phone. The GPS is only available if it is enabled in your phone's Settings. Network location depends on your carrier."));
	mg_printf(conn,"<h4 class=\"list\">%s</h4>",lang(ri,"Chat"));
	mg_printf(conn,"%s",lang(ri,"Read and write messages. Every message is in the same list, and everyone can empty that list."));
	mg_printf(conn,"<h4 class=\"list\">%s</h4>",lang(ri,"Read /sdcard/public/"));
	mg_printf(conn,"%s",lang(ri,"If this permission is set and the permission \"Read files\" is not, then under the menu \"Files\" only the content of /sdcard/public will be available for read. Please create that directory if you are about to use this function."));
	mg_printf(conn,"<h4 class=\"list\">%s</h4>",lang(ri,"Contacts, Sms, Calls"));
	mg_printf(conn,"%s",lang(ri,"Read personal data such as contacts, messages and call list."));
	mg_printf(conn,"<h4 class=\"list\">%s</h4>",lang(ri,"Read files"));
	mg_printf(conn,"%s",lang(ri,"Read all files on the phone. Be careful, the contacts, messages, call list and the passwords of Webkey are stored in files!"));
	mg_printf(conn,"<h4 class=\"list\">%s</h4>",lang(ri,"Sdcard"));
	mg_printf(conn,"%s",lang(ri,"Read and modify the content of the sdcard."));
	
	if (ri->permissions == PERM_ROOT)
	{
		mg_printf(conn,"<h3>%s</h3>",lang(ri,"Log (activities is logged once in every 10 minutes)"));
		FILE *f = fopen(logfile.c_str(),"r");
		if(f)
		{
			char buff[256];
			while (fgets(buff, sizeof(buff)-1, f) != NULL)
			{
				mg_printf(conn,"%s<br/>",buff);
			}
			fclose(f);
		}
		mg_printf(conn,"<form name=\"dellog_form\" method=\"post\"><input type=\"hidden\" name=\"dellog\" value=\"dellog\"><input type=\"submit\" value=\"%s\"></input></form>",lang(ri,"Clear log"));
	}
	mg_printf(conn,"</body></html>");
	if (post_data)
		delete[] post_data;
}
static void
screenshot(struct mg_connection *conn,
                const struct mg_request_info *ri, void *data)
{
	if (ri->permissions != PERM_ROOT && (ri->permissions&PERM_SCREENSHOT)==0)
		return;
	lock_wakelock();
	access_log(ri,"view screenshot");
	int orient = 0;
	bool png = false;
	if (ri->uri[12] == 'p')
		png = true;
	if (ri->uri[16] == 'h') // horizontal
		orient = 1;
	int lowres = 0;
	if (ri->uri[17] == 'l') // low res
		lowres = 1;
	firstfb = false;
	if (ri->uri[18] == 'f') // first fb
		firstfb = true;
	bool flip = false;
	if (ri->uri[19] == 'f') // flip
		flip = true;
	bool wait = false;
	if (ri->uri[20] == 'w') // wait for diff
		wait = true;
	bool asfile = false;
	if (ri->uri[21] == 'f') // save as file
	{
		asfile = true;
	}
	if (!asfile)
		send_ok(conn);

	lastorient = orient;
	lastflip = flip;
	if (wait && !picchanged)
	{
		pthread_mutex_lock(&diffmutex);
		pthread_cond_broadcast(&diffstartcond);
		if (!exit_flag)
			pthread_cond_wait(&diffcond,&diffmutex);
		pthread_mutex_unlock(&diffmutex);
	}
	if (!pict)
		init_fb();
	FILE* f;
	std::string path = dir+"tmp";
	if (!recording)
	{
		if (!pthread_mutex_trylock(&pngmutex))
		{
			update_image(orient,lowres,png,flip);
			pthread_mutex_unlock(&pngmutex);
		}
		else
		{
			pthread_mutex_lock(&pngmutex);
			pthread_mutex_unlock(&pngmutex);
		}
	}
	else
	{
		char num[32];
		path = "/sdcard/webkey_TEMP/screenshot_";
		pthread_mutex_lock(&pngmutex);
		int fin = recordingnumfinished;
		pthread_mutex_unlock(&pngmutex);
		itoa(num,fin);
		int l = strlen(num);
		if (l<2)
			path = path+"0";
		if (l<3)
			path = path+"0";
		path = path + num;
		if (fin < 0)
			path = dir+"tmp";
	}
	if (png)
	{
		path += ".png";
		f = fopen(path.c_str(),"rb");
	}
	else
	{
		path += ".jpg";
		f = fopen(path.c_str(),"rb");
	}
	if (!f)
	{
		return;
	}
	fseek (f , 0 , SEEK_END);
	int lSize = ftell (f);
	rewind (f);
	char* filebuffer = new char[lSize+1];
	if(!filebuffer)
	{
		error("not enough memory for loading tmp.png\n");
	}
	fread(filebuffer,1,lSize,f);
	filebuffer[lSize] = 0;
	//printf("sent bytes = %d\n",mg_write(conn,filebuffer,lSize));
	if (asfile)
	{
		if (png)
			send_ok(conn,"Content-Type: image/png; charset=UTF-8\r\nContent-Disposition: attachment;filename=screenshot.png",lSize);
		else
			send_ok(conn,"Content-Type: image/jpeg; charset=UTF-8\r\nContent-Disposition: attachment;filename=screenshot.jpg",lSize);
	}
	mg_write(conn,filebuffer,lSize);
	fclose(f);
	delete[] filebuffer;
}
static void
stop(struct mg_connection *conn,
                const struct mg_request_info *ri, void *data)
{
	send_ok(conn);
	if (ri->remote_ip==2130706433) //localhost
	{
//		printf("Stopping server...\n");
		exit_flag = 2;
		mg_printf(conn,"Goodbye.");
		access_log(ri,"stop service");
	}
}
static void
run(struct mg_connection *conn,
                const struct mg_request_info *ri, void *data)
{
	if (ri->permissions != PERM_ROOT)
		return;
	lock_wakelock();
	access_log(ri,"run command");
	char* post_data;
	int post_data_len;
	read_post_data(conn,ri,&post_data,&post_data_len);
	send_ok(conn);
	int n = 4;
	std::string call = "";
	if (post_data_len)
	{
		call = post_data;
	}
	else
	if (ri->uri[n] == '_')
		while (ri->uri[++n])
		{
			int k = getnum(ri->uri+n);
			while(ri->uri[n] && ri->uri[n] != '_') n++;
			if (k<=0 || 255<k)
				continue;
			call += (char)k;
		}
	if (post_data)
		delete[] post_data;
	call += " 2>&1";
	FILE* in;
//	printf("%s\n",call.c_str());
	if ((in = popen(call.c_str(),"r")) == NULL)
		return;
	char buff[256];
	bool empty = true;
	struct timeval tv;
	gettimeofday(&tv,0);
	time_t lasttime = tv.tv_sec;
	while (fgets(buff, sizeof(buff)-1, in) != NULL)
	{
		int i = mg_printf(conn, "%s",buff);
		empty = false;
		if (buff[0] && i == 0)
			break;
		gettimeofday(&tv,0);
		if (lasttime + 120 < tv.tv_sec)
			break;
	}
	if (empty)
		mg_printf(conn,"</pre>empty<pre>");
	pclose(in);
}
static void
sendsms(struct mg_connection *conn,
                const struct mg_request_info *ri, void *data)
{
	if (ri->permissions != PERM_ROOT && (ri->permissions&PERM_SMS_CONTACT)==0)
		return;
	lock_wakelock();
	access_log(ri,"send sms");
	char* post_data;
	int post_data_len;
	read_post_data(conn,ri,&post_data,&post_data_len);
//	printf("sendsms\n");
	send_ok(conn);
	int n = 8;
	int i;
	std::string text = "";
	std::string c = "";
	FILE* out;
	out = fo("smsqueue","w");
	fwrite(post_data,1,post_data_len,out);
	fclose(out);
	syst("/system/bin/am broadcast -a \"webkey.intent.action.SMS.SEND\" -n \"com.webkey/.SMS\"");
	if (post_data)
		delete[] post_data;
}
static void
sendbroadcast(struct mg_connection *conn,
                const struct mg_request_info *ri, void *data)
{
	send_ok(conn);
	mg_printf(conn,"OK");
	if (ri->remote_ip==2130706433 && strcmp(ri->remote_user,"JAVA_CLIENT") == 0) //localhost
	{
		lock_wakelock();
		syst("/system/bin/am broadcast -a \"android.intent.action.BOOT_COMPLETED\" -n com.android.mms/.transaction.SmsReceiver&");
	}
}
static void
gpsset(struct mg_connection *conn,
                const struct mg_request_info *ri, void *data)
{
	send_ok(conn);
	if (ri->remote_ip==2130706433 && strcmp(ri->remote_user,"JAVA_CLIENT") == 0) //localhost
	{
		lock_wakelock();
		int n = strlen(ri->uri)-7;
		int i;
//		printf("%s\n",ri->uri);
		if (n<512)
			for (i=n;i>=0;i--)
				position_value[i] = ri->uri[i+7];
		position_id++;
	}
}
static void
gpsget(struct mg_connection *conn,
                const struct mg_request_info *ri, void *data)
{
	if (ri->permissions != PERM_ROOT && (ri->permissions&PERM_GPS)==0)
		return;
	lock_wakelock();
	access_log(ri,"get gps position");
	send_ok(conn);
	FILE* in;
	int last = ++position_id;
	syst("/system/bin/am broadcast -a \"webkey.intent.action.GPS.START\" -n \"com.webkey/.GPS\"");
	gps_active = true;
	struct timeval tv;
	gettimeofday(&tv,0);
	last_gps_time = tv.tv_sec;
	int i;
	for (i=0; i<25;i++)
	{
		if (last != position_id)
		{
			mg_printf(conn,"%s",position_value);
			break;
		}
		struct timespec tim;
		tim.tv_sec = 0;
		tim.tv_nsec = 200000000; //wait 200 ms
		nanosleep(&tim,0);
	}
}
//static void
//uploadsms(struct mg_connection *conn,
//               const struct mg_request_info *ri, void *data)
//{
//	send_ok(conn);
//	if (ri->remote_ip==2130706433) //localhost
//	{
//		ri->post_data[ri->post_data_len-1] = 0;
//		pthread_mutex_lock(&smsmutex);
//		pthread_cond_broadcast(&smscond);
//		sms = ri->post_data;
//		pthread_mutex_unlock(&smsmutex);
//	}
//}
//static void
//uploadcontacts(struct mg_connection *conn,
//                const struct mg_request_info *ri, void *data)
//{
//	send_ok(conn);
//	if (ri->remote_ip==2130706433) //localhost
//	{
//		ri->post_data[ri->post_data_len-1] = 0;
//		pthread_mutex_lock(&contactsmutex);
//		pthread_cond_broadcast(&contactscond);
//		contacts = ri->post_data;
//		pthread_mutex_unlock(&contactsmutex);
//	}
//}

static void load_mimetypes()
{
	std::string cmd = dir + "sqlite3 /data/data/com.android.providers.contacts/databases/contacts2.db \'select * from mimetypes\'";
	pthread_mutex_lock(&popenmutex);
	fprintf(pipeout,"%s\n",cmd.c_str());
	fflush(NULL);
	char buff[LINESIZE];
	int pos[7];
	int i;
	int j;
	bool first = true;
	while (fgets(buff, LINESIZE-1, pipein) != NULL)
	{
		if (strcmp(buff,"!!!END_OF_POPEN!!!\n")==0)
			break;
		i = getnum(buff);
		if (i>0 && i < 32)
		{
			j=0;
			while(buff[j] && buff[j]!='/' && j<LINESIZE-1)
				j++;
			j++;
			int n = strlen(buff);
			if (buff[n-1]=='\n')
				buff[n-1] = 0;
			if (buff[j])
			{
				mimetypes[i] = buff+j;
//				printf("%d: %s\n",i,mimetypes[i].c_str());
			}
		}
	}
//        fflush(stdout);
	fflush(NULL);
	pthread_mutex_unlock(&popenmutex);
}
static void
smsxml(struct mg_connection *conn,
                const struct mg_request_info *ri, void *data)
{
	if (ri->permissions != PERM_ROOT && (ri->permissions&PERM_SMS_CONTACT)==0)
		return;
	lock_wakelock();
	access_log(ri,"read sms messages");
	send_ok(conn,"Content-Type: text/xml; charset=UTF-8");
	
	std::string cmd = dir + "sqlite3 /data/data/com.android.providers.telephony/databases/mmssms.db 'select \"_id\",\"address\",\"person\",\"date\",\"read\",\"status\",\"type\",\"body\" from sms order by \"date\"'";
//	printf("%s\n",cmd.c_str());
	pthread_mutex_lock(&popenmutex);
	fprintf(pipeout,"%s\n",cmd.c_str());
	fflush(NULL);
	char buff[LINESIZE];
	char conv[LINESIZE];
//	buff[255] = 0;
	int pos[7];
	int i;
	int j;
	mg_printf(conn,"<?xml version=\"1.0\" encoding=\"utf-8\"?>\n<messages>\n");
	bool first = true;
	while (fgets(buff, LINESIZE-1, pipein) != NULL)
	{
		if (strcmp(buff,"!!!END_OF_POPEN!!!\n")==0)
			break;
		i = 0; j = 0;
		while(buff[i])
			if (buff[i++] == '|')
				j++;
		if (j<7)
		{
			mg_printf(conn,"%s",convertxml(conv, buff));
			continue;
		}
		i = 0;
		j = 0;
		while(buff[i] && j < 7 && i < LINESIZE-1)
		{
			if (buff[i] == '|')
			{
				buff[i] = 0;
				pos[j++] = i+1;
			}
			i++;
		}
		if (!first)
			mg_printf(conn,"</body></sms>\n");
		first = false;
		mg_printf(conn,"<sms><id>%s</id><number>%s</number>",buff,convertxml(conv, buff+pos[0]));
		if (buff[pos[1]])
			mg_printf(conn,"<person>%s</person>",convertxml(conv, buff+pos[1]));
		mg_printf(conn,"<date>%s</date><read>%s</read><status>%s</status><type>%s</type><body>%s",buff+pos[2],buff+pos[3],buff+pos[4],buff+pos[5],convertxml(conv, buff+pos[6]));
	}
	if (!first)
		mg_printf(conn,"</body></sms>\n");
	mg_printf(conn,"</messages>");
	fflush(NULL);
	pthread_mutex_unlock(&popenmutex);

//	pthread_mutex_lock(&smsmutex);
//	syst("/system/bin/am broadcast -a \"webkey.intent.action.SMS\"&");
//	pthread_cond_wait(&smscond,&smsmutex);
//	mg_write(conn,sms.c_str(),sms.length());
//	pthread_mutex_unlock(&smsmutex);
}
static void
contactsxml(struct mg_connection *conn,
                const struct mg_request_info *ri, void *data)
{
	if (ri->permissions != PERM_ROOT && (ri->permissions&PERM_SMS_CONTACT)==0)
		return;
	lock_wakelock();
	access_log(ri,"read contact data");
	send_ok(conn,"Content-Type: text/xml; charset=UTF-8");

	std::string cmd = dir + "sqlite3 /data/data/com.android.providers.contacts/databases/contacts2.db \'select \"raw_contact_id\",\"mimetype_id\",\"data1\" from data where \"data1\" <> \"\" order by \"raw_contact_id\"\'";

	char buff[LINESIZE];
	char last[LINESIZE];
	char conv[LINESIZE];
	last[0] = 0;
	int pos[3];
	int i;
	int j;
	mg_printf(conn,"<?xml version=\"1.0\" encoding=\"utf-8\"?>\n<people>\n");
	bool first = true;
	for (i = 0; i < 32; i++)
		if (mimetypes[i].length())
			break;
	if (i==32)
		load_mimetypes();
	int lastmime = -1;
	pthread_mutex_lock(&popenmutex);
	fprintf(pipeout,"%s\n",cmd.c_str());
	fflush(NULL);
	while (fgets(buff, LINESIZE-1, pipein) != NULL)
	{
		if (strcmp(buff,"!!!END_OF_POPEN!!!\n")==0)
			break;
		i = 0;
		j = 0;
		while(buff[i] && buff[i] != '\n' && j < 2 && i < 255)
		{
			if (buff[i] == '|')
			{
				buff[i] = 0;
				pos[j++] = i+1;
			}
			i++;
		}
		if (j<2)
		{
			if (lastmime != -1)
				mg_printf(conn,"\n%s",convertxml(conv, buff));
			continue;
		}
		if (lastmime != -1)
			mg_printf(conn,"</%s>",mimetypes[lastmime].c_str());
		while(buff[i]) i++;
		if (i)
			buff[i-1] = 0;
		if (strcmp(buff,last))
		{
			strcpy(last,buff);
			if (first)
				mg_printf(conn,"<contact><id>%s</id>",buff);
			else
				mg_printf(conn,"</contact>\n<contact><id>%s</id>",buff);
			first = false;
		}
		int n = getnum(buff+pos[0]);
		lastmime = n;
		if (n > 0 && n < 32 && mimetypes[n].length() && buff[pos[1]])
		{
			mg_printf(conn,"<%s>%s",mimetypes[n].c_str(),convertxml(conv, buff+pos[1]));
		}
	}
	if (lastmime != -1)
		mg_printf(conn,"</%s>",mimetypes[lastmime].c_str());
	if (first)
		mg_printf(conn,"<contact><name>Empty list. Your phone might not be supported. On some newer and older phones the contact list is not in /data/data/com.android.providers.contacts/databases/contacts2.db</name><phone_v2>_</phone_v2></contact></people>");
	else
		mg_printf(conn,"</contact></people>");

	fflush(NULL);
	pthread_mutex_unlock(&popenmutex);

//	pthread_mutex_lock(&contactsmutex);
//	syst("/system/bin/am broadcast -a \"webkey.intent.action.CONTACTS\"&");
//	pthread_cond_wait(&contactscond,&contactsmutex);
//	mg_write(conn,contacts.c_str(),contacts.length());
//	pthread_mutex_unlock(&contactsmutex);
}
static void
callsxml(struct mg_connection *conn,
                const struct mg_request_info *ri, void *data)
{
	if (ri->permissions != PERM_ROOT && (ri->permissions&PERM_SMS_CONTACT)==0)
		return;
	lock_wakelock();
	access_log(ri,"read call log");
	send_ok(conn,"Content-Type: text/xml; charset=UTF-8");

	std::string cmd = dir + "sqlite3 /data/data/com.android.providers.contacts/databases/contacts2.db \'select \"_id\",\"number\",\"date\",\"duration\",\"type\",\"new\",\"name\",\"numbertype\" from calls order by \"_id\"\'";
	pthread_mutex_lock(&popenmutex);
	fprintf(pipeout,"%s\n",cmd.c_str());
	fflush(NULL);


	char buff[LINESIZE];
	char conv[LINESIZE];
	int pos[7];
	int i;
	int j;
	mg_printf(conn,"<?xml version=\"1.0\" encoding=\"utf-8\"?>\n<calls>\n");
	bool first = true;
	while (fgets(buff, 255, pipein) != NULL)
	{
		if (strcmp(buff,"!!!END_OF_POPEN!!!\n")==0)
			break;
		i = 0;
		j = 0;
		while(buff[i] && buff[i] != '\n' && j < 7 && i < 255)
		{
			if (buff[i] == '|')
			{
				buff[i] = 0;
				pos[j++] = i+1;
			}
			i++;
		}
		if (j<7)
			continue;
		while(buff[i]) i++;
		if (i)
			buff[i-1] = 0;
		if (i-1 && buff[i-2] == 13)
			buff[i-2] = 0;
		mg_printf(conn,"<call><id>%s</id><number>%s</number><date>%s</date><duration>%s</duration><type>%s</type><new>%s</new><name>%s</name><numbertype>%s</numbertype></call>\n",buff,buff+pos[0],buff+pos[1],buff+pos[2],buff+pos[3],buff+pos[4],convertxml(conv,buff+pos[5]),buff+pos[6]);
	}
	mg_printf(conn,"</calls>");
	pthread_mutex_unlock(&popenmutex);
}
static void
intent(struct mg_connection *conn,
                const struct mg_request_info *ri, void *data)
{
	if (ri->permissions != PERM_ROOT)
		return;
	lock_wakelock();
	access_log(ri,"start intent");
	send_ok(conn);
	int n = 7;
	std::string call = "/system/bin/am start ";
        while (ri->uri[++n])
	{
		int k = getnum(ri->uri+n);
		while(ri->uri[n] && ri->uri[n] != '_') n++;
		if (k<=0 || 255<k)
			continue;
		char ch = (char)k;
		if ((ch >= 'a' && ch <= 'z') ||
			(ch >= 'A' && ch <= 'Z') || ch == ' ')
			call += ch;
		else
		{
			call += '\\';
			call += ch;
		}
	}
//	printf("%s\n",ri->uri+8);
//	printf("%s\n",call.c_str());
		
	syst(call.c_str());
}
static void
exports(struct mg_connection *conn,
                const struct mg_request_info *ri, void *data)
{
	if (ri->permissions != PERM_ROOT && (ri->permissions&PERM_SMS_CONTACT)==0)
		return;
	lock_wakelock();
	access_log(ri,"export data");
	char buff[LINESIZE];
	char last[LINESIZE];
	char conv[LINESIZE];
	int cols;
	std::string cmd;
	std::string header;
	int datepos=-1;
	int type;
	bool winnewline = false;
	if (ri->uri[8] == '0')
	{
		cmd = dir + "sqlite3 /data/data/com.android.providers.contacts/databases/contacts2.db \'select \"raw_contact_id\",\"mimetype_id\",\"data1\" from data where \"data1\" <> \"\" order by \"raw_contact_id\"\'";
		cols = 3-1;
		header = "contacts";
		type = 0;
	}
	if (ri->uri[8] == '1')
	{
		cmd = dir + "sqlite3 /data/data/com.android.providers.telephony/databases/mmssms.db 'select \"_id\",\"address\",\"person\",\"date\",\"read\",\"status\",\"type\",\"body\" from sms order by \"date\"'";
		cols = 8-1;
		header = "messages";
		datepos = 3;
		type = 1;
	}
	if (ri->uri[8] == '2')
	{
		cmd = dir + "sqlite3 /data/data/com.android.providers.contacts/databases/contacts2.db \'select \"_id\",\"number\",\"date\",\"duration\",\"type\",\"new\",\"name\",\"numbertype\" from calls order by \"_id\"\'";
		cols = 8-1;
		header = "call_list";
		datepos = 2;
		type = 2;
	}
	int format = ri->uri[9]-48;
	int dateformat = ri->uri[10]-48;
	int datesep = ri->uri[11]-48;
	int datein = ri->uri[12]-48;
	if (ri->uri[13] == '1')
		winnewline = true;
	int datetimezone = (ri->uri[14]-48)*10+ri->uri[15]-48-10-12;
	if (format < 0 || format > 2 || dateformat < 0 || dateformat > 2 || datesep < 0 || datesep > 2 || datein < 0 || datein > 1 || datetimezone < -12 || datetimezone > 12)
		return;
	switch(format)
	{
		case 0: header = std::string()+"Content-Type: text/plain; charset=UTF-8\r\nContent-Disposition: attachment;filename="+header+".txt"; break;
		case 1: header = std::string()+"Content-Type: text/csv; charset=UTF-8\r\nContent-Disposition: attachment;filename="+header+".csv"; break;
		case 2: header = std::string()+"Content-Type: text/xml; charset=UTF-8\r\nContent-Disposition: attachment;filename="+header+".xml"; break;
	}
	send_ok(conn,header.c_str());
//	fflush(NULL);
//	if ((in = popen(cmd.c_str(),"r")) == NULL)
//	in = mypopen("/data/data/com.webkey/files/sqlite3 /data/data/com.android.providers.contacts/databases/contacts2.db \'select \"raw_contact_id\",\"mimetype_id\",\"data1\" from data where \"data1\" <> \"\" order by \"raw_contact_id\"\'","r");
	int pos[16];
	int i;
	int j;
	int n;
	std::string mimedata[32];
	int maxmime = 0;
	switch(type)
	{
		case 0: 
			for (i = 0; i < 32; i++)
				if (mimetypes[i].length())
					break;
			if (i==32)
				load_mimetypes();
			for (maxmime = 31; maxmime >= 0; maxmime--)
				if (mimetypes[maxmime].length())
					break;
			maxmime++;

			switch(format)
			{
				case 0: 
					mg_printf(conn,"Contacts:");
					break;
				case 1:
					mg_printf(conn,"id;");
					for (i = 1; i < maxmime; i++)
						if (mimetypes[i].length())
							mg_printf(conn,"%s;",mimetypes[i].c_str());
					if (winnewline)
						mg_printf(conn,"\r\n");
					else
						mg_printf(conn,"\n");
					break;
				case 2:
					if (winnewline)
						mg_printf(conn,"<?xml version=\"1.0\" encoding=\"utf-8\"?>\r\n<people>\r\n");
					else
						mg_printf(conn,"<?xml version=\"1.0\" encoding=\"utf-8\"?>\n<people>\n");
					break;
			}
			break;
		case 1: 
			switch(format)
			{
				case 0: 
					mg_printf(conn,"Messages:");
					break;
				case 1:
					mg_printf(conn,"id;address;person;date;read;status;type;body");
					break;
				case 2:
					if (winnewline)
						mg_printf(conn,"<?xml version=\"1.0\" encoding=\"utf-8\"?>\r\n<messages>\r\n");
					else
						mg_printf(conn,"<?xml version=\"1.0\" encoding=\"utf-8\"?>\n<messages>\n");
					break;
			}
			break;
		case 2: 
			switch(format)
			{
				case 0: 
					if (winnewline)
						mg_printf(conn,"Calls:\r\n\r\n");
					else
						mg_printf(conn,"Calls:\n\n");
					break;
				case 1:
					mg_printf(conn,"id;number;date;duration;type;new;name;numbertype");
					break;
				case 2:
					if (winnewline)
						mg_printf(conn,"<?xml version=\"1.0\" encoding=\"utf-8\"?>\r\n<calls>\r\n");
					else
						mg_printf(conn,"<?xml version=\"1.0\" encoding=\"utf-8\"?>\n<calls>\n");
					break;
			}
			break;
	}
	bool first = true;
	int lastmime = -1;
	pthread_mutex_lock(&popenmutex);
	fprintf(pipeout,"%s\n",cmd.c_str());
	fflush(NULL);
	while (fgets(buff, LINESIZE-1, pipein) != NULL)
	{
		if (strcmp(buff,"!!!END_OF_POPEN!!!\n")==0)
		{
			break;
		}
		i = 0;
		j = 0;
		while(buff[i] && buff[i] != '\n' && j < cols && i < LINESIZE)
		{
			if (buff[i] == '|')
			{
				buff[i] = 0;
				pos[j++] = i+1;
			}
			i++;
		}
		while(buff[i]) i++;
		if (i)
			buff[i-1] = 0;
		if (j<cols)
		{
			switch(format)
			{
				case 0:
					if (winnewline)
						mg_printf(conn,"\r\n%s",buff);
					else
						mg_printf(conn,"\n%s",buff);
					break;
				case 1:
					if (type == 0)
					{
						if (lastmime != -1)
							mimedata[lastmime] = mimedata[lastmime] + removesemicolon(conv,buff);
					}
					else
						mg_printf(conn," %s",removesemicolon(conv,buff));
					break;
				case 2:
					if (winnewline)
						mg_printf(conn,"\r\n%s",convertxml(conv,buff));
					else
						mg_printf(conn,"\n%s",convertxml(conv,buff));
					break;
			}
			continue;
		}
		bool newid = false;
		switch(type)
		{
			case 0: 
				if (strcmp(buff,last))
				{
					if (!first && format==1)
					{
						mg_printf(conn,"%s;",last);
						for (i = 1; i < maxmime; i++)
							mg_printf(conn,"%s;",removesemicolon(conv,mimedata[i].c_str()));
						if (winnewline)
							mg_printf(conn,"\r\n");
						else
							mg_printf(conn,"\n");
					}
					strcpy(last,buff);
					newid = true;
					for (i = 0; i < 32; i++)
						mimedata[i] = "";
				}
				switch(format)
				{
					case 0: 
						if (newid)
						{
							if (winnewline)
								mg_printf(conn,"\r\n\r\n%s\r\n",buff);
							else
								mg_printf(conn,"\n\n%s\n",buff);
						}
						n = getnum(buff+pos[0]);
						if (n>0 && n < 32)
						{
							if (winnewline)
								mg_printf(conn,"%s: %s\r\n",mimetypes[n].c_str(),buff+pos[1]);
							else
								mg_printf(conn,"%s: %s\n",mimetypes[n].c_str(),buff+pos[1]);
							lastmime = n;
						}
						break;
					case 1:
						n = getnum(buff+pos[0]);
						if (n>0 && n < 32)
						{
							if (mimedata[n].length())
								mimedata[n] = mimedata[n] + ", " + (buff+pos[1]);
							else
								mimedata[n] = buff+pos[1];
						}
						break;
					case 2:
						if (lastmime != -1)
							mg_printf(conn,"</%s>",mimetypes[lastmime].c_str());
						if (first)
							mg_printf(conn,"<contact><id>%s</id>",buff);
						else if(newid)
						{
							if (winnewline)
								mg_printf(conn,"</contact>\r\n<contact><id>%s</id>",buff);
							else
								mg_printf(conn,"</contact>\n<contact><id>%s</id>",buff);
						}
						int n = getnum(buff+pos[0]);
						lastmime = n;
						if (n > 0 && n < 32 && mimetypes[n].length() && buff[pos[1]])
						{
							mg_printf(conn,"<%s>%s",mimetypes[n].c_str(),convertxml(conv, buff+pos[1]));
						}
						break;
				}
				break;
			case 1: 
				if (pos[3]>pos[2]+4)
					buff[pos[3]-4]=0;
				switch(format)
				{
					case 0: 
						if (winnewline)
							mg_printf(conn,"\r\n\r\n");
						else
							mg_printf(conn,"\n\n");
						mg_printf(conn,"%s. %s (%s), ",buff,buff+pos[1],buff+pos[0]);
						if (buff[pos[5]] == '1')
							mg_printf(conn,"in, ");
						else 
						if (buff[pos[5]] == '2')
							mg_printf(conn,"out, ");
						mg_printf(conn,humandate(conv,getnum(buff+pos[2]),dateformat,datesep,datein,datetimezone));
						if (winnewline)
							mg_printf(conn,":\r\n");
						else
							mg_printf(conn,":\n");
						mg_printf(conn,"%s",buff+pos[6]);
						break;
					case 1:
						if (winnewline)
							mg_printf(conn,"\r\n");
						else
							mg_printf(conn,"\n");
						mg_printf(conn,"%s;%s;%s;%s;%s;%s;%s",buff,buff+pos[0],buff+pos[1],humandate(conv,getnum(buff+pos[2]),dateformat,datesep,datein,datetimezone),buff+pos[3],buff+pos[4],buff+pos[5]);
						mg_printf(conn,";%s",removesemicolon(conv,buff+pos[6]));
						break;
					case 2:
						if (!first)
						{
							if (winnewline)
								mg_printf(conn,"</body></sms>\r\n");
							else
								mg_printf(conn,"</body></sms>\n");
						}
						mg_printf(conn,"<sms><id>%s</id><address>%s</address><person>%s</person><date>%s</date><read>%s</read><status>%s</status><type>%s</type><body>%s",buff,buff+pos[0],buff+pos[1],buff+pos[2],buff+pos[3],buff+pos[4],buff+pos[5],convertxml(conv,buff+pos[6]));
						break;
				}
				break;
			case 2: 
				if (pos[2]>pos[1]+4)
					buff[pos[2]-4]=0;
				switch(format)
				{
					case 0: 
						if (winnewline)
							mg_printf(conn,"\r\n\r\n");
						else
							mg_printf(conn,"\n\n");
						mg_printf(conn,"%s. %s (%s), ",buff,buff+pos[5],buff+pos[0]);
						if (buff[pos[3]] == '1')
							mg_printf(conn,"in, ");
						else 
						if (buff[pos[3]] == '2')
							mg_printf(conn,"out, ");
						else 
						if (buff[pos[3]] == '3')
							mg_printf(conn,"rejected, ");
						mg_printf(conn,humandate(conv,getnum(buff+pos[1]),dateformat,datesep,datein,datetimezone));
						mg_printf(conn,", ");
						mg_printf(conn,humandur(conv,getnum(buff+pos[2])));
						if (winnewline)
							mg_printf(conn,"\r\n");
						else
							mg_printf(conn,"\n");
						break;
					case 1:
						if (winnewline)
							mg_printf(conn,"\r\n");
						else
							mg_printf(conn,"\n");
						mg_printf(conn,"%s;%s;%s;%s;%s;%s;%s;%s",buff,buff+pos[0],humandate(conv,getnum(buff+pos[1]),dateformat,datesep,datein,datetimezone),buff+pos[2],buff+pos[3],buff+pos[4],buff+pos[5],buff+pos[6]);
						break;
					case 2:
						if (winnewline)
							mg_printf(conn,"<call><id>%s</id><number>%s</number><date>%s</date><duration>%s</duration><type>%s</type><new>%s</new><name>%s</name><numbertype>%s</numbertype></call>\r\n",buff,buff+pos[0],buff+pos[1],buff+pos[2],buff+pos[3],buff+pos[4],convertxml(conv,buff+pos[5]),buff+pos[6]);
						else
							mg_printf(conn,"<call><id>%s</id><number>%s</number><date>%s</date><duration>%s</duration><type>%s</type><new>%s</new><name>%s</name><numbertype>%s</numbertype></call>\n",buff,buff+pos[0],buff+pos[1],buff+pos[2],buff+pos[3],buff+pos[4],convertxml(conv,buff+pos[5]),buff+pos[6]);
						break;
				}
				break;
		}
//		int n = getnum(buff+pos[0]);
//		lastmime = n;
//		if (n > 0 && n < 32 && mimetypes[n].length() && buff[pos[1]])
//		{
//			mg_printf(conn,"<%s>%s",mimetypes[n].c_str(),convertxml(conv, buff+pos[1]));
//		}
		first = false;
	}
//	if (lastmime != -1)
//		mg_printf(conn,"</%s>",mimetypes[lastmime].c_str());
//	if (first)
//		mg_printf(conn,"</people>");
//	else
//		mg_printf(conn,"</contact></people>");
	
	pthread_mutex_unlock(&popenmutex);

	switch(type)
	{
		case 0: 
			switch(format)
			{
				case 0: 
					break;
				case 1:
					if (!first)
					{
						mg_printf(conn,"%s;",last);
						for (i = 0; i < maxmime; i++)
							mg_printf(conn,"%s;",mimedata[i].c_str());
						if (winnewline)
							mg_printf(conn,"\r\n");
						else
							mg_printf(conn,"\n");
					}
					break;
				case 2:
					if (lastmime != -1)
						mg_printf(conn,"</%s></contact>",mimetypes[lastmime].c_str());
					mg_printf(conn,"</people>");
					break;
			}
			break;
		case 1: 
			switch(format)
			{
				case 0: 
					break;
				case 1:
					break;
				case 2:
					if (!first)
					{
						if (winnewline)
							mg_printf(conn,"</body></sms>\r\n");
						else
							mg_printf(conn,"</body></sms>\n");
					}
					mg_printf(conn,"</messages>");
					break;
			}
			break;
		case 2: 
			switch(format)
			{
				case 0: 
					break;
				case 1:
					break;
				case 2:
					mg_printf(conn,"</calls>");
					break;
			}
			break;
	}
	

//	pthread_mutex_lock(&contactsmutex);
//	syst("/system/bin/am broadcast -a \"webkey.intent.action.CONTACTS\"&");
//	pthread_cond_wait(&contactscond,&contactsmutex);
//	mg_write(conn,contacts.c_str(),contacts.length());
//	pthread_mutex_unlock(&contactsmutex);
}
static void
password(struct mg_connection *conn,
                const struct mg_request_info *ri, void *data)
{
	send_ok(conn);
//	if (ri->remote_ip==2130706433 && exit_flag == 0) //localhost
//	{
//		mg_printf(conn,admin_password.c_str());
//	}
}

static std::string update_dyndns(__u32 ip)
{
	if (!ip)
		return "no IP address found";
	int s;
	if ((s = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		//error in opening socket
		return "error opening socket";
	}
	struct sockaddr_in addr;
	struct hostent *hp;
	if ((hp = gethostbyname("members.dyndns.org")) == NULL)
	{
		close(s);
		return "unable to resolve members.dyndns.org";
	}
	bcopy ( hp->h_addr, &(addr.sin_addr.s_addr), hp->h_length);
	addr.sin_port = htons(80);
	addr.sin_family = AF_INET;
	if (connect(s, (struct sockaddr *)&addr, sizeof(struct sockaddr_in)) < 0)
	{
		close(s);
		return "unable to connect members.dyndns.org";
	}
	std::string data = "GET /nic/update?hostname=";
	data += dyndns_host+"&myip=";
	in_addr r;
	r.s_addr = ip;
	data += inet_ntoa(r);
	data += "&wildcard=NOCHG&mx=NOCHG&backmx=NOCHG HTTP/1.0\r\nHost: members.dyndns.org\r\nAuthorization: Basic ";
	data += dyndns_base64;
	data += "\r\nUser-Agent: Webkey\r\n\r\n";
	if (send(s, data.c_str(), data.size(),MSG_NOSIGNAL) < 0)
	{
		close(s);
		return "unable to send data to members.dyndns.org";
	}
	char buf[256];
	bzero(buf, sizeof(buf));
	if (read(s, buf, sizeof(buf)) < 0)
	{
		close(s);
		return "unable to receive data from members.dyndns.org";
	}
	int n = contains(buf,"\r\n\r\n");
	std::string ans;
        if(n!=0)
		ans = (buf+n+3);
	shutdown(s,SHUT_RDWR);
	close(s);
	if (contains(ans.c_str(),"good") || contains(ans.c_str(),"nochg"))
	{
		dyndns_last_updated_ip = ip;
		return "update's ok, dyndns answered: "+ans;
	}
	if (contains(ans.c_str(),"badauth") || contains(ans.c_str(),"!donator") || contains(ans.c_str(),"notfqdn") 
			|| contains(ans.c_str(),"nohost") || contains(ans.c_str(),"numhost") 
			|| contains(ans.c_str(),"abuse") || contains(ans.c_str(),"badagent"))
	{
		dyndns = false;
		return "dyndns rejected, their answer: "+ans;
	}
	return "unknown answer: "+ans;
}

static __u32 ipaddress()
{
	struct ifreq *ifr;
	struct ifconf ifc;
	int numif;
	int s, j;

	memset(&ifc, 0, sizeof(ifc));
	ifc.ifc_ifcu.ifcu_req = NULL;
	ifc.ifc_len = 0;

	if ((s = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		//error in opening socket
		return 0;
	}
	if (ioctl(s, SIOCGIFCONF, &ifc) < 0) {
		perror("ioctl");
		return 0;
	}

	if ((ifr = new ifreq[ifc.ifc_len+2]) == NULL) {
		//error in the number of sockets
		close(s);
		return 0;
	}
	ifc.ifc_ifcu.ifcu_req = ifr;

	if (ioctl(s, SIOCGIFCONF, &ifc) < 0) {
		//error in getting the list
		close(s);
		return 0;
	}

	numif = ifc.ifc_len / sizeof(struct ifreq);
	__u32 ip = 0;
	__u32 ip192 = 0;
	for (j = 0; j < numif; j++)
	{
		struct ifreq *r = &ifr[j];
		struct ifreq c = *r;
		if (ioctl(s, SIOCGIFFLAGS, &c) < 0)
		{
			//error in getting a property
			close(s);
			return 0;
		}
		if ((c.ifr_flags & IFF_UP) && ((c.ifr_flags & IFF_LOOPBACK) == 0)) 
		{
			__u32 t = ((struct sockaddr_in *)&r->ifr_addr)->sin_addr.s_addr;
			if ((t&255) == 192 && ((t>>8)&255) == 168)
				ip192 = t;
			else
				ip = t;
		}

	}
	if (ip == 0)
	{
		ip = ip192;
	}
	delete[] ifr;
	close(s);
	return ip;
}
static void
dyndnsset(struct mg_connection *conn,
                const struct mg_request_info *ri, void *data)
{
	send_ok(conn);
	if (ri->remote_ip!=2130706433 || strcmp(ri->remote_user,"JAVA_CLIENT") != 0) //localhost
	{
		return;
	}
	int n = strlen(ri->uri);
       	if (n == 7)
	{
		dyndns = false;
		mg_printf(conn,"stopped using dyndns\n");
		return;
	}
	dyndns = true;
	dyndns_last_updated_ip = 0;
	dyndns_host = "";
	dyndns_base64 = "";
	int i = 7;
	while (i<n && ri->uri[i]!='&')
	{
		dyndns_host += ri->uri[i];
		i++;
	}
	i++;
	while (i<n)
	{
		dyndns_base64 += ri->uri[i];
		i++;
	}
	__u32 ip = ipaddress();
	mg_printf(conn,"%s",update_dyndns(ip).c_str());
}

static void
status(struct mg_connection *conn,
                const struct mg_request_info *ri, void *data)
{
	send_ok(conn);
//test
//        if (ioctl(fbfd, FBIOGET_VSCREENINFO, &scrinfo) != 0)
//		return;
//	mg_printf(conn,"%d\n",scrinfo.yoffset);

	if (ri->permissions != PERM_ROOT)
		return;
	lock_wakelock();

	if (modelname[0])
		mg_printf(conn,"%s, ",modelname);

	FILE* f;
	double a,b;
	if ((f = fopen("/proc/uptime","r")) && fscanf(f,"%lf %lf\n",&a,&b) == 2)
	{
		int A = (int)a/60;
		int B = (int)b/60;
		mg_printf(conn,"%s: ",lang(ri,"uptime"));
		mg_printf(conn,"<abbr title=\"%s: %d\">",lang(ri,"uptime secs"),(int)a);
		if (A < 2*60)
			mg_printf(conn,"%d %s</abbr>, ",A,lang(ri,"mins"));
		else if (A < 2*24*60)
			mg_printf(conn,"%d %s</abbr>, ",A/60,lang(ri,"hours"));
		else if ((A/60)%24 == 1)
		{
			mg_printf(conn,"%d %s ",A/60/24,lang(ri,"days"));
			mg_printf(conn,"%d %s</abbr>, ",(A/60)%24,lang(ri,"hour"));
		}
		else
		{
			mg_printf(conn,"%d %s ",A/60/24,lang(ri,"days"));
			mg_printf(conn,"%d %s</abbr>, ",(A/60)%24,lang(ri,"hours"));
		}
		if ((int)b)
		{
			mg_printf(conn,"%s: ",lang(ri,"CPUup"));
			mg_printf(conn,"<abbr title=\"%s: %d\">",lang(ri,"CPU uptime secs"),(int)b);
			if (B < 2*60)
				mg_printf(conn,"%d %s</abbr>, ",B,lang(ri,"mins"));
			else if (B < 2*24*60)
				mg_printf(conn,"%d %s</abbr>, ",B/60,lang(ri,"hours"));
			else if ((B/60)%24 == 1)
			{
				mg_printf(conn,"%d %s ",B/60/24,lang(ri,"days"));
				mg_printf(conn,"%d %s</abbr>, ",(B/60)%24,lang(ri,"hour"));
			}
			else
			{
				mg_printf(conn,"%d %s ",B/60/24,lang(ri,"days"));
				mg_printf(conn,"%d %s</abbr>, ",(B/60)%24,lang(ri,"hours"));
			}
			mg_printf(conn,"</abbr>");
		}
	}
	if (f)
		fclose(f);			
	if (f = fopen("/proc/meminfo","r"))
	{
		char name[100];
		int size;
		while(fscanf(f,"%s %d kB\n",&name,&size) == 2)
		{
			if (!strcmp(name,"MemTotal:") || !strcmp(name,"MemFree:") || ((!strcmp(name,"SwapTotal:") || !strcmp(name,"SwapFree:")) && size))
			{
				if (size < 1000)
					mg_printf(conn,"%s <span class=\"gray\">%d</span> kB, ",name,size);
				else
					mg_printf(conn,"%s %d<span class=\"gray\"> %03d</span> kB, ",name,size/1000,size%1000);
			}
		}
		fclose(f);
	}
	if (f = fopen("/proc/net/dev","r"))
	{
		char name[200];
		int bin,bout,pin,pout,t;
		fgets(name,200,f);
		fgets(name,200,f);
		while(fscanf(f,"%s %d %d",&name,&bin,&pin) == 3)
		{
			//if (!strcmp(name,"MemTotal:") || !strcmp(name,"MemFree:") || ((!strcmp(name,"SwapTotal:") || !strcmp(name,"SwapFree:")) && size))
			int i;
			for (i=0; i< 6;i++)
			{
				if (fscanf(f,"%d",&t) != 1)
					break;
			}
			if (i<6)
				break;
			if(fscanf(f,"%d %d",&bout,&pout) != 2)
				break;
			if (bin+bout && strcmp(name,"lo:"))
			{
				if (bin+bout < 1024*1000)
					mg_printf(conn,"<abbr title=\"%s kbytes in: %d, out: %d; packages in: %d, out: %d\">%s</abbr>.<span class=\"gray\">%d</span> kB, ",name,bin>>10,bout>>10,pin,pout,name,(bin+bout)>>10);
				else
					mg_printf(conn,"<abbr title=\"%s kbytes in: %d, out: %d; packages in: %d, out: %d\">%s</abbr> %d<span class=\"gray\"> %03d</span> kB, ",name,bin>>10,bout>>10,pin,pout,name,((bin+bout)>>10)/1000,((bin+bout)>>10)%1000);
			}
			fgets(name,200,f);
		}
		fclose(f);
	}
	
	int fd = open("/sys/class/leds/lcd-backlight/brightness", O_RDONLY);
	char value[20];
	int n;
	if (fd >= 0)
	{
		n = read(fd, value, 10);
		if (n)
			mg_printf(conn,"%s: %d%%, ",lang(ri,"Brightness"),100*getnum(value)/max_brightness);
		close(fd);
	}
	if (recording)
	{
		mg_printf(conn,"<span style=\"color: red\">%s %d ",lang(ri,"Recorded"),recordingnum);
		mg_printf(conn,"%s</span>, ",lang(ri,"images"));
	}
}
static void
sdcard(struct mg_connection *conn,
                const struct mg_request_info *ri, void *data)
{
	if (ri->permissions != PERM_ROOT && (ri->permissions&PERM_SDCARD)==0)
		return;
	lock_wakelock();
	access_log(ri,"browse sdcard");
	send_ok(conn,"Content-Type: text/html; charset=UTF-8\r\nSet-Cookie: path=/");
	FILE* f = fo("sdcard.html","rb");
	if (!f)
		return;
	fseek (f , 0 , SEEK_END);
	int lSize = ftell (f);
	rewind (f);
	char* filebuffer = new char[lSize+1];
	if (!filebuffer)
		return;
	fread(filebuffer,1,lSize,f);
	filebuffer[lSize] = 0;
	mg_write(conn,filebuffer,lSize);
	fclose(f);
	delete[] filebuffer;
}

int remove_directory(const char *path)
{
   DIR *d = opendir(path);
   size_t path_len = strlen(path);
   int r = -1;
   if (d)
   {
      struct dirent *p;
      r = 0;
      while (!r && (p=readdir(d)))
      {
          int r2 = -1;
          char *buf;
          size_t len;
          if (!strcmp(p->d_name, ".") || !strcmp(p->d_name, ".."))
             continue;
	  std::string next = std::string(path) + std::string("/") + std::string(p->d_name);
	  struct stat statbuf;
             if (!stat(next.c_str(), &statbuf))
             {
                if (S_ISDIR(statbuf.st_mode))
                {
                   r2 = remove_directory(next.c_str());
                }
                else
                {
                   r2 = unlink(next.c_str());
                }
             }
          r = r2;
      }
      closedir(d);
   }
   if (!r)
   {
 	  r = rmdir(path);
   }
   return r;
}


static void
content(struct mg_connection *conn,
                const struct mg_request_info *ri, void *data)
{
	if (ri->permissions != PERM_ROOT && (ri->permissions&PERM_SDCARD)==0)
		return;
	lock_wakelock();
	access_log(ri,"browse sdcard");
	char* post_data;
	int post_data_len;
	read_post_data(conn,ri,&post_data,&post_data_len);
//	printf("request: %s\n",ri->query_string);
	//save edited file
	if (ri->query_string == NULL && post_data_len && startswith(post_data,"get_action=put_content"))
	{
		std::string file = getoption(post_data,"file=");
		char to[FILENAME_MAX];
		strcpy(to,file.c_str());
		url_decode(to, strlen(to), to, FILENAME_MAX, true);
		remove_double_dots_and_double_slashes(to);
		std::string code = getoption(post_data,"content=");
		char* c = new char[code.length()+1];
		strcpy(c,code.c_str());
		url_decode(c,strlen(c),c,strlen(c),true);
		mg_printf(conn,"HTTP/1.1 200 OK\r\nCache-Control: no-store, no-cache, must-revalidate\r\nCache-Control: post-check=0, pre-check=0\r\nPragma: no-cache\r\nContent-Type: text/plain;\r\nConnection: close\r\n\r\n");
		if (post_data_len == 65536)
		{
			mg_printf(conn,"The file is too large.");
			delete[] c;
			return;
		}
		FILE* out;
		if (out = fopen(to, "w"))
		{
			fwrite(c, 1, strlen(c), out);
			fclose(out);
			mg_printf(conn,"The file has been saved successfully");
		}
		else
			mg_printf(conn,"1000");

		delete[] c;
		return;
	}
	int i;
	for (i = 0; i < ri->num_headers; i++)
	{
		if (!strncasecmp("Content-Type", ri->http_headers[i].name,12) &&
			!strncasecmp("multipart/form-data", ri->http_headers[i].value,19))
			break;
	}
//	printf("%d\n",__LINE__);
	if (i < ri->num_headers) //non-html5 upload
	{
//		printf("%d\n",__LINE__);
		i = 0;
		while (i < post_data_len && post_data[i] != '\r') i++;
		if (i>=post_data_len) {if (post_data) delete[] post_data; return;}
		post_data[i] = 0;
		std::string boundary = std::string("\r\n") + std::string(post_data);
		i+=2;
//		printf("%d\n",__LINE__);
		if (i>=post_data_len) {if (post_data) delete[] post_data; return;}
//		printf("%s\n",post_data+i);
		if (strncasecmp("Content-Disposition: form-data; name=\"userfile_",post_data+i,47))
			{if (post_data) delete[] post_data; return;}
		i += 47;
//		printf("%d\n",__LINE__);
		while (i < post_data_len && post_data[i] != '_') i++;
		i++; 
		if (i>=post_data_len) {if (post_data) delete[] post_data; return;}
		int start = i;
		while (i < post_data_len && post_data[i] != '"') i++;
		if (i>=post_data_len) {if (post_data) delete[] post_data; return;}
		post_data[i] = 0;
//		printf("%d\n",__LINE__);
		url_decode(post_data+start, strlen(post_data+start), post_data+start, FILENAME_MAX, true);
		std::string dir = post_data+start;
//		printf("dir = %s\n",dir.c_str());
		while (i < post_data_len-4 && strncmp(post_data+i,"filename=\"",10)) i++;
		i += 10;
		start = i;
		while (i < post_data_len-4 && post_data[i] != '"') i++;
		if (i>=post_data_len) {if (post_data) delete[] post_data; return;}
//		printf("%d\n",__LINE__);
		post_data[i] = 0;
		std::string filename = post_data+start;
//		printf("filename = %s\n",filename.c_str());
		while (i < post_data_len-4 && strncmp(post_data+i,"\r\n\r\n",4)) i++;
		i+=4;
		if (i>=post_data_len) {if (post_data) delete[] post_data; return;}
		start = i;
//		printf("%d\n",__LINE__);
		send_ok(conn);
		FILE * f = fopen((dir+"/"+filename).c_str(),"w");
		if (!f)
		{
//		printf("%d\n",__LINE__);
			mg_printf(conn,"<html><script language=\"javascript\">if(parent.ajaxplorer.actionBar.multi_selector) parent.ajaxplorer.actionBar.multi_selector.submitNext('Error opening file for writing');</script></html>\n");
			if (post_data)
				delete[] post_data;
			return;
		}

		int already_read = 65536;
		while(post_data_len == 65536)
		{
			int s = fwrite(post_data+start,1,65536-500-start,f);
//			printf("s = %d\n",s);
//			printf("start = %d\n",start);
			if(s < 65536-500-start)
				{
					mg_printf(conn,"<html><script language=\"javascript\">if(parent.ajaxplorer.actionBar.multi_selector) parent.ajaxplorer.actionBar.multi_selector.submitNext('Error writing. Is disk full?');</script></html>\n");
					fclose(f);
					if (post_data)
						delete[] post_data;
					return;
				}
			start = 0;
			int l = contentlen(conn) - already_read;

			if (l > 65536-500)
				l = 65536-500;
			memcpy(post_data,post_data+65536-500,500);
			post_data_len = mg_read(conn,post_data+500,l) + 500;
			post_data[post_data_len] = 0;
			already_read += post_data_len - 500;
//			printf("post_data_len = %d\n",post_data_len);
		}
		i=0;
		while (i < post_data_len && strncmp(post_data+i,boundary.c_str(),boundary.size())) i++;
		if (i>=post_data_len)
		{
			mg_printf(conn,"<html><script language=\"javascript\">if(parent.ajaxplorer.actionBar.multi_selector) parent.ajaxplorer.actionBar.multi_selector.submitNext('Error, no boundary found at the end of the stream. Strange...');</script></html>\n");
			if (post_data)
				delete[] post_data;
			fclose(f);
			return;
		}
		if(fwrite(post_data+start,1,i-start,f) < i-start)
			{
				mg_printf(conn,"<html><script language=\"javascript\">if(parent.ajaxplorer.actionBar.multi_selector) parent.ajaxplorer.actionBar.multi_selector.submitNext('Error writing. Is disk full?');</script></html>\n");
				fclose(f);
				if (post_data)
					delete[] post_data;
				return;
			}
		fclose(f);
		mg_printf(conn,"<html><script language=\"javascript\">if(parent.ajaxplorer.actionBar.multi_selector) parent.ajaxplorer.actionBar.multi_selector.submitNext();</script></html>\n");
		if (post_data)
			delete[] post_data;
		return;
	}
	if (ri->query_string == NULL)
		return;
	//send_ok(conn);
	FILE* f = NULL;
	if (startswith(ri->query_string,"get_action=save_user_pref")
	|| startswith(ri->query_string,"get_action=switch_repository"))
	{
		sdcard(conn,ri,data);
		return;
	}
	std::string action = getoption(ri->query_string,"action=");
	
	if (action == "get_boot_conf")
	{
		send_ok(conn,"Content-Type: text/javascript; charset=UTF-8");
		f = fo("ae_get_boot_conf","rb");
	}
	if (strcmp(ri->query_string,"get_action=get_template&template_name=flash_tpl.html&pluginName=uploader.flex&encode=false")==0)
	{
		send_ok(conn);
		f = fo("ae_get_template","rb");
		if (f)
		{
			fseek (f , 0 , SEEK_END);
			int lSize = ftell (f);
			rewind (f);
			char* filebuffer = new char[lSize+1];
			if (!filebuffer)
				return;
			fread(filebuffer,1,lSize,f);
			filebuffer[lSize] = 0;
			mg_write(conn,filebuffer,lSize);
			fclose(f);
			delete[] filebuffer;
//			int q;
//			for (q = 0; q < 32; q++)
//			{
//				upload_random[q] = (char)(rand()%10+48);
//			}
//			mg_write(conn,upload_random,32);
			f = NULL;
//			f =  fo("ae_get_template2","rb");
		}
	}
	if (action == "get_xml_registry")
	{
		send_ok(conn,"Content-Type: text/xml; charset=UTF-8");
		f = fo("ae_get_xml_registry","rb");
	}
	if (f)
	{
		fseek (f , 0 , SEEK_END);
		int lSize = ftell (f);
		rewind (f);
		char* filebuffer = new char[lSize+1];
		if (!filebuffer)
			return;
		fread(filebuffer,1,lSize,f);
		filebuffer[lSize] = 0;
		mg_write(conn,filebuffer,lSize);
		fclose(f);
		delete[] filebuffer;
		return;
	}
	if (action == "ls")
	{
//		printf("%s\n",ri->query_string);
		send_ok(conn,"Content-Type: text/xml; charset=UTF-8");
		std::string dp = getoption(ri->query_string,"dir=");
		char dirpath[FILENAME_MAX];
//		if (getoption(ri->query_string,"playlist=") == "true")
//		{
//			dp = base64_decode(dp);
//			strcpy(dirpath, dp.c_str());
//		}
//		else
//		{
			strcpy(dirpath, dp.c_str());
			url_decode(dirpath, strlen(dirpath), dirpath, FILENAME_MAX, true);
//		}
//		printf("%s\n",dirpath);
		if (!startswith(dirpath,"/sdcard"))
		{
			(void) mg_printf(conn, "<?xml version=\"1.0\" encoding=\"UTF-8\"?><tree filename=\"\" text=\"\" is_file=\"false\"><tree is_file=\"false\" filename=\"/sdcard\" text=\"sdcard\" icon=\"folder.png\" mimestring=\"Directory\" is_image=\"0\"/></tree>");
			return;
		}
//			strcpy(dirpath,"/sdcard");
		remove_double_dots_and_double_slashes(dirpath);
//		printf("%s\n",dirpath);

		my_send_directory(conn,dirpath);
	}
	if (action == "download"
	 || action == "preview_data_proxy"
	 || action == "audio_proxy"
	 || action == "get_content"
	 || action == "open_with")
	{
//		printf("%s\n",ri->query_string);
		std::string fp = getoption(ri->query_string,"file=");
		if (action == "audio_proxy")
			fp = base64_decode(fp);
		char filepath[FILENAME_MAX];
		strcpy(filepath, fp.c_str());
		url_decode(filepath, strlen(filepath), filepath, FILENAME_MAX, true);
		if (!startswith(filepath,"/sdcard"))
			return;
		remove_double_dots_and_double_slashes(filepath);
		int i;
		for (i = strlen(filepath)-1; i>=0; i--)
			if (filepath[i]=='/')
				break;
//		printf("%s\n",filepath);
//		printf("%s\n",filepath+i+1);
		if (action=="preview_data_proxy")
		{
			mg_printf(conn,"HTTP/1.1 200 OK\r\nCache-Control: no-store, no-cache, must-revalidate\r\nCache-Control: post-check=0, pre-check=0\r\nPragma: no-cache\r\nContent-Type: image/jpg; name=\"%s\"\r\nConnection: close\r\n\r\n",filepath+i+1);
//			printf("OK\n");
		}
		if (action == "open_with" || action == "get_content")
		{
			mg_printf(conn,"HTTP/1.1 200 OK\r\nCache-Control: no-store, no-cache, must-revalidate\r\nCache-Control: post-check=0, pre-check=0\r\nPragma: no-cache\r\nContent-Type: text/plain;\r\nConnection: close\r\n\r\n");
		}
		if (action == "download")
		{
			mg_printf(conn,"HTTP/1.1 200 OK\r\nCache-Control: no-store, no-cache, must-revalidate\r\nCache-Control: post-check=0, pre-check=0\r\nPragma: no-cache\r\nname=\"%s\"\r\nContent-Disposition: attachment; filename=\"%s\"\r\nConnection: close\r\n\r\n",filepath+i+1,filepath+i+1);
		}
		if (action == "audio_proxy")
		{
			mg_printf(conn,"HTTP/1.1 200 OK\r\nCache-Control: no-store, no-cache, must-revalidate\r\nCache-Control: post-check=0, pre-check=0\r\nPragma: no-cache\r\nContent-Type: audio/mp3: name=\"%s\"\r\nConnection: close\r\n\r\n",filepath+i+1);
		}
		sendfile(filepath,conn);

		return;
	}
	if (action == "rename")
	{
		std::string fp = getoption(ri->query_string,"file=");
		char filepath[FILENAME_MAX];
		strcpy(filepath, fp.c_str());
		url_decode(filepath, strlen(filepath), filepath, FILENAME_MAX, true);
		if (!startswith(filepath,"/sdcard"))
			return;
		remove_double_dots_and_double_slashes(filepath);
		int i;
		for (i = strlen(filepath)-1; i>=0; i--)
			if (filepath[i]=='/')
				break;

		fp = getoption(ri->query_string,"filename_new=");
		char newfile[FILENAME_MAX];
		strcpy(newfile, filepath);
		newfile[i+1] = 0;
		strcat(newfile, fp.c_str());
		url_decode(newfile, strlen(newfile), newfile, FILENAME_MAX, true);
		remove_double_dots_and_double_slashes(newfile);

		int st = rename(filepath, newfile);
		send_ok(conn);
		if (st == 0)
		{
			char buff[LINESIZE];
			convertxml(buff,filepath);
			char buff2[LINESIZE];
			convertxml(buff2,fp.c_str());
			mg_printf(conn,"<?xml version=\"1.0\" encoding=\"UTF-8\"?><tree><message type=\"SUCCESS\">%s has been renamed to %s</message><reload_instruction object=\"data\" node=\"\" file=\"%s\"/></tree>",buff,buff2,buff2);
		}
		else
		{
			char buff[LINESIZE];
			convertxml(buff,filepath);
			char buff2[LINESIZE];
			convertxml(buff2,fp.c_str());
			mg_printf(conn,"<?xml version=\"1.0\" encoding=\"UTF-8\"?><tree><message type=\"ERROR\">Error renaming %s to %s</message></tree>",buff,buff2);
		}
		return;
	}
	if (action == "mkdir")
	{
		std::string fp = getoption(ri->query_string,"dir=");
		if (!startswith(fp.c_str(),"%2Fsdcard"))
			fp = "/sdcard/"+fp;
		char filepath[FILENAME_MAX];
		strcpy(filepath, fp.c_str());
		url_decode(filepath, strlen(filepath), filepath, FILENAME_MAX, true);

		std::string nd = getoption(ri->query_string,"dirname=");
		std::string dirname = fp + '/' + nd;
		char newdir[FILENAME_MAX];
		strcpy(newdir, dirname.c_str());
		url_decode(newdir, strlen(newdir), newdir, FILENAME_MAX, true);
		remove_double_dots_and_double_slashes(newdir);

		int st = mkdir(newdir, 0777);
		send_ok(conn);
		if (st == 0)
		{
			char buff[LINESIZE];
			convertxml(buff,newdir);
			char buff2[LINESIZE];
			convertxml(buff2,nd.c_str());
			mg_printf(conn,"<?xml version=\"1.0\" encoding=\"UTF-8\"?><tree><message type=\"SUCCESS\">The directory %s has been created.</message><reload_instruction object=\"data\" node=\"\" file=\"%s\"/></tree>",buff,buff2);
		}
		else
		{
			char buff[LINESIZE];
			convertxml(buff,newdir);
			mg_printf(conn,"<?xml version=\"1.0\" encoding=\"UTF-8\"?><tree><message type=\"ERROR\">Error creating directory %s</message></tree>",buff);
		}
		return;
	}
	if (action == "mkfile")
	{
		std::string file = getoption(ri->query_string,"dir=") + "/" + getoption(ri->query_string,"filename=");
		if (!startswith(file.c_str(),"%2Fsdcard"))
			file = "/sdcard/"+file;
		
		char filepath[FILENAME_MAX];
		strcpy(filepath, file.c_str());
		url_decode(filepath, strlen(filepath), filepath, FILENAME_MAX, true);
		remove_double_dots_and_double_slashes(filepath);

		FILE* f;
		char buff[LINESIZE];
		convertxml(buff,filepath);
		if (f=fopen(filepath,"a"))
		{
			mg_printf(conn,"<?xml version=\"1.0\" encoding=\"UTF-8\"?><tree><message type=\"SUCCESS\">The file %s has been created.</message><reload_instruction object=\"data\" node=\"\" file=\"%s\"/></tree>",buff,buff);
			fclose(f);
			return;
		}
		mg_printf(conn,"<?xml version=\"1.0\" encoding=\"UTF-8\"?><tree><message type=\"ERROR\">Error creating file %s</message></tree>",buff);
	}
	if (action == "delete" || action == "copy" || action == "move")
	{
		bool copy = (action == "copy");
		bool del = (action == "delete");
		bool move = (action == "move");
		std::string dest = getoption(ri->query_string,"dest=");
		if (!startswith(dest.c_str(),"%2Fsdcard"))
			dest = "/sdcard/"+dest;
		int i = -1;
		std::string file;
		int ok = 0;
		int failed = 0;
		char filepath[FILENAME_MAX];
		char to[FILENAME_MAX];


		while(1)
		{
			if (i == -1)
				file = getoption(ri->query_string,"file=");
			else
			{
				char name[32];
				strcpy(name,"file_");
				char num[32];
				itoa(num,i);
				strcat(name,num);
				strcat(name,"=");
				file = getoption(ri->query_string,name);
			}
			if (file == "" && i != -1)
				break;
			if (file == "" && i == -1)
			{
				i++;
				continue;
			}
			if (!startswith(file.c_str(),"%2Fsdcard"))
				file = "/sdcard/"+file;

			strcpy(filepath, file.c_str());
			url_decode(filepath, strlen(filepath), filepath, FILENAME_MAX, true);
			remove_double_dots_and_double_slashes(filepath);

			if (del)
			{
				  struct stat statbuf;
				  if (!stat(filepath, &statbuf))
				  {
					  int r = -1;
					  if (S_ISDIR(statbuf.st_mode))
						  r = remove_directory(filepath);
					  else
						  r = unlink(filepath);
					  if (!r)
						  ok++;
				  }
				  else
					  ok++; //opera bug
				//if (remove(filepath) == 0)
				//	system((std::string("rm -rf \"")+std::string(filepath)+std::string("\"")).c_str());
			}
			if (move)
			{
				strcpy(to,dest.c_str());
				url_decode(to, strlen(to), to, FILENAME_MAX, true);
				remove_double_dots_and_double_slashes(to);
				int j = strlen(to);
				if (j && to[j-1] != '/')
				{
					to[j++] = '/';
					to[j] = 0;
				}
				int n = strlen(filepath)+1;
				int i = n;
				for (;i>=0 && filepath[i] != '/';i--);
				for(i++; i < n; i++)
					to[j++] = filepath[i];
				to[j++] = 0;
				if (!rename(filepath, to))
					ok++;
//				else
//					printf("AAAAAAAA %s - %s\n",filepath,to);
			}
			if (copy)
			{
//				printf("COPY\n");
				strcpy(to,dest.c_str());
				url_decode(to, strlen(to), to, FILENAME_MAX, true);
				remove_double_dots_and_double_slashes(to);
				int j = strlen(to);
				if (j && to[j-1] != '/')
				{
					to[j++] = '/';
					to[j] = 0;
				}
				int n = strlen(filepath)+1;
				int i = n;
				for (;i>=0 && filepath[i] != '/';i--);
				for(i++; i < n; i++)
					to[j++] = filepath[i];
				to[j++] = 0;
				FILE* in, *out;
				char buff[65526];
				struct stat s;
				if (!stat(filepath,&s) && S_ISREG(s.st_mode) && (in = fopen(filepath, "r")))
				{
					if ((out = fopen(to, "w")))
					{
						while(j = fread(buff, 1, 65536, in))
						{
//							printf("%s - %s %d\n",filepath,to,j);
							fwrite(buff, 1, j, out);
						}
						fclose(out);
						ok++;
					}
					fclose(in);
				}
			}
//			printf("%d\n",i);
			i++;
		}
		char d[FILENAME_MAX];
		strcpy(d,dest.c_str());
		url_decode(d, strlen(d), d, FILENAME_MAX, true);
		remove_double_dots_and_double_slashes(d);
		if (i == 0 && del)
		{
			char buff[LINESIZE];
			convertxml(buff,filepath);
			if (ok == 0)
				mg_printf(conn,"<?xml version=\"1.0\" encoding=\"UTF-8\"?><tree><message type=\"ERROR\">Error deleting file or directory %s</message></tree>",buff);
			else
				mg_printf(conn,"<?xml version=\"1.0\" encoding=\"UTF-8\"?><tree><message type=\"SUCCESS\">The file or directory %s has been deleted.</message><reload_instruction object=\"data\" node=\"\"/></tree>",buff);
		}
		if (i == 0 && move)
		{
			char buff[LINESIZE];
			convertxml(buff,filepath);
			char buff2[LINESIZE];
			convertxml(buff2,to);
			char buff3[LINESIZE];
			convertxml(buff3,d);
			if (ok == 0)
				mg_printf(conn,"<?xml version=\"1.0\" encoding=\"UTF-8\"?><tree><message type=\"ERROR\">Error moving file or directory %s to %s.</message></tree>",buff,buff2);
			else
				mg_printf(conn,"<?xml version=\"1.0\" encoding=\"UTF-8\"?><tree><message type=\"SUCCESS\">The file or directory %s has been moved to %s.</message><reload_instruction object=\"data\" node=\"\"/><reload_instruction object=\"data\" node=\"%s\"/></tree>",buff,buff2,buff3);
		}
		if (i == 0 && copy)
		{
			char buff[LINESIZE];
			convertxml(buff,filepath);
			char buff2[LINESIZE];
			convertxml(buff2,to);
			char buff3[LINESIZE];
			convertxml(buff3,d);
			if (ok == 0)
				mg_printf(conn,"<?xml version=\"1.0\" encoding=\"UTF-8\"?><tree><message type=\"ERROR\">Error copying file or directory %s to %s.</message></tree>",buff,buff2);
			else
				mg_printf(conn,"<?xml version=\"1.0\" encoding=\"UTF-8\"?><tree><message type=\"SUCCESS\">The file or directory %s has been copied to %s.</message><reload_instruction object=\"data\" node=\"\"/><reload_instruction object=\"data\" node=\"%s\"/></tree>",buff,buff2,buff3);
		}
		if (i>0 && del)
		{
			if (ok != i)
				mg_printf(conn,"<?xml version=\"1.0\" encoding=\"UTF-8\"?><tree><message type=\"ERROR\">Error deleting some files or directories</message><reload_instruction object=\"data\" node=\"\"/></tree>");
			else
				mg_printf(conn,"<?xml version=\"1.0\" encoding=\"UTF-8\"?><tree><message type=\"SUCCESS\">The selected files and directories have been deleted.</message><reload_instruction object=\"data\" node=\"\"/></tree>");
		}
		if (i>0 && move)
		{
			char buff3[LINESIZE];
			convertxml(buff3,d);
			if (ok != i)
				mg_printf(conn,"<?xml version=\"1.0\" encoding=\"UTF-8\"?><tree><message type=\"ERROR\">Error moving some files or directories</message><reload_instruction object=\"data\" node=\"\"/><reload_instruction object=\"data\" node=\"%s\"/></tree>",buff3);
			else
				mg_printf(conn,"<?xml version=\"1.0\" encoding=\"UTF-8\"?><tree><message type=\"SUCCESS\">The selected files and directories have been moved.</message><reload_instruction object=\"data\" node=\"\"/><reload_instruction object=\"data\" node=\"%s\"/></tree>",buff3);
		}
		if (i>0 && copy)
		{
			char buff3[LINESIZE];
			convertxml(buff3,d);
			if (ok != i)
				mg_printf(conn,"<?xml version=\"1.0\" encoding=\"UTF-8\"?><tree><message type=\"ERROR\">Error copying some files or directories</message><reload_instruction object=\"data\" node=\"\"/><reload_instruction object=\"data\" node=\"%s\"/></tree>",buff3);
			else
				mg_printf(conn,"<?xml version=\"1.0\" encoding=\"UTF-8\"?><tree><message type=\"SUCCESS\">The selected files and directories have been copied.</message><reload_instruction object=\"data\" node=\"\"/><reload_instruction object=\"data\" node=\"%s\"/></tree>",buff3);
		}
	}
	if (action == "upload")
	{
		//<html><script language="javascript">
		//
		// if(parent.ajaxplorer.actionBar.multi_selector) parent.ajaxplorer.actionBar.multi_selector.submitNext();</script></html>
		

		//send_ok(conn);
/*		mg_printf(conn,"HTTP/1.1 100 Continue\r\n\r\n");
//		printf("%s\n",ri->post_data);
		int i = 0;
		while(i < post_data_len && post_data[i]!='\n') i++; i++;
		while(i < post_data_len && post_data[i]!='\n') i++; i++;
		while(i < post_data_len && post_data[i]!='\n') i++; i++;
		int start = i;
		while(i < post_data_len && post_data[i]!='\r') i++;
		if (i >= post_data_len)
		{
			mg_printf(conn,"<?xml version=\"1.0\" encoding=\"UTF-8\"?><tree><message type=\"ERROR\">Upload failed.</message><reload_instruction object=\"data\" node=\"\"/></tree>");
			return;
		}
		post_data[i] = 0;
		std::string file = (post_data + start);
		i+=2;
		while(i < post_data_len && post_data[i]!='\n') i++; i++;
		while(i < post_data_len && post_data[i]!='\n') i++; i++;
		while(i < post_data_len && post_data[i]!='\n') i++; i++;
		start = i;
		while(i < post_data_len && post_data[i]!='\r') i++;
		if (i >= post_data_len)
		{
			mg_printf(conn,"<?xml version=\"1.0\" encoding=\"UTF-8\"?><tree><message type=\"ERROR\">Upload failed.</message><reload_instruction object=\"data\" node=\"\"/></tree>");
			return;
		}
		post_data[i] = 0;
		std::string dir = base64_decode(post_data + start);
		i+=2;
		if (!startswith(dir.c_str(),"/sdcard"))
			dir = "/sdcard/"+dir;
		file = dir+'/'+file;
		char filepath[FILENAME_MAX];
		if (file.size() >= FILENAME_MAX-1)
		{
			mg_printf(conn,"<?xml version=\"1.0\" encoding=\"UTF-8\"?><tree><message type=\"ERROR\">Upload failed.</message><reload_instruction object=\"data\" node=\"\"/></tree>");
			return;
		}
		strcpy(filepath,file.c_str());
		remove_double_dots_and_double_slashes(filepath);
//		printf("path = %s\n",filepath);
		while(i < post_data_len && post_data[i]!='\n') i++; i++;
		while(i < post_data_len && post_data[i]!='\n') i++; i++;
		while(i < post_data_len && post_data[i]!='\n') i++; i++;
		while(i < post_data_len && post_data[i]!='\n') i++; i++;
		if (i >= post_data_len)
		{
			mg_printf(conn,"<?xml version=\"1.0\" encoding=\"UTF-8\"?><tree><message type=\"ERROR\">Upload failed.</message><reload_instruction object=\"data\" node=\"\"/></tree>");
			return;
		}
		start = i;
		FILE* f = fopen(filepath,"w");
		if (!f)
		{
			mg_printf(conn,"<?xml version=\"1.0\" encoding=\"UTF-8\"?><tree><message type=\"ERROR\">Upload failed.</message><reload_instruction object=\"data\" node=\"\"/></tree>");
			return;
		}


		int already_read = 65536;
		while(post_data_len == 65536)
		{
			fwrite(post_data+start,1,65536-500-start,f);
			start = 0;
			int l = contentlen(conn) - already_read;

			if (l > 65536-500)
				l = 65536-500;
			memcpy(post_data,post_data+65536-500,500);
			post_data_len = mg_read(conn,post_data+500,l) + 500;
			post_data[post_data_len] = 0;
			already_read += post_data_len - 500;
		}

		i = post_data_len-1;
		while(i >= 0 && post_data[i]!='\n') i--; i--;
		while(i >= 0 && post_data[i]!='\n') i--; i--;
		while(i >= 0 && post_data[i]!='\n') i--; i--;
		while(i >= 0 && post_data[i]!='\n') i--; i--;
		while(i >= 0 && post_data[i]!='\n') i--; i--;
		while(i >= 0 && post_data[i]!='\n') i--; i--;
		if (i < start)
		{
			mg_printf(conn,"<?xml version=\"1.0\" encoding=\"UTF-8\"?><tree><message type=\"ERROR\">Upload failed.</message><reload_instruction object=\"data\" node=\"\"/></tree>");
			return;
		}
		fwrite(post_data+start,1,i-start,f);
		fclose(f);
		mg_printf(conn,"<?xml version=\"1.0\" encoding=\"UTF-8\"?><tree><message type=\"SUCCESS\">The file uploaded to %s.</message><reload_instruction object=\"data\" node=\"\"/></tree>",filepath);
//		printf("upload OK\n");
*/
		send_ok(conn);
		std::string dir = getoption(ri->query_string,"dir=");
		char to[FILENAME_MAX];
		strcpy(to,dir.c_str());
		url_decode(to, strlen(to), to, FILENAME_MAX, true);
		int i;
		std::string file;
		for (i = 0; i < ri->num_headers; i++)
			if (!strcasecmp("X-File-Name", ri->http_headers[i].name))
			{
				file = ri->http_headers[i].value;
				break;
			}
		if (i == ri->num_headers)
		{
			mg_printf(conn,"There should be an X-File-Name in the HTTP header list.");
			return;
		}
		if (!startswith(to,"/sdcard"))
			file = std::string("/sdcard/")+std::string(to)+'/'+file;
		else
			file = std::string(to)+'/'+file;
		char filepath[FILENAME_MAX];
		if (file.size() >= FILENAME_MAX-1)
		{
			mg_printf(conn,"Too long filename");
			return;
		}
		strcpy(filepath,file.c_str());
		remove_double_dots_and_double_slashes(filepath);
		//printf("%s\n",filepath);
		FILE* f = fopen(filepath,"w");
		if (!f)
		{
			mg_printf(conn,"Bad Request3");
			return;
		}


		int already_read = 0;
		while(post_data_len)
		{
			already_read += post_data_len;
			fwrite(post_data,1,post_data_len,f);
			int l = contentlen(conn) - already_read;
			if (l > 65536)
				l = 65536;
			post_data_len = mg_read(conn,post_data,l);
			post_data[post_data_len] = 0;
		}
		fwrite(post_data,1,post_data_len,f);
		fclose(f);
		mg_printf(conn,"OK");
	}
	if (post_data)
		delete[] post_data;
}

static void
testfb(struct mg_connection *conn,
                const struct mg_request_info *ri, void *data)
{
	if (ri->permissions != PERM_ROOT)
		return;
	lock_wakelock();
	access_log(ri,"run testfb");
	send_ok(conn);

        size_t pixels;

//        pixels = scrinfo.xres_virtual * scrinfo.yres_virtual;
        pixels = scrinfo.xres * scrinfo.yres;
        bytespp = scrinfo.bits_per_pixel / 8;
	if (bytespp == 3)
		bytespp = 4;

        mg_printf(conn,"We might read from unused memory, it might happen that Webkey gets stopped.\n");
        mg_printf(conn,"xres=%d, yres=%d, xresv=%d, yresv=%d, xoffs=%d, yoffs=%d, bpp=%d\n",
          (int)scrinfo.xres, (int)scrinfo.yres,
          (int)scrinfo.xres_virtual, (int)scrinfo.yres_virtual,
          (int)scrinfo.xoffset, (int)scrinfo.yoffset,
          (int)scrinfo.bits_per_pixel);



	int rr = scrinfo.red.offset;
	int rl = 8-scrinfo.red.length;
	int gr = scrinfo.green.offset;
	int gl = 8-scrinfo.green.length;
	int br = scrinfo.blue.offset;
	int bl = 8-scrinfo.blue.length;
	int j;
	mg_printf(conn,"rr=%d, rl=%d, gr=%d, gl=%d, br=%d, bl=%d\n",rr,rl,gr,gl,br,bl);

	mg_printf(conn,"nonstd=%d\n",scrinfo.nonstd);
	mg_printf(conn,"activate=%d\n",scrinfo.activate);
	mg_printf(conn,"height=%d\n",scrinfo.height);
	mg_printf(conn,"width=%d\n",scrinfo.width);
	mg_printf(conn,"accel_flags=%d\n",scrinfo.accel_flags);
	mg_printf(conn,"pixclock=%d\n",scrinfo.pixclock);
	mg_printf(conn,"left_margin=%d\n",scrinfo.left_margin);
	mg_printf(conn,"right_margin=%d\n",scrinfo.right_margin);
	mg_printf(conn,"upper_margin=%d\n",scrinfo.upper_margin);
	mg_printf(conn,"lower_margin=%d\n",scrinfo.lower_margin);
	mg_printf(conn,"hsync_len=%d\n",scrinfo.hsync_len);
	mg_printf(conn,"vsync_len=%d\n",scrinfo.vsync_len);
	mg_printf(conn,"sync=%d\n",scrinfo.sync);
	mg_printf(conn,"vmode=%d\n",scrinfo.vmode);
	mg_printf(conn,"rotate=%d\n",scrinfo.rotate);
	mg_printf(conn,"reserved[0]=%d\n",scrinfo.reserved[0]);
	mg_printf(conn,"reserved[1]=%d\n",scrinfo.reserved[1]);
	mg_printf(conn,"reserved[2]=%d\n",scrinfo.reserved[2]);
	mg_printf(conn,"reserved[3]=%d\n",scrinfo.reserved[3]);
	mg_printf(conn,"reserved[4]=%d\n",scrinfo.reserved[4]);

	int m = scrinfo.yres*scrinfo.xres_virtual;
	int offset = scrinfo.yoffset*scrinfo.xres_virtual+scrinfo.xoffset;

	if (!pict)
		init_fb();
	memcpy(copyfb,(char*)fbmmap+offset*bytespp,m*bytespp);

	mg_printf(conn,"offset*bytespp=%d\n",offset*bytespp);
	mg_printf(conn,"m*bytespp=%d\n",m*bytespp);

	if (!graph)
		init_fb();
	if (!graph)
	{
		mg_printf(conn,"graph is null\n");
		return;
	}

	int i;
	unsigned short int* map = (unsigned short int*)fbmmap;
	unsigned short int* map2 = (unsigned short int*)copyfb;
	for (i = 0; i < 500; i++)
	{
		mg_printf(conn,"map[%d] is %d\n",100*i+i,map[100*i+i]);
		mg_printf(conn,"map2[%d] is %d\n",100*i+i,map2[100*i+i]);
	}
		
}

static void
testtouch(struct mg_connection *conn,
                const struct mg_request_info *ri, void *data)
{
	if (ri->permissions != PERM_ROOT)
		return;
	lock_wakelock();
	access_log(ri,"run testtouch");
	send_ok(conn);
	int i;
#ifdef MYDEB
	char touch_device[26] = "/android/dev/input/event0";
#else
	char touch_device[18] = "/dev/input/event0";
#endif
	int tfd = -1;
	for (i=0; i<10; i++)
	{
		char name[256]="Unknown";
		touch_device[sizeof(touch_device)-2] = '0'+(char)i;
		struct input_absinfo info;
		if((tfd = open(touch_device, O_RDWR)) == -1)
		{
			continue;
		}
		mg_printf(conn,"searching for touch device, opening %s ... ",touch_device);
		if (ioctl(tfd, EVIOCGNAME(sizeof(name)),name) < 0)
		{
			mg_printf(conn,"failed, no name\n");
			close(tfd);
			tfd = -1;
			continue;
		}
		mg_printf(conn,"device name is %s\n",name);
		//
		// Get the Range of X and Y
		if(ioctl(tfd, EVIOCGABS(ABS_X), &info))
		{
			printf("failed, no ABS_X\n");
			close(tfd);
			tfd = -1;
			continue;
		}
		xmin = info.minimum;
		xmax = info.maximum;
		if (xmin == 0 && xmax == 0)
		{
			if(ioctl(tfd, EVIOCGABS(53), &info))
			{
				printf("failed, no ABS_X\n");
				close(tfd);
				tfd = -1;
				continue;
			}
			xmin = info.minimum;
			xmax = info.maximum;
		}

		if(ioctl(tfd, EVIOCGABS(ABS_Y), &info)) {
			printf("failed, no ABS_Y\n");
			close(tfd);
			tfd = -1;
			continue;
		}
		ymin = info.minimum;
		ymax = info.maximum;
		if (ymin == 0 && ymax == 0)
		{
			if(ioctl(tfd, EVIOCGABS(54), &info))
			{
				printf("failed, no ABS_Y\n");
				close(tfd);
				tfd = -1;
				continue;
			}
			ymin = info.minimum;
			ymax = info.maximum;
		}
		bool t = contains(name,"touch");
		bool tk = contains(name,"touchkey");
		if (t && !tk)
			printf("there is \"touch\", but not \"touchkey\" in the name\n");
		if (!(t && !tk) && (xmin < 0 || xmin == xmax))	// xmin < 0 for the compass
		{
			printf("failed, xmin<0 || xmin==xmax\n");
			close(tfd);
			tfd = -1;
			continue;
		}
		mg_printf(conn,"we choose this one\n");
		__android_log_print(ANDROID_LOG_INFO,"Webkey C++","using touch device: %s",touch_device);
		ymin = info.minimum;
		ymax = info.maximum;
		mg_printf(conn,"xmin = %d, xmax = %d, ymin = %d, ymax = %d\n",xmin,xmax,ymin,ymax);
	}
	mg_printf(conn,"Please use the touchscreen on the phone for a while.\n");
	for (i = 0; i < 100; i++)
	{
		struct input_event ev;
		if(read(touchfd, &ev, sizeof(ev)) < 0)
			mg_printf(conn,"touchfd read failed.\n");
		else
		{
			mg_printf(conn,"ev.type = %d, ev.code = %d, ev.value = %d\n",ev.type,ev.code,ev.value);
		}
	}
	mg_printf(conn,"Finished.\n");
}

static void read_prefs()
{
	std::string sharedpref = dir + "../shared_prefs/com.webkey_preferences.xml";
	server = false;
	dyndns = false;
	FILE* sp = fopen(sharedpref.c_str(),"r");
	if (sp)
	{
		char buff[256];
		while (fgets(buff, sizeof(buff)-1, sp) != NULL)
		{
			int n = strlen(buff);
			if (startswith(buff,"<string name=\"dddomain\">"))
			{
				int i = 0;
				for(i=24;i<n;i++)
					if (buff[i]=='<')
					{
						buff[i]=0;
						break;
					}
				dyndns_base64 = (buff+24);
			}
			if (startswith(buff,"<string name=\"hash\">"))
			{
				int i = 0;
				for(i=20;i<n;i++)
					if (buff[i]=='<')
					{
						buff[i]=0;
						break;
					}
				dyndns_base64 = (buff+20);
			}
			if (startswith(buff,"<boolean name=\"ddusing\" value=\"true\""))
			{
				dyndns = true;
			}
			if (startswith(buff,"<string name=\"port\">"))
			{
				int i = 0;
				for(i=20;i<n;i++)
					if (buff[i]=='<')
					{
						buff[i]=0;
						break;
					}
				port = strtol(buff+20, 0, 10);
			}
			if (startswith(buff,"<string name=\"sslport\">"))
			{
				int i = 0;
				for(i=23;i<n;i++)
					if (buff[i]=='<')
					{
						buff[i]=0;
						break;
					}
				sslport = strtol(buff+23, 0, 10);
			}
			if (startswith(buff,"<string name=\"random\">"))
			{
				int i = 0;
				for(i=22;i<n;i++)
					if (buff[i]=='<')
					{
						buff[i]=0;
						break;
					}
				if (!server_random || strcmp(server_random,buff+22))
				{
					char * t = server_random;
					server_random = new char[n];
					strcpy(server_random,buff+22);
					if (t)
						delete[] t;
				}
			}
			if (startswith(buff,"<string name=\"username\">"))
			{
				int i = 0;
				for(i=24;i<n;i++)
					if (buff[i]=='<')
					{
						buff[i]=0;
						break;
					}
				if (!server_username || strcmp(server_username,buff+24))
				{
					char * t = server_username;
					server_username = new char[n];
					strcpy(server_username,buff+24);
					if (t)
						delete[] t;
				}
			}
			if (startswith(buff,"<boolean name=\"server\" value=\"true\""))
			{
				server = true;
			}
		}
		fclose(sp);
	}
}

static void
reread(struct mg_connection *conn,
                const struct mg_request_info *ri, void *data)
{
	send_ok(conn);
	if (ri->remote_ip!=2130706433 || strcmp(ri->remote_user,"JAVA_CLIENT") != 0) //localhost
	{
		printf("reread request failed, permission's denied.\n");
		return;
	}
	read_prefs();
	server_changes++;
}
static void
test(struct mg_connection *conn,
                const struct mg_request_info *ri, void *data)
{
    mg_printf(conn,"Webkey");
}
static void
javatest(struct mg_connection *conn,
                const struct mg_request_info *ri, void *data)
{
    send_ok(conn);
    mg_printf(conn,"Webkey");
}

static void
check_notify()
{
	struct timeval t;
	gettimeofday(&t, NULL);

	if (notify.lastalarm + 60*notify.interval > t.tv_sec)
		return;

	bool oldstate = (notify.smson && notify.smsalarmed) || (notify.callon && notify.callalarmed);
	struct stat s;
	if (notify.smson && !stat("/data/data/com.android.providers.telephony/databases/mmssms.db",&s) && s.st_mtime != notify.lastsmstime)
	{
		notify.lastsmstime = s.st_mtime;
		std::string cmd = dir + "sqlite3 /data/data/com.android.providers.telephony/databases/mmssms.db 'select \"_id\",\"read\",\"type\" from sms where \"type\"==1 and \"read\"==0'";
		pthread_mutex_lock(&popenmutex);
		fprintf(pipeout,"%s\n",cmd.c_str());
		fflush(NULL);
		char buff[LINESIZE];
		notify.smsalarmed = false;
		while (fgets(buff, LINESIZE-1, pipein) != NULL)
		{
			if (strcmp(buff,"!!!END_OF_POPEN!!!\n")==0)
				break;
			else
				notify.smsalarmed = true;
		}
		pthread_mutex_unlock(&popenmutex);
	}
	if (notify.callon && !stat("/data/data/com.android.providers.contacts/databases/contacts2.db",&s) && s.st_mtime != notify.lastcalltime)
	{
		notify.lastcalltime = s.st_mtime;
		std::string cmd = dir + "sqlite3 /data/data/com.android.providers.contacts/databases/contacts2.db \'select \"_id\",\"type\",\"new\" from calls where \"type\"==3 and \"new\"==1\'";
		pthread_mutex_lock(&popenmutex);
		fprintf(pipeout,"%s\n",cmd.c_str());
		fflush(NULL);
		char buff[LINESIZE];
		notify.callalarmed = false;
		while (fgets(buff, LINESIZE-1, pipein) != NULL)
		{
			if (strcmp(buff,"!!!END_OF_POPEN!!!\n")==0)
				break;
			else
				notify.callalarmed = true;
		}
		pthread_mutex_unlock(&popenmutex);
	}
	if ((notify.smson && notify.smsalarmed) || (notify.callon && notify.callalarmed))
	{
		if (notify.blink && notify.blinktype == 1)
			set_blink(1,notify.blinkon,notify.blinkoff);

		if (oldstate)
		{
			if (notify.blink && notify.blinktype == 0)
				set_blink(1,notify.blinkon,notify.blinkoff);
			if (notify.vibrate)
			{
				lock_wakelock();
				int v, p, i;
				p = 0; v = 0;
				i = -1;
				bool b;
				b = false;
				while (i < notify.vibratepatt.size() || i == -1)
				{
					if (i == -1 || notify.vibratepatt[i] == '~')
					{
						p = v;
						v = getnum(notify.vibratepatt.c_str()+i+1);
						b = !b;
						if (b)
							vibrator_on(v);
						else
							usleep((p+v)*1000);
					}
					i++;
				}
				if (!b && notify.blink && notify.blinktype == 0)
					usleep(v*1000);
			}
			if (notify.blink && notify.blinktype == 0)
				set_blink(0,0,0);
		}
		notify.lastalarm = t.tv_sec;
	}
	else
		if (oldstate && notify.blink && notify.blinktype == 1)
			set_blink(0,0,0);
}
//from ShellInABox
static char *jsonEscape(const char *buf, int len) {
	static const char *hexDigit = "0123456789ABCDEF";
  // Determine the space that is needed to encode the buffer
  int count                   = 0;
  const char *ptr             = buf;
  for (int i = 0; i < len; i++) {
    unsigned char ch          = *(unsigned char *)ptr++;
    if (ch < ' ') {
      switch (ch) {
      case '\b': case '\f': case '\n': case '\r': case '\t':
        count                += 2;
        break;
      default:
        count                += 6;
        break;
      }
    } else if (ch == '"' || ch == '\\' || ch == '/') {
      count                  += 2;
    } else if (ch > '\x7F') {
      count                  += 6;
    } else {
      count++;
    }
  }

  // Encode the buffer using JSON string escaping
  char *result;
  result                = new char[count + 1];
  char *dst                   = result;
  ptr                         = buf;
  for (int i = 0; i < len; i++) {
    unsigned char ch          = *(unsigned char *)ptr++;
    if (ch < ' ') {
      *dst++                  = '\\';
      switch (ch) {
      case '\b': *dst++       = 'b'; break;
      case '\f': *dst++       = 'f'; break;
      case '\n': *dst++       = 'n'; break;
      case '\r': *dst++       = 'r'; break;
      case '\t': *dst++       = 't'; break;
      default:
      unicode:
        *dst++                = 'u';
        *dst++                = '0';
        *dst++                = '0';
        *dst++                = hexDigit[ch >> 4];
        *dst++                = hexDigit[ch & 0xF];
        break;
      }
    } else if (ch == '"' || ch == '\\' || ch == '/') {
      *dst++                  = '\\';
      *dst++                  = ch;
    } else if (ch > '\x7F') {
      *dst++                  = '\\';
      goto unicode;
    } else {
      *dst++                  = ch;
    }
  }
  *dst++                      = '\000';
  return result;
}

// from Android-Terminal-Emulator, check
// http://github.com/jackpal/Android-Terminal-Emulator/blob/master/jni/termExec.cpp
static int create_subprocess(pid_t* pProcessId)
{
    char *devname;
   // int ptm;
    pid_t pid;

    int ptm = open("/dev/ptmx", O_RDWR); // | O_NOCTTY);
    if(ptm < 0){
        printf("[ cannot open /dev/ptmx - %s ]\n",strerror(errno));
        return -1;
    }
    fcntl(ptm, F_SETFD, FD_CLOEXEC);

    if(grantpt(ptm) || unlockpt(ptm) ||
       ((devname = (char*) ptsname(ptm)) == 0)){
        printf("[ trouble with /dev/ptmx - %s ]\n", strerror(errno));
        return -1;
    }


    pid = fork();
    if(pid < 0) {
        printf("- fork failed: %s -\n", strerror(errno));
        return -1;
    }

    if(pid == 0){
	    close(ptm);
	    int pts;
	    setsid();
	    pts = open(devname, O_RDWR);
        if(pts < 0) exit(-1);

	fflush(NULL);
        dup2(pts, 0);
        dup2(pts, 1);
        dup2(pts, 2);

	char** env = new char*[6];
	env[0] = "TERM=xterm";
	env[1] = "LINES=25";
	env[2] = "COLUMNS=80";
	env[3] = 0;
//	env[1] = "HOME=/sdcard";
	env[3] = "HOME=/sdcard";
	env[4] = "PATH=/data/local/bin:/usr/bin:/usr/sbin:/bin:/sbin:/system/bin:/system/xbin:/system/xbin/bb:/system/sbin";
	env[5] = 0;
        execle("/system/bin/bash", "/system/bin/bash", NULL, env);
        execle("/system/xbin/bash", "/system/xbin/bash", NULL, env);
        execle("/system/xbin/bb/bash", "/system/xbin/bb/bash", NULL, env);
        execle("/system/bin/sh", "/system/bin/sh", NULL, env);
//        execle("/bin/bash", "/bin/bash", NULL, env);
        exit(-1);
    } else {
	usleep(100000);
        *pProcessId = pid;
	//char buf[200];
//	write(ptm,"# Closed after 60 minutes of inactivity.\n",8);
        return ptm;
    }
}

static void
shellinabox(struct mg_connection *conn,
                const struct mg_request_info *ri, void *data)
{
	if (ri->permissions != PERM_ROOT)
		return;
	access_log(ri,"run terminal");
	char* post_data;
	int post_data_len;
	read_post_data(conn,ri,&post_data,&post_data_len);
	int i = 0;
	std::string param;
	std::string value;
	int p = 0;
	int width = 0;
	int height = 0;
	std::string sessionstr;
	std::string keys;
	bool keys_received = false;
	bool root_url = false;
	while (true)
	{
		if (i== post_data_len || post_data[i] == '&')
		{
			p = 0;
			if (param == "width")
				width = atoi(value.c_str());
			else if (param == "height")
				height = atoi(value.c_str());
			else if (param == "session")
				sessionstr = value;
			else if (param == "keys")
			{
				keys = value;
				keys_received = true;
			}
			else if (param == "rooturl")
				root_url = true;

			param = "";
			value = "";
		}
		else
		if (post_data[i] == '=')
		{
			p = 1;
		}
		else
		{
			if (p)
				value += post_data[i];
			else
				param += post_data[i];
		}
		if (i == post_data_len)
			break;
		i++;
	}
	send_ok(conn,"Content-type: application/json; charset=utf-8");
	SESSION* session = NULL;
	int sessionid = -1;
	int ptm = 0;
	if (sessionstr.length())
		sessionid = atoi(sessionstr.c_str());
	if ( root_url )
	{
		int pid;
		while (1)
		{
			ptm = create_subprocess(&pid);
			char buf[6];
			fd_set set;
			struct timeval timeout;
			FD_ZERO(&set);
			FD_SET(ptm, &set);
			timeout.tv_sec = 0;
			timeout.tv_usec = 100;
			int s = select(FD_SETSIZE, &set, NULL, NULL, &timeout);
			if (s>0)
			{
				read(ptm,buf,6);
				break;
			}
			kill(pid,SIGKILL);
		}
		session = new SESSION;
		session->pid = pid;
		session->ptm = ptm;
		session->oldwidth = 180;
		session->oldheight = 25;
		session->alive = true;
		struct timeval tv;
		gettimeofday(&tv,0);
		session->lastused = tv.tv_sec;
		pthread_mutex_init(&(session->mutex), NULL);
		sessions.push_back(session);
		if (ptm >= 0)
			mg_printf(conn,"{\"session\":\"%d\",\"data\":\"\"}\n",sessions.size()-1);
		//	mg_printf(conn,"{\"session\":\"%d\",\"data\":\"%s\\r\\n\"}\n",sessions.size()-1,lang(ri,"This terminal will be closed after 60 minutes of inactivity."));
		printf("terminal started\n");
		if (post_data)
			delete[] post_data;
		return;
	}
	if (sessionid == -1 || sessionid >= sessions.size())
	{
		printf("Wrong sessionid\n");
		if (post_data)
			delete[] post_data;
		return;
	}
	session = sessions[sessionid];
	if (session->alive == false)
	{
		if (post_data)
			delete[] post_data;
		return;
	}
	struct timeval tv;
	gettimeofday(&tv,0);
	session->lastused = tv.tv_sec;

	if (width && height && (width != session->oldwidth || height != session->oldheight))
	{
	      struct winsize win;
	      ioctl(session->ptm, TIOCGWINSZ, &win);
	      win.ws_row   = height;
	      win.ws_col   = width;
	      ioctl(session->ptm, TIOCSWINSZ, &win);
	      session->oldwidth = width;
	      session->oldheight = height;
	      //printf("size updated\n");
	}

//	printf("POST %s\n",ri->post_data);
  if (keys_received) {
//	  printf("%s\n",keys.c_str());
    lock_wakelock();
    char *keyCodes;
    keyCodes = new char[keys.length()/2];
    int len               = 0;
    for (const unsigned char *ptr = (const unsigned char *)keys.c_str(); ;) {
      unsigned c0         = *ptr++;
      if (c0 < '0' || (c0 > '9' && c0 < 'A') ||
          (c0 > 'F' && c0 < 'a') || c0 > 'f') {
        break;
      }
      unsigned c1         = *ptr++;
      if (c1 < '0' || (c1 > '9' && c1 < 'A') ||
          (c1 > 'F' && c1 < 'a') || c1 > 'f') {
        break;
      }
      keyCodes[len++]     = 16*((c0 & 0xF) + 9*(c0 > '9')) +
                                (c1 & 0xF) + 9*(c1 > '9');
    }
    write(session->ptm, keyCodes, len);
    delete[] keyCodes;
    if (post_data)
	delete[] post_data;
    return;
  }
  if (post_data)
	  delete[] post_data;
  pthread_mutex_lock(&(session->mutex));
  char buf[2048];
  fd_set set;
  struct timeval timeout;
  FD_ZERO(&set);
  FD_SET(session->ptm, &set);
  timeout.tv_sec = 30;
  timeout.tv_usec = 0;
  int s = select(FD_SETSIZE, &set, NULL, NULL, &timeout);
  int n = 1;
  if (s == 1)
	  n = read(session->ptm,buf,2047);
  else if (s == 0)
  {
	  mg_printf(conn,"{\"session\":\"%d\",\"data\":\"\"}\n",sessionid);
	  pthread_mutex_unlock(&(session->mutex));
	  return;
  }
//  if (n<=0 || (n==4 && buf[0] == '\x1B' && buf[1] == '[' && buf[2] == '3' && buf[3] == 'n'))
  if (s<0 || n <= 0)
  {
	  session->alive = false;
	  close(session->ptm);
	  printf("CLOSED TERMINAL\n");
	  pthread_mutex_unlock(&(session->mutex));
	  return;
  }
  char* t = jsonEscape(buf,n);
//  printf("%d: ",n);
//  for (i=0; i < n; i++)
//	  printf("%d, ",buf[i]);
//  printf("\n");
//  if (n==4 && buf[0] == '\x1B' && buf[1] == '[' && buf[2] == '0' && buf[3] == 'n')
//	  mg_printf(conn,"{\"session\":\"%d\",\"data\":\"\"}\n",sessionid);
//  else
	mg_printf(conn,"{\"session\":\"%d\",\"data\":\"%s\"}\n",sessionid,t);
//  printf("%s\n",t);
  delete[] t;
  pthread_mutex_unlock(&(session->mutex));
}
static void
startrecord(struct mg_connection *conn,
                const struct mg_request_info *ri, void *data)
{
	if (ri->permissions != PERM_ROOT && (ri->permissions&PERM_SCREENSHOT)==0)
		return;
	lock_wakelock();
	access_log(ri,"record screenshots");
	int st = mkdir("/sdcard/webkey_TEMP", 0777);
	int orient = 0;
	bool png = false;
	if (ri->uri[12] == 'p')
		png = true;
	if (ri->uri[16] == 'h') // horizontal
		orient = 1;
	int lowres = 0;
	if (ri->uri[17] == 'l') // low res
		lowres = 1;
	firstfb = false;
	if (ri->uri[18] == 'f') // first fb
		firstfb = true;
	bool flip = false;
	if (ri->uri[19] == 'f') // flip
		flip = true;
	recordingnum = 0;
	recordingnumfinished = -1;
	recording = true;
	while (recording && recordingnum < 1000)
	{
		if (!pict)
			init_fb();
		if (!pthread_mutex_trylock(&pngmutex))
		{
//			printf("A\n");
			update_image(orient,lowres,png,flip);
			pthread_mutex_unlock(&pngmutex);
		}
		else
		{
//			printf("B\n");
			pthread_mutex_lock(&pngmutex);
			pthread_mutex_unlock(&pngmutex);
		}
		lastorient = orient;
		lastflip = flip;
		if (!picchanged)
		{
//			printf("C\n");
//			printf("picchanged = %d\n",picchanged);
			pthread_mutex_lock(&diffmutex);
			pthread_cond_broadcast(&diffstartcond);
			if (!exit_flag)
				pthread_cond_wait(&diffcond,&diffmutex);
			pthread_mutex_unlock(&diffmutex);
//			printf("picchanged = %d\n",picchanged);
		}
	}
	send_ok(conn);
}
static void
finishrecord(struct mg_connection *conn,
                const struct mg_request_info *ri, void *data)
{
	if (ri->permissions != PERM_ROOT && (ri->permissions&PERM_SCREENSHOT)==0)
		return;
	lock_wakelock();
	recording = false;
	pthread_mutex_lock(&pngmutex);
	pthread_mutex_unlock(&pngmutex);
	zipFile zf = zipOpen("/sdcard/webkey_TEMP/screenshots.zip",0);
	   DIR *d = opendir("/sdcard/webkey_TEMP/");
	   size_t path_len = strlen("/sdcard/webkey_TEMP/");
	   int r = -1;
	   if (d)
	   {
	      struct dirent *p;
	      r = 0;
	      while (!r && (p=readdir(d)))
	      {
		  int r2 = -1;
		  char *buf;
		  size_t len;
		  int l = strlen(p->d_name);
		  if (l > 3 && (strcmp(p->d_name+l-3,"jpg") == 0 || !strcmp(p->d_name+l-3, "png")))
		  {
			  zip_fileinfo fi;
			  fi.tmz_date.tm_year = 2010;
			  fi.dosDate = 0;
			  fi.internal_fa = 0;
			  fi.external_fa = 0;
			  if (strcmp(p->d_name+l-3,"jpg") == 0)
				  zipOpenNewFileInZip(zf,p->d_name,&fi,NULL,0,NULL,0,"",Z_DEFLATED,Z_DEFAULT_COMPRESSION);
			  else
				  zipOpenNewFileInZip(zf,p->d_name,&fi,NULL,0,NULL,0,"",0,0);
			  FILE* f;
			  std::string path = std::string("/sdcard/webkey_TEMP/") + p->d_name;
			  f = fopen(path.c_str(),"rb");
			  if (!f)
				  return;
			  fseek (f , 0 , SEEK_END);
			  int lSize = ftell (f);
			  rewind (f);
			  char* filebuffer = new char[lSize+1];
			  if(!filebuffer)
			  {
				  error("not enough memory for loading tmp.png\n");
			  }
			  fread(filebuffer,1,lSize,f);
			  filebuffer[lSize] = 0;
			  zipWriteInFileInZip(zf,filebuffer,lSize);
			  fclose(f);
			  delete[] filebuffer;
			  zipCloseFileInZip(zf);
		  }
	      }
	      zipClose(zf,"");
	      FILE* f;
	      f = fopen("/sdcard/webkey_TEMP/screenshots.zip","rb");
	      fseek (f , 0 , SEEK_END);
	      int lSize = ftell (f);
	      rewind (f);
	      char* buf[65536];
	      if (!f)
		      return;
	      send_ok(conn,"Content-Type: application/zip; charset=UTF-8\r\nContent-Disposition: attachment;filename=screenshots.zip",lSize);
	      while(1)
	      {
		      int num_read = 0;
		      if ((num_read = fread(buf, 1, 65536, f)) == 0)
			      break;
		      if (mg_write(conn, buf, num_read) != num_read)
			      break;
	      }
	      fclose(f);
	      remove_directory("/sdcard/webkey_TEMP");
	   }
}
static void *event_handler(enum mg_event event,
                           struct mg_connection *conn,
                           const struct mg_request_info *request_info) {
  if (event != MG_NEW_REQUEST)
	  return 0;
  //access_log(request_info,"log in");
  void *processed = (void*)1;
  if (urlcompare(request_info->uri, "/") || urlcompare(request_info->uri,"/index.html"))
	getfile(conn, request_info, NULL);
  else if (urlcompare(request_info->uri, "/phone.html"))
  {
      index(conn, request_info, NULL);
  }
  else if (urlcompare(request_info->uri, "/calls.html") ||
		  urlcompare(request_info->uri, "/gps.html")||
		  urlcompare(request_info->uri, "/help.html")||
		  urlcompare(request_info->uri, "/pure_menu.html")||
		  urlcompare(request_info->uri, "/pure_menu_nochat.html")||
		  urlcompare(request_info->uri, "/export.html")||
		  urlcompare(request_info->uri, "/chat.html")||
		  urlcompare(request_info->uri, "/sms.html")
		  )
	getfile(conn, request_info, NULL);
  else if (urlcompare(request_info->uri, "/setpassword"))
	setpassword(conn, request_info, NULL);
  else if (urlcompare(request_info->uri, "/setpermission"))
	setpermission(conn, request_info, NULL);
  else if (urlcompare(request_info->uri, "/register"))
	reg(conn, request_info, NULL);
  else if (urlcompare(request_info->uri, "/getreg"))
	getreg(conn, request_info, NULL);
  else if (urlcompare(request_info->uri, "/screenshot.*"))
	screenshot(conn, request_info, NULL);
  else if (urlcompare(request_info->uri, "/injkey*"))
	key(conn, request_info, NULL);
  else if (urlcompare(request_info->uri, "/oldkey*"))
	key(conn, request_info, NULL);
  else if (urlcompare(request_info->uri, "/touch_*"))
	touch(conn, request_info, NULL);
  else if (urlcompare(request_info->uri, "/savebuttons"))
	savebuttons(conn, request_info, NULL);
  else if (urlcompare(request_info->uri, "/savekeys"))
	savekeys(conn, request_info, NULL);
  else if (urlcompare(request_info->uri, "/button*"))
	button(conn, request_info, NULL);
  else if (urlcompare(request_info->uri, "/gpsget"))
	gpsget(conn, request_info, NULL);
  else if (urlcompare(request_info->uri, "/gpsset*"))
	gpsset(conn, request_info, NULL);
  else if (urlcompare(request_info->uri, "/config_buttons.html"))
	config_buttons(conn, request_info, NULL);
  else if (urlcompare(request_info->uri, "/config_keys.html"))
	config_keys(conn, request_info, NULL);
  else if (urlcompare(request_info->uri, "/notify.html"))
	notify_html(conn, request_info, NULL);
  else if (urlcompare(request_info->uri, "/setnotify*"))
	setnotify(conn, request_info, NULL);
  else if (urlcompare(request_info->uri, "/intent_*"))
	intent(conn, request_info, NULL);
  else if (urlcompare(request_info->uri, "/run*"))
	run(conn, request_info, NULL);
  else if (urlcompare(request_info->uri, "/stop"))
	stop(conn, request_info, NULL);
//  else if (urlcompare(request_info->uri, "/password"))
//	password(conn, request_info, NULL);
  else if (urlcompare(request_info->uri, "/javatest"))
	javatest(conn, request_info, NULL);
  else if (urlcompare(request_info->uri, "/config"))
	config(conn, request_info, NULL);
  else if (urlcompare(request_info->uri, "/usersconfig"))
	config(conn, request_info, (void*)"nomenu");
  else if (urlcompare(request_info->uri, "/status"))
	status(conn, request_info, NULL);
  else if (urlcompare(request_info->uri, "/dyndns*"))
	dyndnsset(conn, request_info, NULL);
  else if (urlcompare(request_info->uri, "/new*"))
	emptyresponse(conn, request_info, NULL);
  else if (urlcompare(request_info->uri, "/password.txt*"))
	emptyresponse(conn, request_info, NULL);
  else if (urlcompare(request_info->uri, "/sdcard*"))
	sdcard(conn, request_info, NULL);
  else if (urlcompare(request_info->uri, "/content.php*"))
	content(conn, request_info, NULL);
  else if (urlcompare(request_info->uri, "/export_*"))
	exports(conn, request_info, NULL);
  else if (urlcompare(request_info->uri, "/client/flash/content.php*"))
	content(conn, request_info, NULL);
  else if (urlcompare(request_info->uri, "/touchtest"))
	testtouch(conn, request_info, NULL);
  else if (urlcompare(request_info->uri, "/fbtest"))
	testfb(conn, request_info, NULL);
  else if (urlcompare(request_info->uri, "/reread"))
	reread(conn, request_info, NULL);
  else if (urlcompare(request_info->uri, "/sms.xml"))
	smsxml(conn, request_info, NULL);
  else if (urlcompare(request_info->uri, "/contacts.xml"))
	contactsxml(conn, request_info, NULL);
  else if (urlcompare(request_info->uri, "/calls.xml"))
	callsxml(conn, request_info, NULL);
  else if (urlcompare(request_info->uri, "/waitdiff"))
	waitdiff(conn, request_info, NULL);
  else if (urlcompare(request_info->uri, "/sendsms"))
	sendsms(conn, request_info, NULL);
  else if (urlcompare(request_info->uri, "/sendbroadcast"))
	sendbroadcast(conn, request_info, NULL);
  else if (urlcompare(request_info->uri, "/shellinabox*"))
	shellinabox(conn, request_info, NULL);
  else if (urlcompare(request_info->uri, "/getchatmessage_*"))
	getchatmessage(conn, request_info, NULL);
  else if (urlcompare(request_info->uri, "/writechatmessage"))
	writechatmessage(conn, request_info, NULL);
  else if (urlcompare(request_info->uri, "/clearchatmessage"))
	clearchatmessage(conn, request_info, NULL);
  else if (urlcompare(request_info->uri, "/phonegetchatmessage_*"))
	phonegetchatmessage(conn, request_info, NULL);
  else if (urlcompare(request_info->uri, "/phonewritechatmessage"))
	phonewritechatmessage(conn, request_info, NULL);
  else if (urlcompare(request_info->uri, "/phoneclearchatmessage"))
	phoneclearchatmessage(conn, request_info, NULL);
  else if (urlcompare(request_info->uri, "/adjust_light_*"))
	adjust_light(conn, request_info, NULL);
  else if (urlcompare(request_info->uri, "/startrecord*"))
	startrecord(conn, request_info, NULL);
  else if (urlcompare(request_info->uri, "/finishrecord"))
	finishrecord(conn, request_info, NULL);
  else if (urlcompare(request_info->uri, "/login"))
  {
	mg_printf(conn,"HTTP/1.1 200 OK\r\nCache-Control: no-store, no-cache, must-revalidate\r\nCache-Control: post-check=0, pre-check=0\r\nPragma: no-cache\r\nContent-Type: text/html; charset=utf-8\r\nConnection: close\r\n\r\n<html><head><meta http-equiv=\"refresh\" content=\"0;url=phone.html\"></head><body>Redirecting...</body></html>");
  }
  else if (urlcompare(request_info->uri, "/test"))
	test(conn, request_info, NULL);
  else
	processed = NULL;

/*  if (event == MG_NEW_REQUEST) {
    if (!request_info->is_ssl) {
      redirect_to_ssl(conn, request_info);
    } else if (!is_authorized(conn, request_info)) {
      redirect_to_login(conn, request_info);
    } else if (strcmp(request_info->uri, authorize_url) == 0) {
      authorize(conn, request_info);
    } else if (strcmp(request_info->uri, "/ajax/get_messages") == 0) {
      ajax_get_messages(conn, request_info);
    } else if (strcmp(request_info->uri, "/ajax/send_message") == 0) {
      ajax_send_message(conn, request_info);
    } else {
      // No suitable handler found, mark as not processed. Mongoose will
      // try to serve the request.
      processed = NULL;
    }
  } else {
    processed = NULL;
  }
*/
  return processed;
}


int main(int argc, char **argv)
{

	__android_log_print(ANDROID_LOG_INFO,"Webkey C++","service's started");
//#ifdef VFP
//	printf("Compiled with VFP floating point (for example Xperia X10 needs that).\n");
//#else
//	printf("Compiled without VFP floating point.\n");
//#endif
	modelname[0] = 0;
	deflanguage[0] = 0;
	__system_property_get("ro.product.model",modelname);
	__system_property_get("persist.sys.language",deflanguage);
	char manu[PROP_VALUE_MAX];
	manu[0] = 0;
	__system_property_get("ro.product.manufacturer",manu);
	if (strcmp(manu,"samsung")==0)
		samsung = true;
	int i; 
	for (i=0;deflanguage[i] && i < PROP_VALUE_MAX;i++)
		if (deflanguage[i] >= 'A' && deflanguage[i] <= 'Z')
			deflanguage[i] += 'a'-'A';
	printf("deflanguage = %s\n",deflanguage);
	printf("phone model: %s\n",modelname);
	if (strcmp(modelname,"GT-I5800")==0)
	{
		force_240 = true;
	}
	if (strcmp(modelname,"Geeksphone ONE")==0)
		force_240 = true;
	if (strcmp(modelname,"ZTE-BLADE")==0)
		is_zte_blade = true;

	pid_t pid;
	if (pipe(pipeforward))
		error("Unable to set up pipes.");
	if (pipe(pipeback))
		error("Unable to set up pipes.");

	pid = fork();
	if (pid < (pid_t)0)
	{
		error("Unable to fork().");
	}
	if (pid == 0)
	{
		close(pipeback[0]);
		close(pipeforward[1]);
		char line[LINESIZE];
		char buff[LINESIZE];
		FILE* in = fdopen(pipeforward[0],"r");
		if (!in)
			error("Unable to open pipeforward.");
		FILE* out = fdopen(pipeback[1],"w");
		if (!out)
			error("Unable to open pipeback.");
		while (fgets(line, sizeof(line)-1, in) != NULL) 
		{
			if (strlen(line))
				line[strlen(line)-1] = 0;
			if (line[0] == 'S')
			{
				if (fork()==0)
				{
					system(line+1);
					return EXIT_FAILURE;
				}
				fflush(out);
				continue;
			}
			fflush(NULL);
			FILE* p = popen(line,"r");
			if (p)
			{
				while (fgets(buff, sizeof(buff)-1, p) != NULL)
				{
					fprintf(out,"%s",buff);
				}
				pclose(p);
			}
			else
				printf("error calling popen\n");
			fprintf(out,"!!!END_OF_POPEN!!!\n");
			fflush(out);
		}
		fclose(in);
		fclose(out);
		close(pipeback[1]);
		close(pipeforward[0]);
		printf("popen fork closed\n");
		return EXIT_FAILURE;
	}
	close(pipeback[1]);
	close(pipeforward[0]);
	pipeout = fdopen(pipeforward[1],"w");
	if (!pipeout)
		error("Unable to open pipeout.");
	pipein = fdopen(pipeback[0],"r");
	if (!pipein)
		error("Unable to open pipein.");


//	int terminal_pid;
//	create_subprocess(&terminal_pid);

	for (i = strlen(argv[0])-1; i>=0; i--)
		if (argv[0][i] == '/')
		{
			argv[0][i+1]=0;
			break;
		}
	dir = argv[0];
	logfile = dir+"log.txt";
	pthread_mutex_init(&logmutex, NULL);
	pthread_mutex_init(&chatmutex, NULL);
	pthread_cond_init(&chatcond,0);
	access_log(NULL,"service's started");
	dirdepth = -1;
	for (i = 0; i < strlen(argv[0]); i++)
		if (argv[0][i] == '/')
			dirdepth++;
	//printf("%d\n",dirdepth);
	port = 80;
	read_prefs();
	if (argc == 2)
		port = strtol (argv[1], 0, 10);
	if (port <= 0)
		error("Invalid port\n");
        if ((fbfd = open(FB_DEVICE, O_RDONLY)) == -1)
        {
                error("open framebuffer\n");
        }

        if (ioctl(fbfd, FBIOGET_VSCREENINFO, &scrinfo) != 0)
        {
                error("error reading screeninfo\n");
        }
	if (scrinfo.xres_virtual == 240) //for x10 mini pro
	       scrinfo.xres_virtual = 256;
	if (scrinfo.xres == 256)
	{
		int nall = 0;
		int n;
		int fd = open(FB_DEVICE, O_RDONLY);
		char tmp[1024];
		if (fd>=0)
		{
			while (n = read(fd, tmp, 1024))
			{
				nall += n;
			}
			printf("%d\n",nall);
			close(fd);
			bytespp = scrinfo.bits_per_pixel / 8;
			if (bytespp == 3)
				bytespp = 4;
			if (scrinfo.xres*scrinfo.yres*bytespp*2 < nall)
				force_240 = true;
		}
	}
	if (force_240)
		scrinfo.xres_virtual = scrinfo.xres = 240;

	max_brightness = 255;
	int fd = open("/sys/class/leds/lcd-backlight/max_brightness", O_RDONLY);
	char value[20];
	int n;
	if (fd >= 0)
	{
		n = read(fd, value, 10);
		if (n)
			max_brightness = getnum(value);
		close(fd);
	}
	char buffer[8192];
        (void) signal(SIGCHLD, signal_handler);
        (void) signal(SIGTERM, signal_handler);
        (void) signal(SIGINT, signal_handler);
	itoa(buffer,port);
//        if (mg_set_option(ctx, "ports", buffer) != 1)
//                error("Error in configurate port.\n");
	passfile = "passwords.txt";
	FILE* pf = fopen((dir+passfile).c_str(),"r");
	if (pf)
		fclose(pf);
	else
	{
		pf = fopen((dir+passfile).c_str(),"w");
		if (pf)
			fclose(pf);
	}
	char prot[512];
	sprintf(prot,(std::string("/favicon.ico=,/flags_=,/javatest=,/gpsset=,/stop=,/dyndns=,/reread=,/test=,/sendbroadcast=,/phonegetchatmessage=,/phonewritechatmessage=,/phoneclearchatmessage=,/index.html=,/register=,/reganim.gif=,/=")+dir+passfile).c_str());
//        mg_set_option(ctx, "auth_realm", "Webkey");
#ifdef __linux__
//        mg_set_option(ctx, "error_log", "log.txt");
#endif
#ifndef __linux__
//	mg_set_option(ctx, "protect", (std::string("/password=,/gpsset=,/stop=,/dyndns=,/reread=,/test=,/sendbroadcast=,/=")+dir+passfile).c_str());
//	mg_set_option(ctx, "root",dir.c_str());
#else
//	mg_set_option(ctx, "root",dir.c_str());
#endif
	struct timeval tv;
	gettimeofday(&tv,0);
	srand ( time(NULL)+tv.tv_usec );
	for (i = 0; i < 10; i++)
	{
		upload_random[i] = (char)(rand()%26+97);
	}
	FILE* auth;
	if ((auth = fopen((dir+"authkey.txt").c_str(),"w")))
	{
		fprintf(auth,"%s",upload_random);
		fclose(auth);
	}
	char strport[16];
	char strport_ssl[16];
	char docroot[256];
	strcpy(docroot,dir.c_str());
	itoa(strport,port);
	FILE* sf = fopen((dir+"ssl_cert.pem").c_str(),"r");
	if (sf)
	{
		fclose(sf);
		has_ssl = true;
	}
	else
	{
		//temporary
		//syst("chmod \"a+x\" /data/data/com.webkey/files/openssl");
		syst((dir+"openssl req -x509 -nodes -days 5650 -newkey rsa:1024 -keyout "+dir+"ssl_cert.pem -out "+dir+"ssl_cert.pem -config "+dir+"ssleay.cnf").c_str());
		sf = fopen((dir+"ssl_cert.pem").c_str(),"r");
		if (sf)
		{
			fclose(sf);
			has_ssl = true;
		}
	}
	if (has_ssl)
		unlink((dir+"openssl").c_str());
	static const char *options[] = {
		"document_root", docroot,
		"listening_ports", strport,
		"num_threads", "10",
		"protect_uri", prot,
		"authentication_domain", "Webkey",
		NULL
	};
	strcpy(strport_ssl,strport);
	char t[10];
	itoa(t,sslport);
	strcat(strport_ssl,",");
	strcat(strport_ssl,t);
	strcat(strport_ssl,"s");
	char sslc[100];
	strcpy(sslc,(dir+"ssl_cert.pem").c_str());
	static const char *options_ssl[] = {
		"document_root", docroot,
		"listening_ports", strport_ssl,
		"ssl_certificate", sslc,
		"num_threads", "10",
		"protect_uri", prot,
		"authentication_domain", "Webkey",
		NULL
	};
	
	if (has_ssl)
	{
		printf("SSL is ON\n");
		if ((ctx = mg_start(upload_random,&event_handler,options_ssl)) == NULL) {
			error("Cannot initialize Mongoose context");
		}
	}
	else
	{
		printf("SSL is OFF, error generating the key.\n");
		if ((ctx = mg_start(upload_random,&event_handler,options)) == NULL) {
			error("Cannot initialize Mongoose context");
		}
	}
	
	pthread_mutex_init(&pngmutex, NULL);
	pthread_mutex_init(&diffmutex, NULL);
	pthread_mutex_init(&popenmutex, NULL);
	pthread_mutex_init(&uinputmutex, NULL);
	pthread_mutex_init(&wakelockmutex, NULL);
	pthread_cond_init(&diffcond,0);
	pthread_cond_init(&diffstartcond,0);
//	pthread_mutex_init(&smsmutex,0);
//	pthread_mutex_init(&contactsmutex,0);
//	pthread_cond_init(&smscond,0);
//	pthread_cond_init(&contactscond,0);
//	admin_password = "";
	chat_random = time(NULL) + tv.tv_usec;
	chat_count = 1;
	loadchat();
	wakelock = true;	//just to be sure
	unlock_wakelock(true);
//	for (i = 0; i < 8; i++)
//	{
//		char c[2]; c[1] = 0; c[0] = (char)(rand()%26+97);
//		admin_password += c;
//	}
//	mg_modify_passwords_file(ctx, (dir+passfile).c_str(), "admin", admin_password.c_str(),-1);
//	mg_modify_passwords_file(ctx, (dir+passfile).c_str(), "admin", admin_password.c_str(),-2);
	//load_mimetypes();
	notify.smsalarmed = false;
	notify.callalarmed = false;
	notify.lastsmstime = 0;
	notify.lastcalltime = 0;
	notify.lastalarm = -1000000;
	FILE* in = fopen((dir + "notify.txt").c_str(),"r");
	if (in)
	{
		char buff[LINESIZE];
		if (fgets(buff, LINESIZE-1, in) != NULL)
			setupnotify(buff);
		fclose(in);
	}



//        printf("Webkey %s started on port(s) [%s], serving directory [%s]\n",
//            mg_version(),
//            mg_get_option(ctx, "ports"),
//            mg_get_option(ctx, "root"));


	printf("starting touch...\n");
	init_touch();
	printf("starting uinput...\n");
	init_uinput();
        fflush(stdout);

	pthread_t backthread;
	backserver_parameter par;
	par.server_username = &server_username;
	par.server_random = &server_random;
	par.server = &server;
	par.server_changes = &server_changes;
	par.server_port = &(strport[0]);
	par.ctx = ctx;
	pthread_create(&backthread,NULL,backserver,(void*)&par);
	pthread_t watchthread;
	pthread_create(&watchthread,NULL,watchscreen,(void*)NULL);

	i = 0;
	int d = 0;
	int u = 0;
	__u32 tried = 0;
	__u32 lastip = 0;
	int up = 0;
	while (exit_flag == 0)
	{
		i++;
		d++;
		up++;
                sleep(1);
	//usleep(100000);
	//
	//
//		struct timeval tv;
//		tv.tv_sec = 0;
//		tv.tv_usec = 100000;
//		n = select(NULL,NULL, NULL, NULL, &tv);
		if (up%20==0)
			check_notify();
		if (i==10)
		{
			unlock_wakelock(false);
//			set_blink(1,500,200);
//			vibrator_on(1500);

			i = 0;
			if (gps_active)
			{
				gettimeofday(&tv,0);
				if (tv.tv_sec > last_gps_time + 30)
				{
					syst("/system/bin/am broadcast -a \"webkey.intent.action.GPS.STOP\" -n \"com.webkey/.GPS\"");
					gps_active = false;
				}
			}
		}
		//__android_log_print(ANDROID_LOG_INFO,"Webkey C++","debug: d = %d, dyndns = %d, host = %s, dyndns_base64 = %s",d,dyndns,dyndns_host.c_str(),dyndns_base64.c_str());
		if (d==30 || d == 60)
		{
			pthread_mutex_lock(&chatmutex);
			pthread_cond_broadcast(&chatcond);
			pthread_mutex_unlock(&chatmutex);
		}
		if (d==60)
		{
			gettimeofday(&tv,0);
			int q;
			for (q = 0; q < sessions.size(); q++)
			{
				if (sessions[q]->alive)
				{
					if (tv.tv_sec > sessions[q]->lastused + 3600)
					{
						sessions[q]->alive = false;
						kill(sessions[q]->pid,SIGKILL);
						close(sessions[q]->ptm);
						printf("CLOSED\n");
					}
				}
			}
			backdecrease(); //decrease the number of connection to the server
			d=0;
			__u32 ip = ipaddress();
//			printf("IP ADDRESS: %d\n",ip);
//			printf("LAST IP ADDRESS: %d\n",lastip);
//			printf("SERVER_CHANGES before check: %d\n",server_changes);
			if (ip!=lastip)
				server_changes++;
//			printf("SERVER_CHANGES before after: %d\n",server_changes);
			lastip = ip;
			if(dyndns)
			{
				//__android_log_print(ANDROID_LOG_INFO,"Webkey C++","debug: dyndns is true");
				//in_addr r;
				//r.s_addr = ip;
				//__android_log_print(ANDROID_LOG_INFO,"Webkey C++","debug: ip address = %s",inet_ntoa(r));
				//r.s_addr = dyndns_last_updated_ip;
				//__android_log_print(ANDROID_LOG_INFO,"Webkey C++","debug: dyndns_last_updated_ip = %s",inet_ntoa(r));
				if (ip && ip != dyndns_last_updated_ip)
				{
					//__android_log_print(ANDROID_LOG_INFO,"Webkey C++","debug: updateing, u = %d",u);
					//r.s_addr = tried;
					//__android_log_print(ANDROID_LOG_INFO,"Webkey C++","debug: tried = %s",inet_ntoa(r));
					u++;
					if (tried != ip || u==10) //tries to update every 10 mins
					{
						tried = ip;
						u = 0;
						std::string ret = update_dyndns(ip);
//						printf("%s\n",ret.c_str());
						__android_log_print(ANDROID_LOG_INFO,"Webkey C++","debug: ret = %s",ret.c_str());
					}
				}
			}
		}
	}
	pthread_mutex_lock(&diffmutex);
	pthread_cond_broadcast(&diffstartcond);
	pthread_mutex_unlock(&diffmutex);
	pthread_mutex_lock(&chatmutex);
	pthread_cond_broadcast(&chatcond);
	pthread_mutex_unlock(&chatmutex);
	if (gps_active)
		syst("/system/bin/am broadcast -a \"webkey.intent.action.GPS.STOP\" -n \"com.webkey/.GPS\"");
        (void) printf("Exiting on signal %d, "
            "waiting for all threads to finish...", exit_flag);
        fflush(stdout);
        mg_stop(ctx);
        (void) printf("%s", " done.\n");
	unlock_wakelock(true);

	clear();
	pthread_mutex_destroy(&pngmutex);
	fclose(pipeout);
	fclose(pipein);
	close(pipeback[0]);
	close(pipeforward[1]);
        return (EXIT_SUCCESS);
  return 0;
}
