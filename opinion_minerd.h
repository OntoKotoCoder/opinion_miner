#if !defined(_GNU_SOURCE)
        #define _GNU_SOURCE
#endif

#include <iostream>
#include <cstdlib>
#include <string>
#include <cstring>

#include <wchar.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <execinfo.h>
#include <unistd.h>
#include <errno.h>
#include <wait.h>

#include <boost/regex.hpp>
#include <boost/regex/icu.hpp>
#include <boost/locale/encoding_utf.hpp>

#include <libgearman/gearman.h>

#include "processd.h"
 
// лимит для установки максимально кол-во открытых дискрипторов
#define FD_LIMIT				1024*10
 
// константы для кодов завершения процесса
#define CHILD_NEED_WORK			1
#define CHILD_NEED_TERMINATE	2
 
#define PID_FILE				"/opt/opinion_miner/opinion_minerd.pid"
#define CONFIG_FILE				"/opt/opinion_miner/general.cfg"
//#define ERROR_LOG_FILE			"/var/log/opinion_miner/error.log"
//#define WORKR_LOG_FILE			"/var/log/opinion_miner/worker.log"

typedef unordered_map<int, string> t_texts;

string TEXTS = "";
string* TEXTS_PTR = &TEXTS;
int TEXTS_COUNT = 0;
int* TEXTS_COUNT_PTR = &TEXTS_COUNT;

int MonitorProc ();
int WorkProc (gearman_worker_st* gworker);

int LoadConfig ();
int ReloadConfig ();
int init_work_thread (gearman_worker_st* gworker);
void DestroyWorkThread ();

void* gearman_worker_function (gearman_job_st *job, void *context, size_t *result_size, gearman_return_t *ret_ptr);

void set_pid_file (const char* Filename);
int SetFdLimit (int MaxFd);
static void signal_error (int sig, siginfo_t *si, void *ptr);
char* get_time ();
std::wstring utf8_to_wstring(const std::string& str);
std::string wstring_to_utf8(const std::wstring& w_str);
