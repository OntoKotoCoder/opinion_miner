#include "opinion_minerd.h"

using namespace std;     

const char* config_path = "/opt/opinion_miner/general.cfg";
get_parameters* config = new get_parameters(config_path);

int main(int argc, char** argv)
{
    int status;
    int pid;
    
    // если параметров командной строки меньше двух, то покажем как использовать демона
    /*if (argc != 2)
    {
        printf("Usage: ./my_daemon filename.cfg\n");
        return -1;
    }*/

    // загружаем файл конфигурации
    status = LoadConfig();
    
    if (!status) // если произошла ошибка загрузки конфига
    {
        printf("Error: Load config failed\n");
        return -1;
    }
    
    // создаем потомка
    pid = fork();

    if (pid == -1) // если не удалось запустить потомка
    {
        // выведем на экран ошибку и её описание
        printf("Error: Start Daemon failed (%s)\n", strerror(errno));
        
        return -1;
    }
    else if (!pid) // если это потомок
    {
        // данный код уже выполняется в процессе потомка
        // разрешаем выставлять все биты прав на создаваемые файлы, 
        // иначе у нас могут быть проблемы с правами доступа
        umask(0);
        
        // создаём новый сеанс, чтобы не зависеть от родителя
        setsid();
        
        // переходим в корень диска, если мы этого не сделаем, то могут быть проблемы.
        // к примеру с размантированием дисков
        chdir("/");
        
        // закрываем дискрипторы ввода/вывода/ошибок, так как нам они больше не понадобятся
        close(STDIN_FILENO);
        close(STDOUT_FILENO);
        close(STDERR_FILENO);
        
        // Данная функция будет осуществлять слежение за процессом
        status = MonitorProc();
        
        return status;
    }
    else // если это родитель
    {
        // завершим процес, т.к. основную свою задачу (запуск демона) мы выполнили
        return 0;
    }
}

int MonitorProc()
{
    int      pid;
    int      status;
    int      need_start = 1;
    sigset_t sigset;
    siginfo_t siginfo;

    ofstream workr_log;

    // настраиваем сигналы которые будем обрабатывать
    sigemptyset(&sigset);
    
    // сигнал остановки процесса пользователем
    sigaddset(&sigset, SIGQUIT);
    
    // сигнал для остановки процесса пользователем с терминала
    sigaddset(&sigset, SIGINT);
    
    // сигнал запроса завершения процесса
    sigaddset(&sigset, SIGTERM);
    
    // сигнал посылаемый при изменении статуса дочернего процесса
    sigaddset(&sigset, SIGCHLD); 
    
    // пользовательский сигнал который мы будем использовать для обновления конфига
    sigaddset(&sigset, SIGHUP); 
    sigprocmask(SIG_BLOCK, &sigset, NULL);

    // данная функция создаст файл с нашим PID'ом
    SetPidFile(PID_FILE);

    workr_log.open(WORKR_LOG_FILE, fstream::app);

    workr_log << get_time() << " [ DAEMON] # PARENT Started" << endl;
    workr_log << get_time() << " [ DAEMON] # PARENT Started a new calculation of the emotional tone" << endl;

    // бесконечный цикл работы
    for (;;)
    {
        // если необходимо создать потомка
        if (need_start)
        {
            // создаём потомка
            pid = fork();
        }
        
        need_start = 1;
        
        if (pid == -1) // если произошла ошибка
        {
            // запишем в лог сообщение об этом
            workr_log << get_time() << " [MONITOR] # PARENT Fork failed " << strerror(errno) << endl;
        }
        else if (!pid) // если мы потомок
        {
            // данный код выполняется в потомке
            
            // запустим функцию отвечающую за работу демона
            status = WorkProc();
            
            // завершим процесс
            exit(status);
        }
        else // если мы родитель
        {
            // данный код выполняется в родителе
            
            // ожидаем поступление сигнала
            sigwaitinfo(&sigset, &siginfo);
            
            // если пришел сигнал от потомка
            if (siginfo.si_signo == SIGCHLD)
            {
                // получаем статус завершение
                wait(&status);
                
                // преобразуем статус в нормальный вид
                status = WEXITSTATUS(status);

                 // если потомок завершил работу с кодом говорящим о том, что нет нужды дальше работать
                if (status == CHILD_NEED_TERMINATE)
                {
                    // запишем в лог сообщени об этом        
                    workr_log << get_time() << " [ DAEMON] # PARENT Calculation of the emotional tone stopped" << endl;
                    
                    // прервем цикл
                    break;
                }
                else if (status == CHILD_NEED_WORK) // если требуется перезапустить потомка
                {
                    // запишем в лог данное событие
                    workr_log << get_time() << " [ DAEMON] # PARENT Started a new calculation of the emotional tone" << endl;
                }
            }
            else if (siginfo.si_signo == SIGHUP) // если пришел сигнал что необходимо перезагрузить конфиг
            {
                //kill(pid, SIGHUP); // перешлем его потомку
                need_start = 0; // установим флаг что нам не надо запускать потомка заново
                // обновим конфиг
                status = ReloadConfig();
                if (status == 0)
                { 
                    workr_log << get_time() << " [ DAEMON] # PARENT Reload config failed" << endl;
                }
                else
                {
                    workr_log << get_time() << " [ DAEMON] # PARENT Reload config OK" << endl;
                }
            }
            else if (siginfo.si_signo == SIGTERM)// если пришел какой-либо другой ожидаемый сигнал
            {
                // запишем в лог информацию о пришедшем сигнале
                workr_log << get_time() << " [MONITOR] # PARENT Signal - " << strsignal(siginfo.si_signo) << endl;
                
                // убьем потомка
                kill(pid, SIGTERM);
                status = 0;
                break;
            }
        }
    }

    // запишем в лог, что мы остановились
    workr_log << get_time() << " [MONITOR] # PARENT Stop" << endl;
    
    // удалим файл с PID'ом
    unlink(PID_FILE);
    workr_log.close();
    return status;
}

int WorkProc()
{
    struct sigaction	sigact;
    sigset_t		sigset;
    int			signo;
    int			status;

    ofstream workr_log;

    // сигналы об ошибках в программе будут обрататывать более тщательно
    // указываем что хотим получать расширенную информацию об ошибках
    sigact.sa_flags = SA_SIGINFO;
    // задаем функцию обработчик сигналов
    sigact.sa_sigaction = signal_error;

    sigemptyset(&sigact.sa_mask);

    // установим наш обработчик на сигналы
    
    sigaction(SIGFPE, &sigact, 0); // ошибка FPU
    sigaction(SIGILL, &sigact, 0); // ошибочная инструкция
    sigaction(SIGSEGV, &sigact, 0); // ошибка доступа к памяти
    sigaction(SIGBUS, &sigact, 0); // ошибка шины, при обращении к физической памяти

    sigemptyset(&sigset);
    
    // блокируем сигналы которые будем ожидать
    // сигнал остановки процесса пользователем
    sigaddset(&sigset, SIGQUIT);
    
    // сигнал для остановки процесса пользователем с терминала
    sigaddset(&sigset, SIGINT);
    
    // сигнал запроса завершения процесса
    sigaddset(&sigset, SIGTERM);

    // сигнал запроса обновления конфигурации
    sigaddset(&sigset, SIGHUP);
    
    // пользовательский сигнал который мы будем использовать для обновления конфига
    sigaddset(&sigset, SIGUSR1); 
    sigprocmask(SIG_BLOCK, &sigset, NULL);

    // Установим максимальное кол-во дискрипторов которое можно открыть
    SetFdLimit(FD_LIMIT);

    workr_log.open(WORKR_LOG_FILE, fstream::app);
    
    // запускаем все рабочие потоки
    status = InitWorkThread();
    if (status == 1)
    {
        workr_log << get_time() << " [ DAEMON] # CHILDR Opinion miner is completed successfully with code: " << status << endl;
        // ждем указанных сообщений
        /*sigwait(&sigset, &signo);
        
        // если то сообщение обновления конфига
        if (signo == SIGHUP)
        {
            // обновим конфиг
            status = ReloadConfig();
            if (status == 0)
            {
                workr_log << get_time() << " WORKR # [DAEMON] Reload config failed" << endl;
            }
            else
            {
                workr_log << get_time() << " WORKR # [DAEMON] Reload config OK" << endl;
            }
        }
        else // если какой-либо другой сигнал, то выйдим из цикла
        {
            break;
        }*/
        
        // остановим все рабочеи потоки и корректно закроем всё что надо
        DestroyWorkThread();
    }
    else
    {
        workr_log << get_time() << " [ DAEMON] # CHILDR Create work thread failed" << endl;
    }
    
    workr_log.close();

    // вернем код не требующим перезапуска
    return CHILD_NEED_WORK;
}

// функция загрузки конфига
int LoadConfig()
{
        config->get_general_params();
        config->get_smad_db_params();
        config->get_dict_db_params();
        config->get_svm_params();
        return 1;
}

// функция которая инициализирует рабочие потоки
int InitWorkThread()
{
    process* proc = new process();
    // код функции
    proc->start_calc_emotion();
    delete proc;
    return 1;
}

// функция которая загрузит конфиг заново
// и внесет нужные поправки в работу
int ReloadConfig()
{
        // код функции
        config->get_general_params();
        config->get_smad_db_params();
        config->get_dict_db_params();
        config->get_svm_params();
        return 1;
}

// функция для остановки потоков и освобождения ресурсов
void DestroyWorkThread()
{
        // тут должен быть код который остановит все потоки и
        // корректно освободит ресурсы
}

static void signal_error(int sig, siginfo_t *si, void *ptr)
{
    void*   ErrorAddr;
    void*   Trace[16];
    int     x;
    int     TraceSize;
    char**  Messages;

    ofstream error_log;
    ofstream workr_log;

    error_log.open(ERROR_LOG_FILE, fstream::app);
    workr_log.open(WORKR_LOG_FILE, fstream::app);

    // запишем в лог что за сигнал пришел
    //WriteLog("[DAEMON] Signal: %s, Addr: 0x%0.16X\n", strsignal(sig), si->si_addr);
    error_log << get_time() << " # DAEMN: Signal - " << strsignal(sig) << " | Addr: - " << si->si_addr << endl;
    
    #if __WORDSIZE == 64 // если дело имеем с 64 битной ОС
        // получим адрес инструкции которая вызвала ошибку
        ErrorAddr = (void*)((ucontext_t*)ptr)->uc_mcontext.gregs[REG_RIP];
    #else 
        // получим адрес инструкции которая вызвала ошибку
        ErrorAddr = (void*)((ucontext_t*)ptr)->uc_mcontext.gregs[REG_EIP];
    #endif

    // произведем backtrace чтобы получить весь стек вызовов 
    TraceSize = backtrace(Trace, 16);
    Trace[1] = ErrorAddr;

    // получим расшифровку трасировки
    Messages = backtrace_symbols(Trace, TraceSize);
    if (Messages)
    {
        error_log << "== Backtrace [start at: " << get_time() << "] ==" << endl;
        
        // запишем в лог
        for (x = 1; x < TraceSize; x++)
        {
            error_log << get_time() << " " << Messages[x] << endl;
        }
        
        error_log << "== End Backtrace [finished at: " << get_time() << "] ==" << endl;
        free(Messages);
    }

    workr_log << get_time() << " [ DAEMON] # [DAEMON] Stopped" << endl;
    
    // остановим все рабочие потоки и корректно закроем всё что надо
    DestroyWorkThread();
    
    workr_log.close();
    error_log.close();

    // завершим процесс с кодом требующим перезапуска
    exit(CHILD_NEED_WORK);
}

int SetFdLimit(int MaxFd)
{
    struct rlimit lim;
    int          status;

    // зададим текущий лимит на кол-во открытых дискриптеров
    lim.rlim_cur = MaxFd;
    // зададим максимальный лимит на кол-во открытых дискриптеров
    lim.rlim_max = MaxFd;

    // установим указанное кол-во
    status = setrlimit(RLIMIT_NOFILE, &lim);
    
    return status;
}

void SetPidFile(char* Filename)
{
    ofstream pid_file;

    pid_file.open(PID_FILE);
    if (pid_file.is_open())
    {
        pid_file << getpid();
        pid_file.close();
    }
}

char* get_time ()
{
        time_t rawtime;
        struct tm* timeinfo;
        char* _buffer = new char[17];

        time (&rawtime);
        timeinfo = localtime (&rawtime);

        strftime (_buffer,80,"%d.%m.%y-%H:%M:%S",timeinfo);
        return _buffer;
}
