#include "opinion_minerd.h"

using namespace std;     

string config_path = "/opt/opinion_miner/general.cfg";
get_parameters* config = new get_parameters(&config_path[0]);

int main(int argc, char** argv)
{
	int status;
	int pid;

	// åñëè ïàðàìåòðîâ êîìàíäíîé ñòðîêè ìåíüøå äâóõ, òî ïîêàæåì êàê èñïîëüçîâàòü äåìîíà
	/*if (argc != 2)
	{
		printf("Usage: ./my_daemon filename.cfg\n");
		return -1;
	}*/

	// çàãðóæàåì ôàéë êîíôèãóðàöèè
	status = LoadConfig();
	
	if (!status) // åñëè ïðîèçîøëà îøèáêà çàãðóçêè êîíôèãà
	{
		printf("Error: Load config failed\n");
		return -1;
	}

	// ñîçäàåì ïîòîìêà
	pid = fork();

	if (pid == -1) // åñëè íå óäàëîñü çàïóñòèòü ïîòîìêà
	{
		// âûâåäåì íà ýêðàí îøèáêó è å¸ îïèñàíèå
		printf("Error: Start Daemon failed (%s)\n", strerror(errno));

		return -1;
	}
	else if (!pid) // åñëè ýòî ïîòîìîê
	{
		// äàííûé êîä óæå âûïîëíÿåòñÿ â ïðîöåññå ïîòîìêà
		// ðàçðåøàåì âûñòàâëÿòü âñå áèòû ïðàâ íà ñîçäàâàåìûå ôàéëû, 
		// èíà÷å ó íàñ ìîãóò áûòü ïðîáëåìû ñ ïðàâàìè äîñòóïà
		umask(0);

		// ñîçäà¸ì íîâûé ñåàíñ, ÷òîáû íå çàâèñåòü îò ðîäèòåëÿ
		setsid();
 
		// ïåðåõîäèì â êîðåíü äèñêà, åñëè ìû ýòîãî íå ñäåëàåì, òî ìîãóò áûòü ïðîáëåìû.
		// ê ïðèìåðó ñ ðàçìàíòèðîâàíèåì äèñêîâ
		chdir("/");

		// çàêðûâàåì äèñêðèïòîðû ââîäà/âûâîäà/îøèáîê, òàê êàê íàì îíè áîëüøå íå ïîíàäîáÿòñÿ
		close(STDIN_FILENO);
		close(STDOUT_FILENO);
		close(STDERR_FILENO);

		// Äàííàÿ ôóíêöèÿ áóäåò îñóùåñòâëÿòü ñëåæåíèå çà ïðîöåññîì
		status = MonitorProc();

		return status;
	}
	else // åñëè ýòî ðîäèòåëü
	{
		// çàâåðøèì ïðîöåñ, ò.ê. îñíîâíóþ ñâîþ çàäà÷ó (çàïóñê äåìîíà) ìû âûïîëíèëè
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

	// íàñòðàèâàåì ñèãíàëû êîòîðûå áóäåì îáðàáàòûâàòü
	sigemptyset(&sigset);

	// ñèãíàë îñòàíîâêè ïðîöåññà ïîëüçîâàòåëåì
	sigaddset(&sigset, SIGQUIT);

	// ñèãíàë äëÿ îñòàíîâêè ïðîöåññà ïîëüçîâàòåëåì ñ òåðìèíàëà
	sigaddset(&sigset, SIGINT);

	// ñèãíàë çàïðîñà çàâåðøåíèÿ ïðîöåññà
	sigaddset(&sigset, SIGTERM);
    
	// ñèãíàë ïîñûëàåìûé ïðè èçìåíåíèè ñòàòóñà äî÷åðíåãî ïðîöåññà
	sigaddset(&sigset, SIGCHLD); 
    
	// ñèãíàë îáíîâëåíèÿ êîíôèãóðàöèè
	sigaddset(&sigset, SIGHUP); 
	sigprocmask(SIG_BLOCK, &sigset, NULL);

	// äàííàÿ ôóíêöèÿ ñîçäàñò ôàéë ñ íàøèì PID'îì
	set_pid_file(PID_FILE);

	workr_log.open(config->worker_log, fstream::app);

	workr_log << get_time() << " [ DAEMON] # PARENT Started" << endl;
	workr_log << get_time() << " [ DAEMON] # PARENT Started a new calculation of the emotional tone" << endl;

	// ---> GEARMAN <---
			// Ëÿìáäà ôóíêöèÿ: ñòàòóñ âûïîëíåíèÿ ðàçëè÷íûõ îïåðàöèé ñ gearman
			auto status_print = [&workr_log](gearman_return_t gserver_code, string operation_name){
				workr_log << get_time() << " [GEARMAN] # " << operation_name << " code: " << gserver_code << " --  "; 
				if (gearman_success(gserver_code)) workr_log << "success";
				if (gearman_failed(gserver_code)) workr_log << "failed";
				if (gearman_continue(gserver_code)) workr_log << "continue";
				workr_log << endl;
			};

			gearman_worker_st* gworker = gearman_worker_create(nullptr);

			// Òóò íàäî ðåãèñòðèðîâàòü äåìîíà â gearman
			const char* ghost = "127.0.0.1";
			in_port_t gport = 4730;

			gearman_return_t gs_code = gearman_worker_add_server(gworker, ghost, gport);

			status_print(gs_code, "add server");

			const char* function_name = "emote2";
			unsigned timeout = 0;
			void * job_context = nullptr;

			gs_code = gearman_worker_add_function(gworker, function_name, timeout, gearman_worker_function, job_context);
			gearman_worker_set_timeout(gworker, 1000);
			status_print(gs_code, "add function");

    // áåñêîíå÷íûé öèêë ðàáîòû
	for (;;)
	{
		// åñëè íåîáõîäèìî ñîçäàòü ïîòîìêà
		if (need_start)
		{
			// ñîçäà¸ì ïîòîìêà
			pid = fork();
		}
	
		need_start = 1;

		if (pid == -1) // åñëè ïðîèçîøëà îøèáêà
		{
			// çàïèøåì â ëîã ñîîáùåíèå îá ýòîì
			workr_log << get_time() << " [MONITOR] # PARENT Fork failed " << strerror(errno) << endl;
		}
		else if (!pid) // åñëè ìû ïîòîìîê
		{
			// äàííûé êîä âûïîëíÿåòñÿ â ïîòîìêå

			// çàïóñòèì ôóíêöèþ îòâå÷àþùóþ çà ðàáîòó äåìîíà
			gs_code = gearman_worker_add_function(gworker, function_name, timeout, gearman_worker_function, job_context);
			status = WorkProc(gworker);

			// çàâåðøèì ïðîöåññ
			exit(status);
		}
		else // åñëè ìû ðîäèòåëü
		{
			// äàííûé êîä âûïîëíÿåòñÿ â ðîäèòåëå

			// îæèäàåì ïîñòóïëåíèå ñèãíàëà
			sigwaitinfo(&sigset, &siginfo);

			// åñëè ïðèøåë ñèãíàë îò ïîòîìêà
			if (siginfo.si_signo == SIGCHLD)
			{
				// ïîëó÷àåì ñòàòóñ çàâåðøåíèå
				wait(&status);

				// ïðåîáðàçóåì ñòàòóñ â íîðìàëüíûé âèä
				status = WEXITSTATUS(status);

				// åñëè ïîòîìîê çàâåðøèë ðàáîòó ñ êîäîì ãîâîðÿùèì î òîì, ÷òî íåò íóæäû äàëüøå ðàáîòàòü
				if (status == CHILD_NEED_TERMINATE)
				{
					// çàïèøåì â ëîã ñîîáùåíè îá ýòîì        
					workr_log << get_time() << " [ DAEMON] # PARENT Calculation of the emotional tone stopped" << endl;

					// ïðåðâåì öèêë
					break;
				}
				else if (status == CHILD_NEED_WORK) // åñëè òðåáóåòñÿ ïåðåçàïóñòèòü ïîòîìêà
				{
					// çàïèøåì â ëîã äàííîå ñîáûòèå
					workr_log << get_time() << " [ DAEMON] # PARENT Started a new calculation of the emotional tone" << endl;
				}
			}
			else if (siginfo.si_signo == SIGHUP) // åñëè ïðèøåë ñèãíàë ÷òî íåîáõîäèìî ïåðåçàãðóçèòü êîíôèã
			{
				//kill(pid, SIGHUP); // ïåðåøëåì åãî ïîòîìêó
				need_start = 0; // óñòàíîâèì ôëàã ÷òî íàì íå íàäî çàïóñêàòü ïîòîìêà çàíîâî
				// îáíîâèì êîíôèã
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
			else if (siginfo.si_signo == SIGTERM)// åñëè ïðèøåë êàêîé-ëèáî äðóãîé îæèäàåìûé ñèãíàë
			{
				// çàïèøåì â ëîã èíôîðìàöèþ î ïðèøåäøåì ñèãíàëå
				workr_log << get_time() << " [MONITOR] # PARENT Signal - " << strsignal(siginfo.si_signo) << endl;

				// óáüåì ïîòîìêà
				kill(pid, SIGTERM);
				status = 0;
				break;
			}
		}
	}
	gearman_worker_free(gworker);
			// ---> GEARMAN <---

	// çàïèøåì â ëîã, ÷òî ìû îñòàíîâèëèñü
	workr_log << get_time() << " [MONITOR] # PARENT Stop" << endl;
    
	// óäàëèì ôàéë ñ PID'îì
	unlink(PID_FILE);
	workr_log.close();
	return status;
}

int WorkProc(gearman_worker_st* gworker)
{
    struct sigaction	sigact;
    sigset_t			sigset;
    //int				signo;      //íà òåêóùèé ìîìåíò íå èñïîëüçóåòñÿ
    int					status;

    ofstream workr_log;

	// ñèãíàëû îá îøèáêàõ â ïðîãðàììå áóäóò îáðàòàòûâàòü áîëåå òùàòåëüíî
    // óêàçûâàåì ÷òî õîòèì ïîëó÷àòü ðàñøèðåííóþ èíôîðìàöèþ îá îøèáêàõ
    sigact.sa_flags = SA_SIGINFO;
    // çàäàåì ôóíêöèþ îáðàáîò÷èê ñèãíàëîâ
    sigact.sa_sigaction = signal_error;

    sigemptyset(&sigact.sa_mask);

    // óñòàíîâèì íàø îáðàáîò÷èê íà ñèãíàëû
    
    sigaction(SIGFPE, &sigact, 0); // îøèáêà FPU
    sigaction(SIGILL, &sigact, 0); // îøèáî÷íàÿ èíñòðóêöèÿ
    sigaction(SIGSEGV, &sigact, 0); // îøèáêà äîñòóïà ê ïàìÿòè
    sigaction(SIGBUS, &sigact, 0); // îøèáêà øèíû, ïðè îáðàùåíèè ê ôèçè÷åñêîé ïàìÿòè

    sigemptyset(&sigset);
    
    // áëîêèðóåì ñèãíàëû êîòîðûå áóäåì îæèäàòü
    // ñèãíàë îñòàíîâêè ïðîöåññà ïîëüçîâàòåëåì
    sigaddset(&sigset, SIGQUIT);
    
    // ñèãíàë äëÿ îñòàíîâêè ïðîöåññà ïîëüçîâàòåëåì ñ òåðìèíàëà
    sigaddset(&sigset, SIGINT);
    
    // ñèãíàë çàïðîñà çàâåðøåíèÿ ïðîöåññà
    sigaddset(&sigset, SIGTERM);

    // ñèãíàë çàïðîñà îáíîâëåíèÿ êîíôèãóðàöèè
    sigaddset(&sigset, SIGHUP);
    
    // ïîëüçîâàòåëüñêèé ñèãíàë êîòîðûé ìû áóäåì èñïîëüçîâàòü äëÿ îáíîâëåíèÿ êîíôèãà
    sigaddset(&sigset, SIGUSR1); 
    sigprocmask(SIG_BLOCK, &sigset, NULL);

    // Óñòàíîâèì ìàêñèìàëüíîå êîë-âî äèñêðèïòîðîâ êîòîðîå ìîæíî îòêðûòü
    SetFdLimit(FD_LIMIT);

    workr_log.open(config->worker_log, fstream::app);
    
    // çàïóñêàåì âñå ðàáî÷èå ïîòîêè
    status = init_work_thread(gworker);

    if (status == 1)
    {
        workr_log << get_time() << " [ DAEMON] # CHILDR Opinion miner is completed successfully with code: " << status << endl;        
        // îñòàíîâèì âñå ðàáî÷åè ïîòîêè è êîððåêòíî çàêðîåì âñ¸ ÷òî íàäî
        DestroyWorkThread();
    }
    else
    {
        workr_log << get_time() << " [ DAEMON] # CHILDR Create work thread failed" << endl;
    }
    
    workr_log.close();

    // âåðíåì êîä íå òðåáóþùèì ïåðåçàïóñêà
    return CHILD_NEED_WORK;
}

// ôóíêöèÿ çàãðóçêè êîíôèãà
int LoadConfig()
{
        config->get_general_params();
        config->get_directories_params();
        config->get_servers_params();
        config->get_svm_params();
        return 1;
}

// ôóíêöèÿ êîòîðàÿ èíèöèàëèçèðóåò ðàáî÷èå ïîòîêè
int init_work_thread(gearman_worker_st* gworker)
{
    ofstream workr_log;
    workr_log.open(config->worker_log, fstream::app);

    ifstream texts_file;
    string text, texts = "";
    size_t texts_count = 0;
    texts_file.open("/opt/opinion_miner/data/texts_from_gearman", fstream::app);
    while (getline(texts_file, text)) {
    	if (text != "") {
    		texts += text + "\n";
    		texts_count++;
    	}
    }
    text = "";

    // ---> GEARMAN <---
    auto status_print = [&workr_log](gearman_return_t gserver_code, string operation_name){
        workr_log << get_time() << " [GEARMAN] # " << operation_name << " code: " << gserver_code << " --  "; 
        if (gearman_success(gserver_code)) workr_log << "success";
        if (gearman_failed(gserver_code)) workr_log << "failed";
        if (gearman_continue(gserver_code)) workr_log << "continue";
        workr_log << endl;
	};

	if (texts_count < 100) {
		workr_log << get_time() << "==============================" << endl;
		gearman_return_t gs_code = gearman_worker_work(gworker);
		workr_log << get_time() << "==============================" << endl;
		status_print(gs_code, "work done");
	}
	else {
		processd* proc = new processd(config);
		if (proc->calculate_tonality_from_gearman(texts) == 1) {
			remove("/opt/opinion_miner/data/texts_from_gearman");
		}
		else {
			workr_log << get_time() << " [GEARMAN] # Something went wrong" << endl;
		}
		delete proc;
	}

	texts_file.close();
	workr_log.close();

	return 1;
}

void* gearman_worker_function (gearman_job_st *job, void *context, size_t *result_size, gearman_return_t *ret_ptr)
{
	ofstream workr_log;
	ofstream texts;

	string text, id;
	wstring w_text, w_id;
	wstringstream clear_w_text;
	wstring::const_iterator w_text_start;
	wstring::const_iterator w_text_end;

	u32regex u32rx = make_u32regex("([а-яА-ЯёЁ]+)");
	wsmatch result;

	workr_log.open(config->worker_log, fstream::app);
	texts.open("/opt/opinion_miner/data/texts_from_gearman", fstream::app);
	//workr_log << get_time() << "==============================" << endl;
	auto jobptr = gearman_job_workload(job); //this takes the data from the client
	//workr_log << get_time() << "==============================" << endl;
	
	char *idptr, *textptr;

	idptr = strtok((char*)jobptr, "\t");
	textptr = strtok(nullptr, "");

	text = string(textptr);
	id = string(idptr);
	w_text = utf8_to_wstring(text);
	w_id = utf8_to_wstring(id);
	w_text_start = w_text.begin();
	w_text_end = w_text.end();

	// Регуляркой забираем только слова на русском языке и убираем все знаки препинания
	while (u32regex_search(w_text_start, w_text_end, result, u32rx)) {
		if (result.empty() == false) {
			clear_w_text << result[1] << " ";
			w_text_start = result[1].second;
		}
	}

	text = wstring_to_utf8(clear_w_text.str());

	texts << string(idptr) << "\t" << text << endl;

	//texts << (wchar_t*)jobptr << endl;
	//workr_log << get_time() << " [GEARMAN] # job received" << std::endl;

	//processd* proc = new processd(config);
	//proc->calculate_tonality_from_gearman(&texts[0]);
	//delete proc;

	texts.close();
	workr_log.close();
	
	*ret_ptr = GEARMAN_SUCCESS;
	*result_size = 6;

	char* r_result = (char*)malloc(10 * sizeof(char));
	return r_result;
}

// ôóíêöèÿ êîòîðàÿ çàãðóçèò êîíôèã çàíîâî
// è âíåñåò íóæíûå ïîïðàâêè â ðàáîòó
int ReloadConfig()
{
	config->get_general_params();
	config->get_directories_params();
	config->get_servers_params();
	config->get_svm_params();
	return 1;
}

// ôóíêöèÿ äëÿ îñòàíîâêè ïîòîêîâ è îñâîáîæäåíèÿ ðåñóðñîâ
void DestroyWorkThread()
{
	// òóò äîëæåí áûòü êîä êîòîðûé îñòàíîâèò âñå ïîòîêè è
	// êîððåêòíî îñâîáîäèò ðåñóðñû
	// O_o
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

    error_log.open(config->error_log, fstream::app);
    workr_log.open(config->worker_log, fstream::app);

    // çàïèøåì â ëîã ÷òî çà ñèãíàë ïðèøåë
    //WriteLog("[DAEMON] Signal: %s, Addr: 0x%0.16X\n", strsignal(sig), si->si_addr);
    error_log << get_time() << " # DAEMN: Signal - " << strsignal(sig) << " | Addr: - " << si->si_addr << endl;
    
    #if __WORDSIZE == 64 // åñëè äåëî èìååì ñ 64 áèòíîé ÎÑ
        // ïîëó÷èì àäðåñ èíñòðóêöèè êîòîðàÿ âûçâàëà îøèáêó
        ErrorAddr = (void*)((ucontext_t*)ptr)->uc_mcontext.gregs[REG_RIP];
    #else 
        // ïîëó÷èì àäðåñ èíñòðóêöèè êîòîðàÿ âûçâàëà îøèáêó
        ErrorAddr = (void*)((ucontext_t*)ptr)->uc_mcontext.gregs[REG_EIP];
    #endif

    // ïðîèçâåäåì backtrace ÷òîáû ïîëó÷èòü âåñü ñòåê âûçîâîâ 
    TraceSize = backtrace(Trace, 16);
    Trace[1] = ErrorAddr;

    // ïîëó÷èì ðàñøèôðîâêó òðàñèðîâêè
    Messages = backtrace_symbols(Trace, TraceSize);
    if (Messages)
    {
        error_log << "== Backtrace [start at: " << get_time() << "] ==" << endl;
        
        // çàïèøåì â ëîã
        for (x = 1; x < TraceSize; x++)
        {
            error_log << get_time() << " " << Messages[x] << endl;
        }
        
        error_log << "== End Backtrace [finished at: " << get_time() << "] ==" << endl;
        free(Messages);
    }

    workr_log << get_time() << " [ DAEMON] # [DAEMON] Stopped" << endl;
    
    // îñòàíîâèì âñå ðàáî÷èå ïîòîêè è êîððåêòíî çàêðîåì âñ¸ ÷òî íàäî
    DestroyWorkThread();
    
    workr_log.close();
    error_log.close();

    // çàâåðøèì ïðîöåññ ñ êîäîì òðåáóþùèì ïåðåçàïóñêà
    exit(CHILD_NEED_WORK);
}

int SetFdLimit(int MaxFd)
{
    struct 	rlimit lim;
    int		status;

    // çàäàäèì òåêóùèé ëèìèò íà êîë-âî îòêðûòûõ äèñêðèïòåðîâ
    lim.rlim_cur = MaxFd;
    // çàäàäèì ìàêñèìàëüíûé ëèìèò íà êîë-âî îòêðûòûõ äèñêðèïòåðîâ
    lim.rlim_max = MaxFd;

    // óñòàíîâèì óêàçàííîå êîë-âî
    status = setrlimit(RLIMIT_NOFILE, &lim);
    
    return status;
}

void set_pid_file(const char* Filename)
{
	ofstream pid_file;
	string pid_file_name = "/opt/opinion_miner/opinion_minerd.pid";
	pid_file.open(pid_file_name);
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

std::wstring utf8_to_wstring(const std::string& str)
{
    return utf_to_utf<wchar_t>(str.c_str(), str.c_str() + str.size());
}
std::string wstring_to_utf8(const std::wstring& w_str)
{
	return utf_to_utf<char>(w_str.c_str(), w_str.c_str() + w_str.size());
}