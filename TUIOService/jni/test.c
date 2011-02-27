
#include <sys/types.h>
#include <sys/stat.h>
#include <jni.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include "suinput.h"

#define DEBUG 1

#if DEBUG
#include <android/log.h>
#  define  D(x...)  __android_log_print(ANDROID_LOG_INFO,"helloneon",x)
#else
#  define  D(...)  do {} while (0)
#endif


jstring
Java_com_sigmusic_tacchi_tuio_TuioTest_stringFromJNI( JNIEnv* env,
                                                  jobject thiz )
{
    
    __android_log_print(ANDROID_LOG_INFO,"testingsz","HELLO FROM C!!!");
    return (*env)->NewStringUTF(env, "Hello asdfasd from JNI !");
}

jstring
Java_com_sigmusic_tacchi_tuio_TuioTest_testSUinputOpen( JNIEnv* env,
                                                  jobject thiz )
{
    
    __android_log_print(ANDROID_LOG_INFO,"testingsz","Attempting to open uinput!!!");
    
    

    return (*env)->NewStringUTF(env, "Success!");
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
