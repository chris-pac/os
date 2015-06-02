/*
*			Christopher Pac
*			9/23/2013
*			Operating Systems Lab 3
*			Virtual Memory Management
*
*
*
*/

// MMU.cpp : Defines the entry point for the console application.
//

#include <stdio.h>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <list>
#include <limits>
using namespace std;

#define PTE_SIZE 64

#define B_SET(x, n)			(x = x | n)
#define B_UNSET(x, n)		(x = x ^ n)
#define B_IS_SET(x, n)      ((x & n)?true:false)
#define B_SET_IDX(x, i)     (x = (((x>>4)<<4)^x) | (i<<4))
#define B_GET_IDX(x)		(x>>4)
enum pte_flags {
	PRESENT				= 0x001,
	MODIFIED			= 0x002,
	REFERENCED			= 0x004,
	PAGEDOUT			= 0x008
};

enum option_flags {
	opO			= 0x001,
	opP			= 0x002,
	opF			= 0x004,
	opS			= 0x008,
	opp			= 0x010,
	opf			= 0x020
};

class MySystem
{
public:
	MySystem();
	MySystem(const char *infile);
	~MySystem();

	void Run();

protected:
	virtual void ExecInstruction(int rw, int inst) = 0;  

protected:
	long get_Count(){return m_lCount;};

private:
	void Exec(int rw, int inst);

protected:
	long m_lCount;

	ifstream fileIn;
};

MySystem::MySystem()
{
	m_lCount = 0;
}

MySystem::MySystem(const char *infile)
{
	m_lCount = -1;

	fileIn.open(infile);

	if (!fileIn.is_open())
		throw;
}

MySystem::~MySystem()
{
}

void MySystem::Exec(int rw, int inst)
{
	m_lCount++;
	ExecInstruction(rw, inst);
}

void MySystem::Run()
{
	char c = '#';
	int rw = 0;
	int inst = 0;

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
		
		fileIn >> rw;

		if (fileIn.good())
		{
			fileIn >> inst;
			Exec(rw, inst);	
		}
	}
}

class MyReplacement
{
public:
	MyReplacement();
	~MyReplacement();

	virtual int GetFrame(vector<long> &vPTE, vector<int> &vFrames) = 0;
	virtual void SetUsed(int inst, int frame){};
protected:
	bool m_bFreeFrames;
};

MyReplacement::MyReplacement()
{
	m_bFreeFrames = true;
}

MyReplacement::~MyReplacement()
{
}

class MyMMU : public MySystem
{
public:
	MyMMU();
	MyMMU(const char *infile, string strOptions, int nFrames);
	~MyMMU();

	void Run(MyReplacement* p);
	void Print();

protected:
	virtual void ExecInstruction(int rw, int inst);  

private:
	MyReplacement* m_pReplacement;

private:
	vector<long> m_vPTE;
	vector<int> m_vFrames;

private:
	enum operations
	{
		UNMAP = 0,
		OUT,
		IN,
		MAP,
		ZERO
	};

	string m_strOp[5];

	long m_fOptions;

	void PrintOP(long nOP, int nPage, int nFrame);
	void PrintOP(long nOP, int nFrame);

	void PrintPTE();
	void PrintFrames();

private:
	long m_lUnmaps;
	long m_lMaps;
	long m_lIns;
	long m_lOuts;
	long m_lZeros;
};
MyMMU::MyMMU()
{

}
MyMMU::MyMMU(const char *infile, string strOptions, int nFrames):MySystem(infile)
{
	m_vPTE.resize(PTE_SIZE,0);
	m_vFrames.resize(nFrames, -1);

	m_strOp[UNMAP] = "UNMAP";
	m_strOp[OUT] = "OUT";
	m_strOp[IN] = "IN";
	m_strOp[MAP] = "MAP";
	m_strOp[ZERO] = "ZERO";

	m_fOptions = 0;

	std::size_t found = strOptions.find("O");
	if (found!=std::string::npos)
		B_SET(m_fOptions, opO);
	found = strOptions.find("P");
	if (found!=std::string::npos)
		B_SET(m_fOptions, opP);
	found = strOptions.find("F");
	if (found!=std::string::npos)
		B_SET(m_fOptions, opF);
	found = strOptions.find("S");
	if (found!=std::string::npos)
		B_SET(m_fOptions, opS);
	found = strOptions.find("p");
	if (found!=std::string::npos)
		B_SET(m_fOptions, opp);
	found = strOptions.find("f");
	if (found!=std::string::npos)
		B_SET(m_fOptions, opf);

	m_lUnmaps = 0;
	m_lMaps = 0;
	m_lIns = 0;
	m_lOuts = 0;
	m_lZeros = 0;

}
MyMMU::~MyMMU()
{
	delete m_pReplacement;
}

void MyMMU::Print()
{
	if (B_IS_SET(m_fOptions, opP))
		PrintPTE();
	if (B_IS_SET(m_fOptions, opF))
		PrintFrames();

	if (B_IS_SET(m_fOptions, opS))
	{
		long lCount = get_Count() + 1;
		unsigned long long totalcost = 0;

		totalcost += m_lMaps*400;
		totalcost += m_lUnmaps*400;
		totalcost += m_lIns*3000;
		totalcost += m_lOuts*3000;
		totalcost += m_lZeros*150;
		totalcost += lCount;

		printf("SUM %d U=%d M=%d I=%d O=%d Z=%d ===> %llu\n", 
			lCount, m_lUnmaps, m_lMaps, m_lIns, m_lOuts, m_lZeros, totalcost);

	}
}

void MyMMU::PrintPTE()
{
	int nSize = m_vPTE.size();
	long lpte = 0;
	char r,m,s;

	for (int i=0; i<nSize;i++)
	{
		lpte = m_vPTE[i];
		if (B_IS_SET(lpte,PRESENT))
		{
			r = B_IS_SET(lpte,REFERENCED)? 'R':'-';
			m = B_IS_SET(lpte,MODIFIED)? 'M':'-';
			s = B_IS_SET(lpte,PAGEDOUT)? 'S':'-';
			printf("%d:%c%c%c ",i,r,m,s);
		}
		else if (B_IS_SET(lpte,PAGEDOUT))
			printf("# ");
		else
			printf("* ");
	}
	printf("\n");
}

void MyMMU::PrintFrames()
{
	int nSize = m_vFrames.size();
	for (int i=0; i<nSize;i++)
	{
		m_vFrames[i]==-1?printf("%c ", '*'):printf("%d ", m_vFrames[i]);
	}
	printf("\n");
}

void MyMMU::PrintOP(long nOP, int nPage, int nFrame)
{
	if (B_IS_SET(m_fOptions, opO))
		printf("%d: %s%*d%4d\n", get_Count(), m_strOp[nOP].c_str(), 9-m_strOp[nOP].size(),nPage, nFrame);
}
void MyMMU::PrintOP(long nOP, int nFrame)
{
	if (B_IS_SET(m_fOptions, opO))
		printf("%d: %s%9d\n", get_Count(), m_strOp[nOP].c_str(), nFrame);
}

void MyMMU::Run(MyReplacement* p)
{
	m_pReplacement = p;
	MySystem::Run();
}

void MyMMU::ExecInstruction(int rw, int inst)
{
	// check index i.e. inst out of bound
	if (B_IS_SET(m_fOptions, opO))
		printf("==> inst: %d %d\n", rw, inst);
	
	if (!B_IS_SET(m_vPTE[inst], PRESENT))
	{
		int nFrame = m_pReplacement->GetFrame(m_vPTE, m_vFrames);

		if (m_vFrames[nFrame] != -1)
		{
			// we need to UNMAP the old one
			PrintOP(UNMAP, m_vFrames[nFrame], nFrame);
			B_UNSET(m_vPTE[m_vFrames[nFrame]], PRESENT);
			m_lUnmaps++;

			if (B_IS_SET(m_vPTE[m_vFrames[nFrame]], MODIFIED))
			{
				// need to PAGE - OUT
				PrintOP(OUT, m_vFrames[nFrame], nFrame);
				B_SET(m_vPTE[m_vFrames[nFrame]], PAGEDOUT);
				m_lOuts++;
				B_UNSET(m_vPTE[m_vFrames[nFrame]], MODIFIED);
			}
		}

		// now map the new one
		if (B_IS_SET(m_vPTE[inst], PAGEDOUT))
		{
			// bring it IN
			PrintOP(IN, inst, nFrame);
			m_lIns++;
		}
		else
		{
			// ZERO out
			PrintOP(ZERO, nFrame);
			m_lZeros++;
		}

		// start MAP
		PrintOP(MAP, inst, nFrame);
		m_lMaps++;
		B_SET_IDX(m_vPTE[inst], nFrame);
		m_vFrames[nFrame] = inst;
		B_SET(m_vPTE[inst], PRESENT);

		//end MAP
	}

	// set REFERENCED
	B_SET(m_vPTE[inst], REFERENCED);
	// set MODIFIED
	if (rw)
		B_SET(m_vPTE[inst], MODIFIED);

	m_pReplacement->SetUsed(inst, B_GET_IDX(m_vPTE[inst]));

	//PrintPTE();
	//PrintFrames();
}


class Random_algo: public MyReplacement
{
public:
	Random_algo();
	Random_algo(const char *randfile, int frames);
	~Random_algo();

	virtual int GetFrame(vector<long> &vPTE, vector<int> &vFrames);
private:
	long m_lRandMax;
	long *m_plRandNums;
	long m_lOFS;
	int myrandom();

	int m_nFrames;
};

Random_algo::Random_algo()
{
}

Random_algo::Random_algo(const char *randfile, int frames)
{
	m_nFrames = frames;

	m_lOFS = -1;
	m_lRandMax = 0;
	m_plRandNums = 0;

	ifstream fileRand (randfile);
	
	if (!fileRand.is_open())
		throw;

	long idx = 0;
	if (!fileRand.eof())
		fileRand >> m_lRandMax;

	m_plRandNums = new long[m_lRandMax];

	while (!fileRand.eof() && idx < m_lRandMax)
	{
		fileRand >> m_plRandNums[idx++];
	}
}

int Random_algo::myrandom()
{
	if (++m_lOFS >= m_lRandMax)
		m_lOFS = 0;
	int myrnd = m_plRandNums[m_lOFS] % m_nFrames;
	return myrnd;
}

Random_algo::~Random_algo()
{
}

int Random_algo::GetFrame(vector<long> &vPTE, vector<int> &vFrames)
{
	if (m_bFreeFrames)
	{
		int nSize = vFrames.size();

		// check for free frame
		for (int i=0; i < nSize; i++)
		{
			if (vFrames[i] == -1)
				return i;
		}

		m_bFreeFrames = false;
	}
	return myrandom();
}


class FIFO_algo: public MyReplacement
{
public:
	FIFO_algo();
	FIFO_algo(int frames);
	~FIFO_algo();

	virtual int GetFrame(vector<long> &vPTE, vector<int> &vFrames);
private:
	int m_nCounter;
	int m_nFrames;
};

FIFO_algo::FIFO_algo()
{
}

FIFO_algo::FIFO_algo(int frames)
{
	m_nFrames = frames;
	m_nCounter = -1;
}

FIFO_algo::~FIFO_algo()
{
}

int FIFO_algo::GetFrame(vector<long> &vPTE, vector<int> &vFrames)
{
	m_nCounter = ++m_nCounter % m_nFrames;

	return m_nCounter;
}

class SecondChance_algo: public MyReplacement
{
public:
	SecondChance_algo();
	SecondChance_algo(int frames);
	~SecondChance_algo();

	virtual int GetFrame(vector<long> &vPTE, vector<int> &vFrames);
private:
	int m_nCounter;
	int m_nFrames;
};

SecondChance_algo::SecondChance_algo()
{
}

SecondChance_algo::SecondChance_algo(int frames)
{
	m_nFrames = frames;
	m_nCounter = -1;
}

SecondChance_algo::~SecondChance_algo()
{
}

int SecondChance_algo::GetFrame(vector<long> &vPTE, vector<int> &vFrames)
{
	m_nCounter = ++m_nCounter % m_nFrames;

	while (vFrames[m_nCounter] != -1 && B_IS_SET(vPTE[vFrames[m_nCounter]], REFERENCED))
	{
		B_UNSET(vPTE[vFrames[m_nCounter]], REFERENCED);
		m_nCounter = ++m_nCounter % m_nFrames;
	}

	return m_nCounter;
}

class LRU_algo: public MyReplacement
{
public:
	LRU_algo(int frames);

	virtual int GetFrame(vector<long> &vPTE, vector<int> &vFrames);
	virtual void SetUsed(int inst, int frame);
private:
	int m_nFrames;
	list<int> m_lstFrames;
	list<int>::iterator m_itrFrames;
	int m_nLastPoped;
};

LRU_algo::LRU_algo(int frames)
{
	m_nFrames = frames;
	m_nLastPoped = -1;
}
void LRU_algo::SetUsed(int inst, int frame)
{
	//if (!(m_nLastPoped == -1 || m_nLastPoped == frame))
	//{
		for (m_itrFrames = m_lstFrames.begin(); m_itrFrames != m_lstFrames.end(); ++m_itrFrames)
		{
			if (*m_itrFrames == frame)
			{
				m_lstFrames.erase(m_itrFrames);
				break;
			}
		}
	//}
	m_lstFrames.push_front(frame);
}

int LRU_algo::GetFrame(vector<long> &vPTE, vector<int> &vFrames)
{
	if (m_bFreeFrames)
	{
		int nSize = vFrames.size();

		// check for free frame
		for (int i=0; i < nSize; i++)
		{
			if (vFrames[i] == -1)
			{
				return i;
			}
		}

		m_bFreeFrames = false;
	}

	m_nLastPoped = m_lstFrames.back();
	m_lstFrames.pop_back();

	return m_nLastPoped;
}

class NRU_algo: public MyReplacement
{
public:
	NRU_algo(const char *randfile, int frames);

	virtual int GetFrame(vector<long> &vPTE, vector<int> &vFrames);
private:
	long m_lRandMax;
	long *m_plRandNums;
	long m_lOFS;
	int myrandom(int num);

	int m_nCounter;

	int m_nFrames;

	int classarray[4][PTE_SIZE];
	int num_pages_in_array[4];
};

NRU_algo::NRU_algo(const char *randfile, int frames)
{
	m_nFrames = frames;
	m_nCounter = 0;

	m_lOFS = -1;
	m_lRandMax = 0;
	m_plRandNums = 0;

	ifstream fileRand (randfile);
	
	if (!fileRand.is_open())
		throw;

	long idx = 0;
	if (!fileRand.eof())
		fileRand >> m_lRandMax;

	m_plRandNums = new long[m_lRandMax];

	while (!fileRand.eof() && idx < m_lRandMax)
	{
		fileRand >> m_plRandNums[idx++];
	}
}

int NRU_algo::myrandom(int num)
{
	if (++m_lOFS >= m_lRandMax)
		m_lOFS = 0;
	int myrnd = m_plRandNums[m_lOFS] % num;
	return myrnd;
}

int NRU_algo::GetFrame(vector<long> &vPTE, vector<int> &vFrames)
{
	// reset REF on th10 call

	if (m_bFreeFrames)
	{
		int nSize = vFrames.size();

		// check for free frame
		for (int i=0; i < nSize; i++)
		{
			if (vFrames[i] == -1)
				return i;
		}

		m_bFreeFrames = false;
	}
	m_nCounter++;
	int nSize = vPTE.size();
	bool bReset = m_nCounter == 10;
	if (bReset)
	{
		//printf("@@ reset NRU refbits while walking PTE\n");
		m_nCounter = 0;
	}

	//classarray

	num_pages_in_array[0] = 0;
	num_pages_in_array[1] = 0;
	num_pages_in_array[2] = 0;
	num_pages_in_array[3] = 0;

	for (int i=0; i<nSize;i++)
	{
		long lVal = vPTE[i];
		if (B_IS_SET(lVal, PRESENT))
		{
			if (!B_IS_SET(lVal, REFERENCED) && !B_IS_SET(lVal, MODIFIED))
			{
				// class 0
				classarray[0][num_pages_in_array[0]] = i;
				num_pages_in_array[0] = num_pages_in_array[0] + 1;
			}
			else if (!B_IS_SET(lVal, REFERENCED) && B_IS_SET(lVal, MODIFIED))
			{
				// class 1
				classarray[1][num_pages_in_array[1]] = i;
				num_pages_in_array[1] = num_pages_in_array[1] + 1;
			}
			else if (B_IS_SET(lVal, REFERENCED) && !B_IS_SET(lVal, MODIFIED))
			{
				// class 2
				classarray[2][num_pages_in_array[2]] = i;
				num_pages_in_array[2] = num_pages_in_array[2] + 1;
			}
			else if (B_IS_SET(lVal, REFERENCED) && B_IS_SET(lVal, MODIFIED))
			{
				// class 3
				classarray[3][num_pages_in_array[3]] = i;
				num_pages_in_array[3] = num_pages_in_array[3] + 1;
			}
		}
		if (bReset)
		{
			if (B_IS_SET(lVal, REFERENCED))
				B_UNSET(vPTE[i], REFERENCED);
		}
	}

	int pte = 0;
	int lowest_class = 0;
	int idx = 0;
	for (int i = 0; i < 4; i++)
	{
		if (num_pages_in_array[i] > 0)
		{
			idx = myrandom(num_pages_in_array[i]);
			pte = classarray[i][idx];
			lowest_class = i;
			break;
		}
	}

	/*printf("@@ lowest_class=%d: selidx=%d from",lowest_class,idx);
	for (int i = 0; i< num_pages_in_array[lowest_class]; i++)
		printf(" %d",classarray[lowest_class][i]);
	printf("\n");*/

	return B_GET_IDX(vPTE[pte]);
}

class Clock_s_algo: public MyReplacement
{
public:
	Clock_s_algo(int frames);

	virtual int GetFrame(vector<long> &vPTE, vector<int> &vFrames);
private:
	int m_nHand;
	int m_nFrames;
};

Clock_s_algo::Clock_s_algo(int frames)
{
	m_nFrames = frames;
	m_nHand = -1;
}

int Clock_s_algo::GetFrame(vector<long> &vPTE, vector<int> &vFrames)
{
	if (m_bFreeFrames)
	{
		int nSize = vFrames.size();

		// check for free frame
		for (int i=0; i < nSize; i++)
		{
			if (vFrames[i] == -1)
			{
				return i;
			}
		}

		m_bFreeFrames = false;
	}

	m_nHand = ++m_nHand % m_nFrames;
	while (B_IS_SET(vPTE[vFrames[m_nHand]], REFERENCED))
	{
		B_UNSET(vPTE[vFrames[m_nHand]], REFERENCED);
		m_nHand = ++m_nHand % m_nFrames;
	}

	return m_nHand;
}

class Clock_b_algo: public MyReplacement
{
public:
	Clock_b_algo(int frames);

	virtual int GetFrame(vector<long> &vPTE, vector<int> &vFrames);

private:
	int m_nHand;
	int m_nFrames;
};

Clock_b_algo::Clock_b_algo(int frames)
{
	m_nFrames = frames;
	m_nHand = -1;
}

int Clock_b_algo::GetFrame(vector<long> &vPTE, vector<int> &vFrames)
{
	if (m_bFreeFrames)
	{
		int nSize = vFrames.size();

		// check for free frame
		for (int i=0; i < nSize; i++)
		{
			if (vFrames[i] == -1)
			{
				return i;
			}
		}

		m_bFreeFrames = false;
	}

	m_nHand = ++m_nHand % PTE_SIZE;

	while (!B_IS_SET(vPTE[m_nHand], PRESENT) || B_IS_SET(vPTE[m_nHand], REFERENCED))
	{
		if (B_IS_SET(vPTE[m_nHand], REFERENCED))
			B_UNSET(vPTE[m_nHand], REFERENCED);
		m_nHand = ++m_nHand % PTE_SIZE;
	}

	return B_GET_IDX(vPTE[m_nHand]);
}

class Aging_b_algo: public MyReplacement
{
public:
	Aging_b_algo(int frames);

	virtual int GetFrame(vector<long> &vPTE, vector<int> &vFrames);

private:
	int m_nFrames;
	vector<unsigned long> m_Age;
	unsigned long m_lOne;
	unsigned long m_lTemp;
};

Aging_b_algo::Aging_b_algo(int frames)
{
	m_nFrames = frames;
	m_Age.resize(PTE_SIZE,0);
	m_lOne = 2147483648;
}


int Aging_b_algo::GetFrame(vector<long> &vPTE, vector<int> &vFrames)
{
	if (m_bFreeFrames)
	{
		int nSize = vFrames.size();

		// check for free frame
		for (int i=0; i < nSize; i++)
		{
			if (vFrames[i] == -1)
			{
				return i;
			}
		}

		m_bFreeFrames = false;
	}
	// aging 
	int nSize = m_Age.size();

	/* comment this code and move to one loop (here for debugging)
	for (int i=0; i<nSize; i++)
	{
		if (B_IS_SET(vPTE[i], REFERENCED))
			m_Age[i] = (m_Age[i]>>1) | m_lOne;
		else
			m_Age[i] = m_Age[i]>>1;
	}*/

	// now loop over all the PRESENT pages and pick the samallest one
	int nIdx = -1;
	for (int i = 0; i<nSize;i++)
	{
		// moved from top and bottom
		if (B_IS_SET(vPTE[i], REFERENCED))
		{
			B_UNSET(vPTE[i], REFERENCED);
			m_Age[i] = (m_Age[i]>>1) | m_lOne;
		}
		else
			m_Age[i] = m_Age[i]>>1;
		// moved from top and bottom

		if (B_IS_SET(vPTE[i], PRESENT))
		{
			if (nIdx == -1)
				nIdx = i;
			else
			{
				if (m_Age[i] < m_Age[nIdx])
					nIdx = i;
			}

		}
	}

	/* comment this code and move to one loop (here for debugging)
	for (int i=0; i<nSize; i++)
	{
		if (B_IS_SET(vPTE[i], REFERENCED))
		{
			B_UNSET(vPTE[i], REFERENCED);
		}
	}*/

	m_Age[nIdx] = 0;

	return B_GET_IDX(vPTE[nIdx]);
}


// Phys aging
class Aging_s_algo: public MyReplacement
{
public:
	Aging_s_algo(int frames);

	virtual int GetFrame(vector<long> &vPTE, vector<int> &vFrames);

private:
	int m_nFrames;
	vector<unsigned long> m_Age;
	unsigned long m_lOne;
	unsigned long m_lTemp;
};

Aging_s_algo::Aging_s_algo(int frames)
{
	m_nFrames = frames;
	m_Age.resize(PTE_SIZE,0);
	m_lOne = 2147483648;
}


int Aging_s_algo::GetFrame(vector<long> &vPTE, vector<int> &vFrames)
{
	if (m_bFreeFrames)
	{
		int nSize = vFrames.size();

		// check for free frame
		for (int i=0; i < nSize; i++)
		{
			if (vFrames[i] == -1)
			{
				return i;
			}
		}

		m_bFreeFrames = false;
	}
	// aging 
	int nSize = m_Age.size();

	for (int i=0; i<nSize; i++)
	{
		if (B_IS_SET(vPTE[i], REFERENCED))
		{
			B_UNSET(vPTE[i], REFERENCED); // moved from below
			m_Age[i] = (m_Age[i]>>1) | m_lOne;
		}
		else
			m_Age[i] = m_Age[i]>>1;
	}

	// now loop over all the PRESENT pages and pick the samallest one
	int nFrameSize = vFrames.size();
	int nFrameIdx = 0;
	for (int i = 0; i<nFrameSize;i++)
	{
		if (m_Age[vFrames[i]] < m_Age[vFrames[nFrameIdx]])
			nFrameIdx = i;
	}

	/* comment this code and move to one loop (here for debugging)
	for (int i=0; i<nSize; i++)
	{
		if (B_IS_SET(vPTE[i], REFERENCED))
		{
			B_UNSET(vPTE[i], REFERENCED);
		}
	}*/

	m_Age[vFrames[nFrameIdx]] = 0;

	return nFrameIdx;
}




//./mmu [-a<algo>] [-o<options>] [–f<num_frames>] inputfile randomfile
int main(int argc, char* argv[])
{
	string strOptions = "OPFS";
	int nFrames = 32;
	char cAlgo = 'l';
	int nIn = 0;
	int nRand = 0;

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
			if (strInput[1] == 'a')
				cAlgo = strInput[2];
			else if (strInput[1] == 'o')
				strOptions = strInput.substr(2);
			else if (strInput[1] == 'f')
				nFrames = stoi(strInput.substr(2));
		}
		else
		{
			if (!nIn)
				nIn = i;
			else
				nRand = i;
		}
	}

	MyReplacement *pRepl = 0;

	switch (cAlgo)
	{
		case 'N':
			pRepl = new NRU_algo(argv[nRand], nFrames);
			break;
		case 'l':
			pRepl = new LRU_algo(nFrames);
			break;
		case 'r':
			pRepl = new Random_algo(argv[nRand], nFrames);
			break;
		case 'f':
			pRepl = new FIFO_algo(nFrames);
			break;
		case 's':
			pRepl = new SecondChance_algo(nFrames);
			break;
		case 'c':
			pRepl = new Clock_s_algo(nFrames);
			break;
		case 'C':
			pRepl = new Clock_b_algo(nFrames);
			break;
		case 'a':
			pRepl = new Aging_s_algo(nFrames);
			break;
		case 'A':
			pRepl = new Aging_b_algo(nFrames);
			break;
		default:
			cout << "No replacement algorithms provided" << endl;
			return 0;
	}

	MyMMU myMMU (argv[nIn], strOptions, nFrames);

	myMMU.Run(pRepl);

	myMMU.Print();

	//system("pause");
	return 0;
}


