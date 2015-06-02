/*
*			Christopher Pac
*			10/15/2013
*			Operating Systems Lab 2
*			Scheduler
*
*************************************************************************************************
class - MySystem
	- Base class for MyOS
	- Keeps track of events (adding/deleting/recycling)
	- Loops through the events
class - MyOS		:Base MySystem										******Key Class******
	- This is the Discrete Event Simulation (DES) object
	- Manages state changes of events (i.e. creates new events based on current events)
	- Prints the statistics
	- Runs the simulation
	- Takes in an abstract Scheduler class
class - MyProcess
	- Keeps track of all process specific information and statistics
class - MyEvent
	- Base class for PSateEvent
	- Has timer info
class - PSateEvent
	- This is the event class that DES manages
	- Has state information
class - MyScheduler														******Key Class******
	- abstract class
	- This is the required "object oriented" class that DES uses
	- Contains three pure virtual functions
		put_process	- takes MyProcess ptr
		get_process	- returns MyProcess ptr
		PrintMyName - used to print the name of the specific Scheduler
	- Scheduler is only aware of a processes not events
class - FCFS		:Base MyScheduler
	- first-come first-served scheduler
	- implemented as a queue
class - LCFS		:Base MyScheduler
	- last-come first-served scheduler
	- implemented as a stack
class - SJF		:Base MyScheduler
	- shortest job first scheduler
	- implemented as a list
class - RR		:Base MyScheduler
	- Round-Robin scheduler
	- implemented as a queue
	- Only RR Scheduler knows about a quantum
Notes:
There are four events that a process goes through: Running, Ready, Blocked, and Terminate. This is specified by PSateType. 
For clarity and easy I added an extra event called 'Terminate' to indicate a process has finished its run and it is done.
There is no 'preempt' event since that is just a Ready event after the process has run its quantum.
*************************************************************************************************
*
*
*			
*/


// MyScheduler.cpp : Defines the entry point for the console application.
//

#include <stdio.h>
#include <iostream>
#include <fstream>
#include <string>
#include <queue>
#include <stack>
#include <list>
using namespace std;

typedef enum p_state
{
	Running = 0,
	Ready,
	Blocked,
	Terminate,
} PStateType;

class MyEvent
{
public:
	MyEvent();
	MyEvent(int time, int ts);
	~MyEvent();
	void Discard(){m_bDiscard = true;};

	int m_nTime;
	int m_nTimeStamp;
	bool m_bDiscard;
};

MyEvent::MyEvent()
{
	m_nTime = 0;
	m_nTimeStamp = 0;
	m_bDiscard = false;
}
MyEvent::MyEvent(int time, int ts)
{
	m_nTime = time;
	m_nTimeStamp = ts;
	m_bDiscard = false;
}

MyEvent::~MyEvent()
{
}

class MySystem
{
public:
	MySystem();
	~MySystem();

	void Run();
public:
	void addEvent(MyEvent *p);
	void delEvent(MyEvent *p = 0);

protected:
	virtual void ExecEvent(MyEvent *p) = 0;  
	virtual void Schedule() = 0;  

protected:
	int get_Now();

private:
	void Exec(MyEvent *p);

private:
	int m_nNow;

	list<MyEvent*> m_EventList;
	list<MyEvent*>::iterator m_itrEventList;
	MyEvent* m_pCurrentEvent;

private:
	// vector of unused events so we dont keep calling new
	vector<MyEvent*> m_vDiscardEvents;
protected:
	template<typename T>
	T* create_newEvent();
};

MySystem::MySystem()
{
	m_pCurrentEvent = 0;
	m_nNow = 0;
}

MySystem::~MySystem()
{
	for (vector<MyEvent*>::iterator it = m_vDiscardEvents.begin() ; it != m_vDiscardEvents.end(); ++it)
		delete *it;
}

template<typename T>
T* MySystem::create_newEvent()
{
	T* pEvent = 0;
	if (!m_vDiscardEvents.empty())
	{
		pEvent = (T*)m_vDiscardEvents.back();
		m_vDiscardEvents.pop_back();
	}
	else
		pEvent = new T();
	return pEvent;
}

int MySystem::get_Now()
{
	return m_nNow;
}

void MySystem::addEvent(MyEvent *p)
{
	for (m_itrEventList = m_EventList.begin(); m_itrEventList != m_EventList.end(); ++m_itrEventList)
	{
		if ((*m_itrEventList)->m_nTime > p->m_nTime)
			break;
	}

	m_EventList.insert(m_itrEventList, p);
}

void MySystem::delEvent(MyEvent *p)
{
	if (p)
	{
		m_EventList.pop_front();

		if (p->m_bDiscard)
		{
			//delete p;
			m_vDiscardEvents.push_back(p);
			p->m_bDiscard = false;
		}
	}
}

void MySystem::Exec(MyEvent *p)
{
	ExecEvent(p);
}

void MySystem::Run()
{
	m_pCurrentEvent = m_EventList.front();
	while (m_pCurrentEvent)
	{
		m_nNow = m_pCurrentEvent->m_nTime;
		Exec(m_pCurrentEvent);

		delEvent(m_pCurrentEvent);

		m_pCurrentEvent = m_EventList.empty() ? 0 : m_EventList.front();
		
		if (m_pCurrentEvent == 0 || m_nNow != m_pCurrentEvent->m_nTime)
			Schedule();

		m_pCurrentEvent = m_EventList.empty() ? 0 : m_EventList.front();
	}
}

class MyProcess;
class MyScheduler
{
public:
	MyScheduler();
	~MyScheduler();

	virtual void put_process(MyProcess *p) = 0;
	virtual MyProcess* get_process() = 0;
	virtual void PrintMyName() = 0;
};

MyScheduler::MyScheduler()
{
}

MyScheduler::~MyScheduler()
{
}

class MyProcess
{
public:
	MyProcess();
	MyProcess(int id, int at, int tc, int cb, int io);
	~MyProcess();

public:
	int getpid(){return pid;};
	int getCB(){return CB;};
	void setCB(int n);
	void setIO(int n){IO = n; totalIO+=IO;};
	int getIO(){return IO;};
	int getAT(){return AT;};
	int getTC(){return TC;};


	void setRunTime(int n);
	int getRunTime(){return runtime;};
	int getRemainingRunTime() {return TC - totalruntime;};
	int getRemainingCB(){return remCB;};

	int getTotalIO(){return totalIO;};
	int getCPUWaitTime(){return totalInactiveTime;};
	
	bool operator > (MyProcess* p2);

	int getseedIO(){return seedIO;};
	int getseedCB(){return seedCB;};
	
	void setFinishTime(int n){finishtime = n;};
	int getFinishTime(){return finishtime;};

	void setTime(int t);
private:
	int pid;
	int AT;
	int TC;
	int CB;
	int IO;
	int runtime;
	int totalruntime;
	int totalIO;
	int remCB;
	int seedIO;
	int seedCB;
	int totalInactiveTime;
	int totalInactiveTimeTmp;
	int finishtime;
};

MyProcess::MyProcess()
{
	pid = 0; AT = 0; TC = 0; CB = 0; IO = 0;
	runtime = 0; finishtime = 0;
	totalruntime = 0; totalIO = 0; remCB = 0;
	seedIO = 0; seedCB = 0;
	totalInactiveTime = 0; totalInactiveTimeTmp = 0;
}

MyProcess::MyProcess(int id, int at, int tc, int cb, int io)
{
	pid = id; AT = at; TC = tc; CB = cb; IO = io;
	runtime = 0; finishtime = 0;
	totalruntime = 0; totalIO = 0; remCB = 0;
	seedIO = io; seedCB = cb;
	totalInactiveTime = 0; totalInactiveTimeTmp = 0;
}

MyProcess::~MyProcess()
{
}

void MyProcess::setTime(int t)
{
	if (totalInactiveTimeTmp == 0)
		totalInactiveTimeTmp = t;
	else
	{
		totalInactiveTime += (t - totalInactiveTimeTmp);
		totalInactiveTimeTmp = 0;
	}
}

bool MyProcess::operator > (MyProcess* p2)
{
	return this->getRemainingRunTime() > p2->getRemainingRunTime();
}

void MyProcess::setRunTime(int n)
{
	int ntime = getRemainingRunTime();
	runtime = (n > ntime) ? ntime : n;
	runtime = n;
}

void MyProcess::setCB(int n)
{
	int ntime = getRemainingRunTime();
	CB = (n > ntime) ? ntime : n;

	// if run time is 0 we use CB
	if (getRunTime() == 0 || getRunTime() > CB)
		runtime = CB;
		
	totalruntime += runtime;

	// recalc the CB now
	remCB = CB - getRunTime();
	if (remCB < 0)
		remCB = 0;
}

class PSateEvent : public MyEvent
{
public:
	PSateEvent();
	PSateEvent(int time, int createtime, PStateType newstate, PStateType ostate, MyProcess *p);
	~PSateEvent();

	PStateType state;
	PStateType oldstate;

	MyProcess *pProc;
};

PSateEvent::PSateEvent()
{
	pProc = 0;
	state = Ready;
	oldstate = Ready;
}

PSateEvent::PSateEvent(int time, int createtime, PStateType newstate, PStateType ostate, MyProcess *p):MyEvent(time, createtime)
{
	oldstate = ostate;
	state = newstate;
	pProc = p;
}

PSateEvent::~PSateEvent()
{
}


class MyOS : public MySystem
{
public:
	MyOS();
	MyOS(const char *infile, const char *randfile, bool bVerbose = false);
	~MyOS();

	void Run(MyScheduler* p);
	void Print();

protected:
	virtual void ExecEvent(MyEvent *p);  
	virtual void Schedule();

	void createPSateChangeEvent(int time, int createtime, MyProcess *proc, PStateType newstate, PStateType oldstate = Ready);
	void createPSateChangeEvent(int time, int createtime, PSateEvent *pstateevent, PStateType newstate);
private:
	long m_lRandMax;
	long *m_plRandNums;
	long m_lOFS;
	int myrandom(int burst);

private:
	MyScheduler* m_pScheduler;
	int m_nRunning;						// -1 if no proc running, pid otherwise
	vector<MyProcess*> m_vProcesses;
	int m_nFinishTime;

private:
	void Print(PSateEvent *pstateevent);
	bool m_bVerboseMode;
private:
	int m_nEndIO;
	int m_nTotalIO;
};

MyOS::MyOS()
{
	m_lOFS = -1;
	m_lRandMax = 0;
	m_pScheduler = 0;
	m_plRandNums = 0;
	m_nRunning = -1;
	m_nFinishTime = 0;

	m_nEndIO = 0;
	m_nTotalIO = 0;

	m_bVerboseMode = false;
}

void MyOS::Run(MyScheduler* p)
{
	m_pScheduler = p;
	MySystem::Run();
}

MyOS::MyOS(const char *infile, const char *randfile, bool bVerbose)
{
	m_lOFS = -1;
	m_lRandMax = 0;
	m_pScheduler = 0;
	m_plRandNums = 0;
	m_nRunning = -1;
	m_nFinishTime = 0;
	m_nEndIO = 0;
	m_nTotalIO = 0;
	m_bVerboseMode = bVerbose;

	ifstream fileIn (infile);
	ifstream fileRand (randfile);

	if (!fileIn.is_open() || !fileRand.is_open())
		throw;

	long idx = 0;
	if (!fileRand.eof())
		fileRand >> m_lRandMax;

	m_plRandNums = new long[m_lRandMax];

	while (!fileRand.eof() && idx < m_lRandMax)
	{
		fileRand >> m_plRandNums[idx++];
	}


	MyProcess *proc = 0;
	int pid = 0;
	int time, TC, CB, IO;

	while (fileIn.good())
	{
		fileIn >> time;
		if (fileIn.good())
		{
			fileIn >> TC >> CB >> IO;
			proc = new MyProcess(pid++, time, TC, CB, IO);
			m_vProcesses.push_back(proc);

			createPSateChangeEvent(time, time, proc, Ready);
		}
	}
}

MyOS::~MyOS()
{
	delete []m_plRandNums;
	delete m_pScheduler;

	for (vector<MyProcess*>::iterator it = m_vProcesses.begin() ; it != m_vProcesses.end(); ++it)
		delete *it;
}

void MyOS::Print(PSateEvent *pstateevent)
{
	/*
	==> 0 0 ts=0 READY  dur=0
	T(0:0): READY -> READY
	*/
		string strstate[] = {
		"RUNNG",			
		"READY",
		"BLOCK",
		"TERMINATE"
	};

	cout << "==> " << pstateevent->m_nTime << " " << pstateevent->pProc->getpid() << " ts=" << pstateevent->m_nTimeStamp << " " << strstate[pstateevent->state]
		<< "  dur=" << pstateevent->m_nTime - pstateevent->m_nTimeStamp << endl;

	if (pstateevent->state != Terminate)
	{
		cout << "T(" << pstateevent->pProc->getpid() << ":" << pstateevent->m_nTime << "): " << strstate[pstateevent->oldstate] << " -> " << strstate[pstateevent->state]
			<< endl << endl;
	}
	else
	{
		cout << "T(" << pstateevent->pProc->getpid() << "): Done" << endl;
	}
}

int MyOS::myrandom(int burst)
{
	if (++m_lOFS >= m_lRandMax)
		m_lOFS = 0;
	int myrnd = 1 + (m_plRandNums[m_lOFS] % burst);
	return myrnd;
}

void MyOS::createPSateChangeEvent(int time, int createtime, MyProcess *proc, PStateType newstate, PStateType oldstate)
{
	PSateEvent *newEvent = create_newEvent<PSateEvent>();
	newEvent->pProc = proc;
	newEvent->state = oldstate;
	createPSateChangeEvent(time, createtime, newEvent, newstate);
}

void MyOS::createPSateChangeEvent(int time, int createtime, PSateEvent *pstateevent, PStateType newstate)
{
	pstateevent->m_nTime = time;
	pstateevent->oldstate = pstateevent->state;
	pstateevent->state = newstate;
	pstateevent->m_nTimeStamp = createtime;
	addEvent(pstateevent);
}

void MyOS::Schedule()
{
	if (m_nRunning != -1)
		return;

	// ask the Scheduler for a process to run!
	MyProcess *proc = m_pScheduler->get_process();

	// cpu IDLE if proc is NULL
	if (proc)
	{
		int nNow = get_Now();
			int nCB = proc->getRemainingCB();
			if (!nCB)
				nCB = myrandom(proc->getseedCB());
		
			proc->setCB(nCB);
		proc->setTime(nNow);

		createPSateChangeEvent(nNow, nNow, proc, Running);
		m_nRunning = proc->getpid();
	}
}

void MyOS::ExecEvent(MyEvent *p)
{
	PSateEvent *stateEvent = (PSateEvent*)p;
	int nNow = get_Now();


	if (stateEvent)
	{
		// print
		if (m_bVerboseMode)
			Print(stateEvent);

		if (stateEvent->state == Ready)
		{
			stateEvent->pProc->setTime(nNow);
			// give the process in the event to a scheduler
			m_pScheduler->put_process(stateEvent->pProc);
			// we are done with this event
			stateEvent->Discard();

			if (stateEvent->pProc->getpid() == m_nRunning)
				m_nRunning = -1;
		}
		else if (stateEvent->state == Blocked)
		{
			int nIO = myrandom(stateEvent->pProc->getseedIO());
			stateEvent->pProc->setIO(nIO);

			// create new even based on this Blocked event
			createPSateChangeEvent(nNow + nIO, nNow, stateEvent, Ready);

			if (stateEvent->pProc->getpid() == m_nRunning)
				m_nRunning = -1;

			// compute IO
			int nVal = 0;
			nVal = (nNow + nIO) - m_nEndIO;
			if (nVal > 0)
			{
				m_nTotalIO += nVal;

				nVal = nNow - m_nEndIO;
				if (nVal > 0)
					m_nTotalIO -= nVal;

				m_nEndIO = nNow + nIO;
			}
		}
		else if (stateEvent->state == Running)
		{
			PStateType procState;
			if (stateEvent->pProc->getRemainingRunTime() == 0)
				procState = Terminate;
			else
				procState = (stateEvent->pProc->getRemainingCB()) ? Ready : Blocked;
			
			createPSateChangeEvent(nNow + stateEvent->pProc->getRunTime(), nNow, stateEvent, procState);
		}
		else if (stateEvent->state == Terminate)
		{
			if (stateEvent->pProc->getpid() == m_nRunning)
				m_nRunning = -1;

			stateEvent->pProc->setFinishTime(nNow);
			m_nFinishTime = nNow;

			// done with event
			stateEvent->Discard();
		}
		else
			cout << "Unknow State??" << endl;
	}
}

void MyOS::Print()
{
	m_pScheduler->PrintMyName();

	int nCPUWait = 0; int nIO = 0; int nTC = 0; int nTT = 0;
	double dNumProc = (double)m_vProcesses.size();

	double dCPUutil = 0.0;
	double dIOutil = 0.0;
	double dAvgTurnaround = 0.0;
	double dAvgCPUwaiting = 0.0;
	double dThroughput = 0.0;

	for (vector<MyProcess*>::iterator it = m_vProcesses.begin() ; it != m_vProcesses.end(); ++it)
	{
		// pid: AT TC CB IO | FT TT IT CW

		int nVal = (*it)->getpid();
		nVal = (*it)->getAT();

		nTC = (*it)->getTC();
		dCPUutil += nTC;

		nVal = (*it)->getseedCB();
		nVal = (*it)->getseedIO();
		nVal = (*it)->getFinishTime();
		nTT = (*it)->getFinishTime() - (*it)->getAT();
		dAvgTurnaround += nTT;

		nIO = (*it)->getTotalIO();
		nCPUWait = (*it)->getCPUWaitTime();
		dAvgCPUwaiting+=nCPUWait;

		printf("%04d: %4d %4d %4d %4d | %4d %4d %4d %4d\n", (*it)->getpid(), (*it)->getAT(), nTC, (*it)->getseedCB(), (*it)->getseedIO(), 
			(*it)->getFinishTime(), nTT, nIO, nCPUWait);
	}

	dCPUutil = dCPUutil/(double)m_nFinishTime;
	dCPUutil = dCPUutil * 100.0;

	dAvgTurnaround = dAvgTurnaround/dNumProc;
	dAvgCPUwaiting = dAvgCPUwaiting/dNumProc;

	dThroughput = dNumProc/(double)m_nFinishTime;
	dThroughput = dThroughput * 100.0;

	dIOutil = (double)m_nTotalIO/(double)m_nFinishTime;
	dIOutil = dIOutil * 100.0;

	printf("SUM: %d %.2lf %.2lf %.2lf %.2lf %.3lf\n",m_nFinishTime, dCPUutil, dIOutil, dAvgTurnaround, dAvgCPUwaiting, dThroughput);

}

// first-come first-served
class FCFS: public MyScheduler
{
public:
	FCFS();
	~FCFS();

	virtual void put_process(MyProcess *p);
	virtual MyProcess* get_process();
	virtual void PrintMyName(){printf("FCFS\n");};

protected:
	queue<MyProcess*> m_ProcessContainer;
};

FCFS::FCFS()
{
}

FCFS::~FCFS()
{
}

void FCFS::put_process(MyProcess *p)
{
	m_ProcessContainer.push(p);
}

MyProcess* FCFS::get_process()
{
	if (!m_ProcessContainer.empty())
	{
		MyProcess *p = m_ProcessContainer.front();
		m_ProcessContainer.pop();

		// Nonpreemptive so set the running time to 0
		p->setRunTime(0);
	
		return p;
	}

	return 0;
}

// last-come first-served
class LCFS: public MyScheduler
{
public:
	LCFS();
	~LCFS();

	virtual void put_process(MyProcess *p);
	virtual MyProcess* get_process();
	virtual void PrintMyName(){printf("LCFS\n");};

protected:
	stack<MyProcess*> m_ProcessContainer;
};

LCFS::LCFS()
{
}

LCFS::~LCFS()
{
}

void LCFS::put_process(MyProcess *p)
{
	m_ProcessContainer.push(p);
}

MyProcess* LCFS::get_process()
{
	if (!m_ProcessContainer.empty())
	{
		MyProcess *p = m_ProcessContainer.top();

		m_ProcessContainer.pop();

		// Nonpreemptive so set the running time to 0
		p->setRunTime(0);
	
		return p;
	}
	return 0;
}

// shortest job first
class SJF: public MyScheduler
{
public:
	SJF();
	~SJF();

	virtual void put_process(MyProcess *p);
	virtual MyProcess* get_process();
	virtual void PrintMyName(){printf("SJF\n");};

protected:
	list<MyProcess*> m_ProcessContainer;
	list<MyProcess*>::iterator m_itrProcess;
};

SJF::SJF()
{
}

SJF::~SJF()
{
}

void SJF::put_process(MyProcess *p)
{
	for (m_itrProcess = m_ProcessContainer.begin(); m_itrProcess != m_ProcessContainer.end(); ++m_itrProcess)
	{
		if (**m_itrProcess > p)
			break;
	}

	m_ProcessContainer.insert(m_itrProcess, p);
}

MyProcess* SJF::get_process()
{
	if (!m_ProcessContainer.empty())
	{
		MyProcess *p = m_ProcessContainer.front();
		m_ProcessContainer.pop_front();

		// Nonpreemptive so set the running time 0
		p->setRunTime(0);
		return p;
	}

	return 0;
}

// Round-Robin
class RR: public MyScheduler
{
public:
	RR();
	RR(int q);
	~RR();

	virtual void put_process(MyProcess *p);
	virtual MyProcess* get_process();
	virtual void PrintMyName(){printf("RR %d\n", m_quantum);};

protected:
	queue<MyProcess*> m_ProcessContainer;
	int m_quantum;
};

RR::RR()
{
	m_quantum = 0;
}

RR::RR(int q)
{
	m_quantum = q;
}

RR::~RR()
{
}

void RR::put_process(MyProcess *p)
{
	m_ProcessContainer.push(p);
}

MyProcess* RR::get_process()
{
	if (!m_ProcessContainer.empty())
	{
		MyProcess *p = m_ProcessContainer.front();
		m_ProcessContainer.pop();

		// preemptive so set the running time to the quantum
		p->setRunTime(m_quantum);
		return p;
	}

	return 0;
}


int main(int argc, char* argv[])
{
	// see TOP of page for extended notes and information

	bool bVerbose = false;
	int nQuantum = 0;
	int nIn = 0;
	int nRand = 0;
	string strScheduler = "-sF";

	if (argc < 3)
	{
		cout << "Error no files" << endl;
		return 0;
	}

	string strInput;

	for (int i = 1; i < argc; i++)
	{
		strInput = argv[i];

		if (strInput[0] == '-')
		{
			if (strInput[1] == 'v')
				bVerbose = true;
			else if (strInput[1] == 's')
				strScheduler = strInput;
		}
		else
		{
			if (!nIn)
				nIn = i;
			else
				nRand = i;
		}
	}

	if (strScheduler.size() < 3)
		strScheduler = "---";

	MyScheduler *pSched = 0;

	switch (strScheduler[2])
	{
		case 'F':
			pSched = new FCFS();
			break;
		case 'L':
			pSched = new LCFS();
			break;
		case 'S':
			pSched = new SJF();
			break;
		case 'R':
			nQuantum = 	(strScheduler.size() > 3)? stoi(strScheduler.substr(3)) : 0;
			pSched = new RR(nQuantum);
			break;
		default:
			cout << "No Scheduler provided" << endl;
			return 0;
	}

	// create the DES
	MyOS myOS (argv[nIn], argv[nRand], bVerbose);

	// run it with with some Scheduler
	myOS.Run(pSched);

	// print the statistics
	myOS.Print();

	return 0;
}

