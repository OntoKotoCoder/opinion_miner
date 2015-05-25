#if !defined(_GNU_SOURCE)
        #define _GNU_SOURCE
#endif

//#include <stdio.h>
#include <iostream>
//#include <stdlib.h>
#include <cstdlib>
//#include <string.h>
#include <string>
#include <cstring>

#include <sys/file.h>
#include <sys/stat.h>
#include <execinfo.h>
#include <unistd.h>
#include <errno.h>
#include <wait.h>


#include "process.h"
 
// лимит для установки максимально кол-во открытых дискрипторов
#define FD_LIMIT				1024*10
 
// константы для кодов завершения процесса
#define CHILD_NEED_WORK			1
#define CHILD_NEED_TERMINATE	2
 
#define CONFIG_FILE				"/opt/opinion_miner/general.cfg"
#define PID_FILE				"/opt/opinion_miner/opinion_minerd.pid"
#define ERROR_LOG_FILE			"/var/log/opinion_miner/error.log"
#define WORKR_LOG_FILE			"/var/log/opinion_miner/worker.log"

int MonitorProc ();
int WorkProc ();

int LoadConfig ();
int InitWorkThread ();
int ReloadConfig ();
void DestroyWorkThread ();

void SetPidFile (char* Filename);
int SetFdLimit (int MaxFd);
static void signal_error (int sig, siginfo_t *si, void *ptr);
char* get_time ();