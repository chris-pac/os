/*
*			Christopher Pac
*			12/09/2013
*			Operating Systems Lab 4
*			IO Management
*
*
*
*/

// IOSched.cpp : Defines the entry point for the console application.
//

//#include "stdafx.h"
#include <stdio.h>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <list>
#include <limits>
#include <queue>
#include <iomanip>
using namespace std;

struct opIO{
	int time;
	int track;
	int id;
};

class IOSched
{
public:
	IOSched();
	~IOSched();

	virtual bool getIO(opIO &io) = 0;
	virtual void putIO(opIO io) = 0;
};

IOSched::IOSched()
{
}

IOSched::~IOSched()
{
}


class MySim
{
public:
	MySim();
	MySim(const char *infile);
	~MySim();

	void Run(IOSched *psched);
	void Print();


protected:
	long get_Count(){return m_lCount;};

protected:
	long m_lCount;

	ifstream fileIn;
	queue<opIO> m_qInput;


	unsigned long total_time;
	unsigned long tot_movement;
	double avg_turnaround;
	double avg_waittime;
	unsigned long max_waittime;
};

MySim::MySim()
{
	m_lCount = 0;
}

MySim::MySim(const char *infile)
{
	m_lCount = -1;

	total_time = 0;
	tot_movement = 0;
	avg_turnaround = 0.0;
	avg_waittime = 0.0;
	max_waittime = 0;

	fileIn.open(infile);

	if (!fileIn.is_open())
		throw;

	char c = '#';
	opIO inIO;

	while (fileIn.good())
	{
		c = fileIn.peek();
		while (c == '\n' || c == 'b' || c == '\t' || c == '#')
		{
			if (c == '#')
				fileIn.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
			else
				fileIn.get();

			c = fileIn.peek();
			if (c == char_traits<char>::eof())
				return;
		}
		
		fileIn >> inIO.time;

		if (fileIn.good())
		{
			fileIn >> inIO.track;
			m_qInput.push(inIO);
		}
	}
}

MySim::~MySim()
{
}

void MySim::Print()
{
	m_lCount++;

	if (m_lCount>0)
	{
		avg_turnaround = avg_turnaround/m_lCount;
		avg_waittime = avg_waittime / m_lCount;
	}

	printf("SUM: %d %d %.2lf %.2lf %d\n",
		total_time,
		tot_movement,
		avg_turnaround,
		avg_waittime,
		max_waittime);
}

void MySim::Run(IOSched *psched)
{
	int currenttrack = 0;
	opIO io;
	opIO io_now;
	int endTime = 0;
	int startTime = 0;
	int track = 0;
	int diff = 0;
	unsigned long waitime = 0;
	while (!m_qInput.empty())
	{
		m_lCount++;
		io = m_qInput.front();
		m_qInput.pop();
		io.id = m_lCount;
		psched->putIO(io);
		//cout << io.time << ":" << setw(6) << io.id << " add " << io.track << endl;
		startTime = io.time;

		while (psched->getIO(io_now))
		{
			//cout << startTime << ":" << setw(6) << io_now.id << " issue " << io_now.track << " " << currenttrack << endl;
			currenttrack = io_now.track;
			diff = track - io_now.track;
			diff = abs(diff);// >= 0 ? diff: -diff;
			endTime = startTime + diff;
			tot_movement += diff;

			if (!m_qInput.empty())
				io = m_qInput.front();
			while (!m_qInput.empty() && (io.time <= endTime)) // less then or should it be less then or equal
			{
				m_lCount++;
				m_qInput.pop();
				io.id = m_lCount;
				psched->putIO(io);
				//cout << io.time << ":" << setw(6) << io.id << " add " << io.track << endl;

				if (!m_qInput.empty())
					io = m_qInput.front();
			}

			avg_turnaround += endTime - io_now.time;
			waitime = startTime - io_now.time;
			avg_waittime += waitime;

			if (waitime > max_waittime)
				max_waittime = waitime;

			track = io_now.track;
			//cout << endTime << ":" << setw(6) << io_now.id << " finish " << endTime-io_now.time << endl;
			startTime = endTime;

		}
	}

	total_time = endTime;
}


class FIFO: public IOSched
{
public:
	virtual bool getIO(opIO &io);
	virtual void putIO(opIO io);
protected:
	queue<opIO> m_qIO;

};

bool FIFO::getIO(opIO &io)
{
	if (!m_qIO.empty())
	{
		io = m_qIO.front();
		m_qIO.pop();
		return true;
	}

	return false;
}

void FIFO::putIO(opIO io)
{
	m_qIO.push(io);
}

class SSTF: public IOSched
{
public:
	SSTF();
	~SSTF();

	virtual bool getIO(opIO &io);
	virtual void putIO(opIO io);
protected:
	list<opIO> m_lIO;

	int m_nLastTrack;
	
	list<opIO>::iterator m_itrRet;
};

SSTF::SSTF()
{
	m_nLastTrack = 0;
}

SSTF::~SSTF()
{
}

bool SSTF::getIO(opIO &io)
{
	if (m_lIO.empty())
		return false;
	list<opIO>::iterator itrR;

	itrR = m_lIO.begin();
	m_itrRet = m_lIO.begin();
	int nDiff = (*m_itrRet).track - m_nLastTrack;
	nDiff = abs(nDiff);
	++m_itrRet;

	int nDiff2 = 0;

	for (; m_itrRet != m_lIO.end(); ++m_itrRet)
	{
		nDiff2 = (*m_itrRet).track - m_nLastTrack;
		nDiff2 = abs(nDiff2);

		if (nDiff2 < nDiff)
		{
			itrR = m_itrRet;
			nDiff = nDiff2;
		}
	}

	io = *itrR;
	m_lIO.erase(itrR);
	m_nLastTrack = io.track;

	return true;
}

void SSTF::putIO(opIO io)
{
	m_lIO.push_back(io);
}

///////////////////////////////////
class SCAN: public IOSched
{
public:
	SCAN();
	~SCAN();

	virtual bool getIO(opIO &io);
	virtual void putIO(opIO io);

	bool IsEmpty(){return m_lIO.empty();};
	SCAN & operator= (const SCAN & other);
	bool m_bUP;
protected:
	list<opIO> m_lIO;

	int m_nLastTrack;
	
	list<opIO>::iterator m_itrRet;

};

SCAN::SCAN()
{
	m_nLastTrack = 0;
	m_bUP = true;
}

SCAN::~SCAN()
{
}

SCAN & SCAN::operator= (const SCAN & other)
{
	m_nLastTrack = other.m_nLastTrack;
	m_bUP = other.m_bUP;
	return *this;
};

bool SCAN::getIO(opIO &io)
{
	if (m_lIO.empty())
		return false;

	for (m_itrRet = m_lIO.begin(); m_itrRet != m_lIO.end(); ++m_itrRet)
	{
		if (m_nLastTrack <= (*m_itrRet).track)
			break;
	}

	if (m_bUP)
	{
		if (m_itrRet == m_lIO.end())
		{
			m_bUP = false;
			--m_itrRet;
		}
	}
	else
	{
		if (m_itrRet == m_lIO.begin())
			m_bUP = true;
		else
		{
			if (m_itrRet == m_lIO.end() || m_nLastTrack != (*m_itrRet).track)
			{
				--m_itrRet;

				if (m_itrRet != m_lIO.begin())
				{
					m_nLastTrack = (*m_itrRet).track;

					do
					{
						--m_itrRet;
					} while (m_nLastTrack == (*m_itrRet).track && m_itrRet != m_lIO.begin());
				
					if (m_nLastTrack != (*m_itrRet).track)
						++m_itrRet;
				}
			}
		}



	}

	io = *m_itrRet;
	m_lIO.erase(m_itrRet);
	m_nLastTrack = io.track;


	return true;
}

void SCAN::putIO(opIO io)
{
	for (m_itrRet = m_lIO.begin(); m_itrRet != m_lIO.end(); ++m_itrRet)
	{
		if (io.track < (*m_itrRet).track)
			break;
	}

	m_lIO.insert(m_itrRet, io);

}

///////////////////////////////////
class CSCAN: public IOSched
{
public:
	CSCAN();
	~CSCAN();

	virtual bool getIO(opIO &io);
	virtual void putIO(opIO io);
protected:
	list<opIO> m_lIO;

	int m_nLastTrack;
	
	list<opIO>::iterator m_itrRet;
};

CSCAN::CSCAN()
{
	m_nLastTrack = 0;
}

CSCAN::~CSCAN()
{
}

bool CSCAN::getIO(opIO &io)
{
	if (m_lIO.empty())
		return false;

	for (m_itrRet = m_lIO.begin(); m_itrRet != m_lIO.end(); ++m_itrRet)
	{
		if (m_nLastTrack <= (*m_itrRet).track)
			break;
	}

	if (m_itrRet == m_lIO.end())
	{
		m_itrRet = m_lIO.begin();
	}

	io = *m_itrRet;
	m_lIO.erase(m_itrRet);
	m_nLastTrack = io.track;


	return true;
}

void CSCAN::putIO(opIO io)
{
	for (m_itrRet = m_lIO.begin(); m_itrRet != m_lIO.end(); ++m_itrRet)
	{
		if (io.track < (*m_itrRet).track)
			break;
	}

	m_lIO.insert(m_itrRet, io);

}

///////////////////////////////////
class FSCAN: public IOSched
{
public:
	FSCAN();
	~FSCAN();

	virtual bool getIO(opIO &io);
	virtual void putIO(opIO io);
protected:
	list<opIO> m_lIO1;
	list<opIO> m_lIO2;

	list<opIO>* m_pputIO;
	list<opIO>* m_pgetIO;
	list<opIO>* m_ptmpIO;

	int m_nLastTrack;
	
	list<opIO>::iterator m_itrRet;

	bool m_bUP;
};

FSCAN::FSCAN()
{
	m_pputIO = &m_lIO1;
	m_pgetIO = &m_lIO2;

	m_nLastTrack = 0;
	m_bUP = true;
}

FSCAN::~FSCAN()
{
}

bool FSCAN::getIO(opIO &io)
{
	if (m_pgetIO->empty() && m_pputIO->empty())
		return false;

	if (m_pgetIO->empty())
	{
		m_ptmpIO = m_pgetIO;
		m_pgetIO = m_pputIO;
		m_pputIO = m_ptmpIO;
		m_bUP = true;
	}

	for (m_itrRet = m_pgetIO->begin(); m_itrRet != m_pgetIO->end(); ++m_itrRet)
	{
		if (m_nLastTrack <= (*m_itrRet).track)
			break;
	}

	if (m_bUP)
	{
		if (m_itrRet == m_pgetIO->end())
		{
			m_bUP = false;
			--m_itrRet;
		}
	}
	else
	{
		if (m_itrRet != m_pgetIO->begin())
		{
			if (m_itrRet == m_pgetIO->end())
				--m_itrRet;
			else if (m_nLastTrack != (*m_itrRet).track)
				--m_itrRet;
		}
		
		if (m_itrRet == m_pgetIO->begin())
			m_bUP = true;
		else
		{
			if (m_nLastTrack != (*m_itrRet).track)
			{
				m_nLastTrack = (*m_itrRet).track;

				do
				{
					--m_itrRet;
				} while (m_nLastTrack == (*m_itrRet).track && m_itrRet != m_pgetIO->begin());
				
				if (m_nLastTrack != (*m_itrRet).track)
					++m_itrRet;
			}
		}

	}

	io = *m_itrRet;
	m_pgetIO->erase(m_itrRet);
	m_nLastTrack = io.track;


	return true;
}

void FSCAN::putIO(opIO io)
{
	for (m_itrRet = m_pputIO->begin(); m_itrRet != m_pputIO->end(); ++m_itrRet)
	{
		if (io.track < (*m_itrRet).track)
			break;
	}

	m_pputIO->insert(m_itrRet, io);
}

//./iosched –s<f|s|S|C|F> <inputfule>
int main(int argc, char* argv[])
{
	char cAlgo = 'f';
	int nIn = 0;

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
			if (strInput[1] == 's')
				cAlgo = strInput[2];
		}
		else
		{
			if (!nIn)
				nIn = i;
		}
	}

	IOSched *psched = 0;

	switch (cAlgo)
	{
		case 'f':
			psched = new FIFO();
			break;
		case 's':
			psched = new SSTF();
			break;
		case 'S':
			psched = new SCAN();
			break;
		case 'C':
			psched = new CSCAN();
			break;
		case 'F':
			psched = new FSCAN();
			break;
		default:
			cout << "No scheduling algorithm provided" << endl;
			return 0;
	}

	MySim mySim (argv[nIn]);

	mySim.Run(psched);

	mySim.Print();

	//system("pause");
	return 0;
}
