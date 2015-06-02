/*
*			Christopher Pac
*			9/23/2013
*			Operating Systems Lab 1
*			Linker
*
*
*
*			Pass One and Pass Two are done in main
*/


// MyLinker.cpp : Defines the entry point for the console application.
//

//#include "stdafx.h"
#include <stdio.h>
//#include <ctype.h>
#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <cctype>
#include <algorithm>
#include <vector>
#include <iomanip>
using namespace std;

const int SYMBOLSIZE = 16;
const int MACHINESIZE = 512;
const int MAXDEFLIST = 16;
const int MAXUSELIST = 16;
const int EMPTYIND = -30000;

const char* IMMEDIATE = "I";
const char* RELATIVE = "R";
const char* ABSOLUTE = "A";
const char* EXTERNAL = "E";

class SymbolInfo;
typedef vector<SymbolInfo*> SymbolTable;
typedef std::vector<SymbolInfo*>::iterator SymbolTableItr;

class SymbolInfo
{
public:
	SymbolInfo();
	SymbolInfo(const string& name, int addr, int num, int err);
	SymbolInfo(SymbolInfo *p);
	~SymbolInfo();

	string SymName;
	int SymAddress;
	int ModNum;
	long SymErr;
};

SymbolInfo::SymbolInfo()
{
	SymName = "";
	SymAddress = 0;
	ModNum = 0;
	SymErr = 0;
}
	
SymbolInfo::SymbolInfo(const string& name, int addr, int num, int err)
{
	SymName = name;
	SymAddress = addr;
	ModNum = num;
	SymErr = err;
}

SymbolInfo::SymbolInfo(SymbolInfo *p)
{
	SymName = p->SymName;
	SymAddress = p->SymAddress;
	ModNum = p->ModNum;
	SymErr = p->SymErr;
}

SymbolInfo::~SymbolInfo()
{
}

class ModInfo
{
public:
	ModInfo();
	~ModInfo();

	int Size;
	int BaseAddr;
};

ModInfo::ModInfo()
{
	Size = 0;
	BaseAddr = 0;
}

ModInfo::~ModInfo()
{
}

class TokensInModules
{
public:
	TokensInModules();
	TokensInModules(const char *filename);

	~TokensInModules();
	
	virtual void Parse() = 0;
	virtual void Print() = 0;

protected:
	ifstream m_fileModules;
	string m_token;
	int m_nLineNum;

	bool GetNextToken();

	int GetLineNum();
	int GetLineOffset();
private:
	stringstream m_sstr;

};

TokensInModules::TokensInModules()
{

}

TokensInModules::TokensInModules(const char *filename)
{
	m_token = "";
	m_nLineNum = 0;
	m_sstr.str( std::string() );
	m_sstr.clear();
	m_sstr.setstate(stringstream::eofbit);

	m_fileModules.open(filename);

	if (!m_fileModules.is_open())
		throw;
}

TokensInModules::~TokensInModules()
{

}

int TokensInModules::GetLineNum()
{
	return m_nLineNum;
}

int TokensInModules::GetLineOffset()
{
	int lineoffset = (int)m_sstr.tellg();
	if (lineoffset == -1)
		lineoffset = (int)m_sstr.str().size();
	return lineoffset;
}

bool TokensInModules::GetNextToken()
{
	m_token = "";

	if (m_sstr.eof())
	{
		if (!m_fileModules.good())
			return false;

		string line = "";
		while (line.empty() && std::getline(m_fileModules, line))
		{
			m_nLineNum++;

			m_sstr.clear();
			m_sstr.str( line );
		}

		if (line.empty())
			return false;
		else
		{
			return GetNextToken();
		}
	}
	else if (!(m_sstr >> m_token))
	{
		if (m_sstr.eof())
			return GetNextToken();
		else
			return false;
		//m_sstr.clear();
		//m_sstr.str( std::string() );
		//return GetNextToken();
	}

	return true;
}

class ParseTokens : public TokensInModules
{
public:
	ParseTokens();
	ParseTokens(const char *filename);
	~ParseTokens();

	virtual void Parse();

protected:

	// definition list parse
	virtual int HandleDefCount();
	virtual string HandleDefSymbol();
	virtual int HandleDefSymbolAddr();
	virtual void HandleDefinition(const string& symname, int symaddr, int mnum) = 0;

	// use list parse
	virtual int HandleUseCount(int mnum = 0);
	virtual string HandleUseSymbol(int mnum = 0);

	// program text
	virtual int HandleCodeCount(int mnum = 0);
	virtual string HandleCodeType();
	virtual int HandleCodeInstr();
	virtual void HandleCode(const string& codetype, int codeinstr, int mnum) = 0;

};

ParseTokens::ParseTokens()
{
}

ParseTokens::ParseTokens(const char *filename):TokensInModules(filename)
{
}

ParseTokens::~ParseTokens()
{
}

void ParseTokens::Parse()
{
	string line;
	int defcount = 0;
	int usecount = 0;
	int codecount = 0;
	string name = "";
	int addr = 0;
	int modulenum = -1;
	while (GetNextToken())
	{
		modulenum++;

		// **** definition list parse ****
		// first taken should be the defcount
		defcount = HandleDefCount();

		for (int i = 0; i < defcount; i++)
		{
			// Symbol (S)
			GetNextToken();
			name = HandleDefSymbol();
			// Address of Symbol R
			GetNextToken();
			addr = HandleDefSymbolAddr();

			HandleDefinition(name, addr, modulenum);
		}

		// **** use list parse ****
		//first should be the use count
		GetNextToken();
		usecount = HandleUseCount(modulenum);

		for (int i = 0; i < usecount; i++)
		{
			// Use Symbols
			GetNextToken();
			HandleUseSymbol(modulenum);
		}

		// **** program text ****
		GetNextToken();
		codecount = HandleCodeCount(modulenum);

		for (int i = 0; i < codecount; i++)
		{
			// Addressing Type
			GetNextToken();
			name = HandleCodeType();
			// Instr
			GetNextToken();
			addr = HandleCodeInstr();

			HandleCode(name, addr, modulenum);
		}
	}
}

int ParseTokens::HandleDefCount()
{
	return stoi(m_token);
}

string ParseTokens::HandleDefSymbol()
{
	return m_token;
}

string ParseTokens::HandleUseSymbol(int mnum)
{
	return m_token;
}

int ParseTokens::HandleDefSymbolAddr()
{
	return stoi(m_token);
}

string ParseTokens::HandleCodeType()
{
	return m_token;
}

int ParseTokens::HandleCodeInstr()
{
	return stoi(m_token);
}

int ParseTokens::HandleUseCount(int mnum)
{
	return stoi(m_token);
}

int ParseTokens::HandleCodeCount(int mnum)
{
	return stoi(m_token);
}

class ErrorsAndWarnings
{
public:
	ErrorsAndWarnings();
	~ErrorsAndWarnings();

protected:
	enum ew_flags {
	  E2SymMultipleDef			= 0x001,	// rule 2
	  E3SymNotDef				= 0x002,	// rule 3
	  W4SymDefNotUsed			= 0x004,	// rule 4
	  W5SymTooBig				= 0x008,	// rule 5
	  E6ExternalEx				= 0x010,	// rule 6
	  W7SymUseNotUsed			= 0x020,	// rule 7
	  E8AbsoluteEx				= 0x040,	// rule 8
	  E9RelativeEx				= 0x080,	// rule 9
	  E10Immediate				= 0x100,	// rule 10
	  E11Opcode					= 0x200		// rule 11
	};

protected:
	long AddEW(long err, long flag);

	void PrintErrorsAndWarnings(long flag, SymbolInfo *p, int extr = 0);
	void PrintErrorsAndWarnings(long flag, long symerr, string symname);
	void PrintErrorsAndWarnings(long flag, int mod, SymbolInfo *p);

private:
	SymbolInfo *m_tempSymInfo;
};

ErrorsAndWarnings::ErrorsAndWarnings()
{
	m_tempSymInfo =  new SymbolInfo();
}

ErrorsAndWarnings::~ErrorsAndWarnings()
{
	delete m_tempSymInfo;
}

long ErrorsAndWarnings::AddEW(long err, long flag)
{
	err = err | flag;
	return err;

}

void ErrorsAndWarnings::PrintErrorsAndWarnings(long flag, long symerr, string symname)
{
	m_tempSymInfo->SymName = symname;
	m_tempSymInfo->SymErr = symerr;
		
	PrintErrorsAndWarnings(flag, m_tempSymInfo);
}

void ErrorsAndWarnings::PrintErrorsAndWarnings(long flag, int mod, SymbolInfo *p)
{
	if (p)
	{
		m_tempSymInfo->SymName = p->SymName;
		m_tempSymInfo->SymErr = p->SymErr;
		m_tempSymInfo->ModNum = mod;
	
		PrintErrorsAndWarnings(flag, m_tempSymInfo);
	}
}

void ErrorsAndWarnings::PrintErrorsAndWarnings(long flag, SymbolInfo *p, int extra)
{
	if (p == 0)
		return;

	long resultflag = flag & p->SymErr;


	if (resultflag & E2SymMultipleDef)
	{
		printf(" Error: This variable is multiple times defined; first value used");
	}
	else if (resultflag & W5SymTooBig)
	{
		printf("Warning: Module %d: %s to big %d (max=%d) assume zero relative\n", (p->ModNum)+1, p->SymName.c_str(), p->SymAddress, extra);

	}
	else if(resultflag & E8AbsoluteEx)
	{
		printf(" Error: Absolute address exceeds machine size; zero used");
	}
	else if(resultflag & E9RelativeEx)
	{
		printf(" Error: Relative address exceeds module size; zero used");
	}
	else if(resultflag & E6ExternalEx)
	{
		printf(" Error: External address exceeds length of uselist; treated as immediate");
	}
	else if(resultflag & E3SymNotDef)
	{
		printf(" Error: %s is not defined; zero used", p->SymName.c_str());
	}
	else if(resultflag & E10Immediate)
	{
		printf(" Error: Illegal immediate value; treated as 9999");
	}
	else if(resultflag & E11Opcode)
	{
		printf(" Error: Illegal opcode; treated as 9999");
	}
	else if(resultflag & W7SymUseNotUsed)
	{
		printf("Warning: In Module %d %s appeared in the uselist but was not actually used\n", (p->ModNum)+1, p->SymName.c_str());
	}
	else if(resultflag & W4SymDefNotUsed)
	{
		printf("Warning: %s was defined in module %d but never used\n", p->SymName.c_str(), (p->ModNum)+1);
	}

}

class PassOne : public ParseTokens, public ErrorsAndWarnings
{
public:
	PassOne();
	PassOne(const char *filename);
	~PassOne();

	virtual void Print();
	//virtual void Parse(){;};
protected:
	// definition list parse
	virtual int HandleDefCount();
	virtual string HandleDefSymbol();
	virtual int HandleDefSymbolAddr();
	virtual void HandleDefinition(const string& symname, int symaddr, int mnum);
	virtual void HandleCode(const string& codetype, int codeinstr, int mnum){;};

	// use list parse
	virtual int HandleUseCount(int mnum =0);
	virtual string HandleUseSymbol(int mnum);

	// program text
	virtual int HandleCodeCount(int mnum = 0);
	virtual string HandleCodeType();
	virtual int HandleCodeInstr();

	// parse checks
	bool CheckSymbol(long errcode=0);
protected:

	int GetNonAlphaNum(const std::string &token);
	int GetNonNum(const std::string &token);

protected:
	enum errorcode
	{
		NUM_EXPECTED = 0,
		SYM_EXPECTED,
		ADDR_EXPECTED,
		SYM_TOLONG,
		TO_MANY_DEF_IN_MODULE,
		TO_MANY_USE_IN_MODULE,
		TO_MANY_SYMBOLS,
		TO_MANY_INSTR,
		CHAR_EXPECTED
	};

	void ParseError(int errcode, int erroffset);

protected:
	SymbolTable m_symtable;

	bool AddSymInfo(SymbolInfo* p);

	typedef vector<ModInfo*> ModuleInfo;
	typedef std::vector<ModInfo*>::iterator ModuleInfoItr;

	ModuleInfo m_modaddr;

	bool AddModInfo(ModInfo* p);

	void ResolveInfo();

public:
	SymbolInfo* GetSymbolInfo(const string& name);
	int GetModuleBaseAddr(int modnum);
	int GetModuleSize(int modnum);

	SymbolInfo* GetSymbolInfobyModandErr(int mod, long err);
	SymbolInfo* GetNextSymbolInfobyModandErr(int mod, long err);

private:
	SymbolTableItr m_SymbolInfobyModandErrItr;
	int m_TotalProgSize;
};

PassOne::PassOne()
{
	m_TotalProgSize = 0;
}

PassOne::PassOne(const char *filename):ParseTokens(filename)
{
	m_TotalProgSize = 0;
}

PassOne::~PassOne()
{
	for (SymbolTableItr it = m_symtable.begin(); it != m_symtable.end(); ++it)
	{
		delete *it;
	}

	for (ModuleInfoItr it = m_modaddr.begin(); it != m_modaddr.end(); ++it)
	{
		delete *it;
	}

}

SymbolInfo* PassOne::GetSymbolInfo(const string& name)
{
	// check if the symbol is not in the table first
	for (SymbolTableItr it = m_symtable.begin(); it != m_symtable.end(); ++it)
	{
		if (name.compare((*it)->SymName) == 0)
		{
			return *it;
		}
	}

	return 0;
}

SymbolInfo* PassOne::GetSymbolInfobyModandErr(int mod, long err)
{
	m_SymbolInfobyModandErrItr = m_symtable.begin();

	while (m_SymbolInfobyModandErrItr != m_symtable.end())
	{
		if ((*m_SymbolInfobyModandErrItr)->ModNum == mod &&  ((*m_SymbolInfobyModandErrItr)->SymErr & err))
		{
			return *m_SymbolInfobyModandErrItr;
		}

		++m_SymbolInfobyModandErrItr;
	}

	return 0;
}

SymbolInfo* PassOne::GetNextSymbolInfobyModandErr(int mod, long err)
{
	++m_SymbolInfobyModandErrItr;
	while (m_SymbolInfobyModandErrItr != m_symtable.end())
	{
		if ((*m_SymbolInfobyModandErrItr)->ModNum == mod &&  ((*m_SymbolInfobyModandErrItr)->SymErr & err))
		{
			return *m_SymbolInfobyModandErrItr;
		}
		
		++m_SymbolInfobyModandErrItr;
	}

	return 0;
}

int PassOne::GetModuleBaseAddr(int modnum)
{
	// *IMPROVE* check if module exits for that num
	return m_modaddr[modnum]->BaseAddr;
}

int PassOne::GetModuleSize(int modnum)
{
	// *IMPROVE* check if module exits for that num
	return m_modaddr[modnum]->Size;
}

bool PassOne::AddSymInfo(SymbolInfo* p)
{
	if (p != 0)
	{
		// check if the symbol is not in the table first
		for (SymbolTableItr it = m_symtable.begin(); it != m_symtable.end(); ++it)
		{
			if (p->SymName.compare((*it)->SymName) == 0)
			{
				(*it)->SymErr = (*it)->SymErr | E2SymMultipleDef;
				return false;
			}
		}

		p->SymErr = p->SymErr | W4SymDefNotUsed; // assume it will not be used
		m_symtable.push_back(p);
	}
	else
		return false;

	return true;
}

bool PassOne::AddModInfo(ModInfo* p)
{
	if (p != 0)
	{
		m_modaddr.push_back(p);
	}
	else
		return false;

	return true;
}

void PassOne::ResolveInfo()
{
	int baseaddr = 0;
	// reslove the base address for each module
	for (ModuleInfoItr it = m_modaddr.begin(); it != m_modaddr.end(); ++it)
	{
		(*it)->BaseAddr = baseaddr;
		baseaddr = baseaddr + (*it)->Size;
	}

	for (SymbolTableItr it = m_symtable.begin(); it != m_symtable.end(); ++it)
	{
		// check if an address appearing in a definition exceeds the size of the module and cheat by printing msg here
		int modsize = m_modaddr[(*it)->ModNum]->Size;
		if ((*it)->SymAddress != 0 && (*it)->SymAddress >= modsize)
		{
			(*it)->SymErr = (*it)->SymErr | W5SymTooBig;
			if (modsize > 0)
				modsize--;
			PrintErrorsAndWarnings(W5SymTooBig, *it, modsize);
			(*it)->SymAddress = 0;
		}
		(*it)->SymAddress = m_modaddr[(*it)->ModNum]->BaseAddr + (*it)->SymAddress;
	}
}

void PassOne::ParseError(int errcode, int erroffset)
{
	string errstr[] = {
		"NUM_EXPECTED",			// Number expect
		"SYM_EXPECTED",			// Symbol Expected
		"ADDR_EXPECTED",		// Addressing Expected
		"SYM_TOLONG",				// Symbol Name is to long
		"TO_MANY_DEF_IN_MODULE", // ..
		"TO_MANY_USE_IN_MODULE",
		"TO_MANY_SYMBOLS",
		"TO_MANY_INSTR",
		"CHAR_EXPECTED"

	};
	
	int nlinenum = GetLineNum();
	int nlineoffset = GetLineOffset();
	if (erroffset == EMPTYIND)
		erroffset =-1;
	printf("Parse Error line %d offset %d: %s\n", nlinenum, nlineoffset - erroffset, errstr[errcode].c_str());
	
	throw 12;
}

int PassOne::GetNonAlphaNum(const std::string &token)
{
	// returns -1 if all alphanum otherwise and reverse index of nonaplhanum
	int i = 0;

	if (token.empty())
		return EMPTYIND;

	for (i = 0; i < (int)token.size(); i++)
	{
		if (!std::isalnum(token[i]))
			break;
	}

	return token.size() - i - 1;
}

int PassOne::GetNonNum(const std::string &token)
{
	// returns -1 if all num otherwise and reverse index of nonnum
	int i = 0;

	if (token.empty())
		return EMPTYIND;

	for (i = 0; i < (int)token.size(); i++)
	{
		if (!std::isdigit(token[i]))
			break;
	}

	return token.size() - i - 1;
}

int PassOne::HandleDefCount()
{
	// check if the token is a number
	int nErrOffset = GetNonNum(m_token);
	if (nErrOffset != -1)
	{
		ParseError(NUM_EXPECTED, nErrOffset);
		return 0;
	}

	int defcount = ParseTokens::HandleDefCount();

	if (defcount > MAXDEFLIST)
	{
		ParseError(TO_MANY_DEF_IN_MODULE, (int)m_token.size()-1);
		return 0;
	}

	return defcount;
}

bool PassOne::CheckSymbol(long errcode)
{
	if (errcode == 0)
		errcode = CHAR_EXPECTED;

	if (m_token.empty())
	{
		ParseError(errcode, EMPTYIND);
		return false;
	}

	//check if its longer than 16, if first char is alpha, and if chars are in [a-Z][a-Z0-9]*
	if (m_token.size() > SYMBOLSIZE)
	{
		ParseError(SYM_TOLONG, (int)m_token.size()-1);
		return false;
	}
	
	if (!std::isalpha(m_token[0]))
	{
		ParseError(errcode, (int)m_token.size()-1);
		return false;
	}

	int nErrOffset = GetNonAlphaNum(m_token);
	if (nErrOffset != -1)
	{
		ParseError(errcode, nErrOffset);
		return false;
	}

	return true;
}

string PassOne::HandleDefSymbol()
{
	if (CheckSymbol(SYM_EXPECTED))
		return ParseTokens::HandleDefSymbol();
	else
		return "";
}

int PassOne::HandleDefSymbolAddr()
{
	int nErrOffset = GetNonNum(m_token);
	if (nErrOffset != -1)
	{
		ParseError(NUM_EXPECTED, nErrOffset);
		return -1;
	}

	return ParseTokens::HandleDefSymbolAddr();
}

void PassOne::HandleDefinition(const string& symname, int symaddr, int mnum)
{
	SymbolInfo *p = new SymbolInfo(symname, symaddr, mnum, 0);

	if (!AddSymInfo(p))
		delete p;
}

int PassOne::HandleUseCount(int mnum)
{
	// check if the token is a number
	int nErrOffset = GetNonNum(m_token);
	if (nErrOffset != -1)
	{
		ParseError(NUM_EXPECTED, nErrOffset);
		return 0;
	}

	int usecount = ParseTokens::HandleUseCount(mnum);

	if (usecount > MAXUSELIST)
	{
		ParseError(TO_MANY_USE_IN_MODULE, (int)m_token.size()-1);
		return 0;
	}

	return usecount;
}

string PassOne::HandleUseSymbol(int mnum)
{
	CheckSymbol(SYM_EXPECTED);
	return "";
}

int PassOne::HandleCodeCount(int mnum)
{
	// check if the token is a number
	int nErrOffset = GetNonNum(m_token);
	if (nErrOffset != -1)
	{
		ParseError(NUM_EXPECTED, nErrOffset);
		return 0;
	}
	
	int nmodsize = ParseTokens::HandleCodeCount(mnum);
	m_TotalProgSize = m_TotalProgSize + nmodsize;
	if (m_TotalProgSize > MACHINESIZE)
	{
		ParseError(TO_MANY_INSTR, (int)m_token.size()-1);
		return 0;
	}

	ModInfo* p = new ModInfo;
	p->Size = nmodsize;
	if (!AddModInfo(p))
		delete p ;

	return nmodsize;
}

string PassOne::HandleCodeType()
{

	if (m_token.size() == 1)
	{
		if (m_token.compare(IMMEDIATE) != 0 && 
			m_token.compare(RELATIVE) != 0 &&
			m_token.compare(ABSOLUTE) != 0 &&
			m_token.compare(EXTERNAL) != 0)
		{
			ParseError(ADDR_EXPECTED, (int)m_token.size()-1);
			return "";
		}
	}
	else
	{
		ParseError(ADDR_EXPECTED, (int)m_token.size()-1);
		return "";
	}

	return "";
}

int PassOne::HandleCodeInstr()
{
	int nErrOffset = GetNonNum(m_token);
	if (nErrOffset != -1)
	{
		ParseError(NUM_EXPECTED, nErrOffset);
	}

	return 0 ;
}

void PassOne::Print()
{
	// **REMOVE need to resolve the sym addresses
	ResolveInfo();

	cout << "Symbol Table" << endl;

	for (SymbolTableItr it = m_symtable.begin(); it != m_symtable.end(); ++it)
	{
		cout << (*it)->SymName << "=" << (*it)->SymAddress;
		PrintErrorsAndWarnings(E2SymMultipleDef	, *it);
		cout << endl;
	}

	cout << endl;
}

class PassTwo : public ParseTokens, public ErrorsAndWarnings
{
public:
	PassTwo();
	PassTwo(const char *filename, PassOne* p);
	~PassTwo();

	void Print();

protected:
	PassOne* m_pPassOne;
protected:
	virtual int HandleUseCount(int mnum=0);
	virtual string HandleUseSymbol(int mnum);
	virtual void HandleDefinition(const string& symname, int symaddr, int mnum) {;};
	virtual void HandleCode(const string& codetype, int codeinstr, int mnum);

protected:
	SymbolTable m_usetable;

	typedef vector<SymbolTable*> UseTable;
	typedef std::vector<SymbolTable*>::iterator UseTableItr;

	UseTable m_usetable2;

	typedef vector<int> MemMap;
	typedef std::vector<int>::iterator MemMapItr;

	typedef vector<long> MemMapErr;
	typedef std::vector<long>::iterator MemMapErrItr;

	typedef vector<string> MemMapSymName;

	MemMap m_MemoryMap;
	MemMapErr m_ErrMap;
	MemMapSymName m_SymName;

protected:
	long m_lMemoryMapErrs;
};

PassTwo::PassTwo()
{
	m_pPassOne = 0;
	m_lMemoryMapErrs = E8AbsoluteEx | E9RelativeEx | E6ExternalEx | E3SymNotDef | E10Immediate | E11Opcode;
}

PassTwo::PassTwo(const char *filename, PassOne* p):ParseTokens(filename)
{
	m_pPassOne = p;
	m_lMemoryMapErrs = E8AbsoluteEx | E9RelativeEx | E6ExternalEx | E3SymNotDef | E10Immediate | E11Opcode;
}

PassTwo::~PassTwo()
{
	for (UseTableItr uit = m_usetable2.begin(); uit != m_usetable2.end(); ++uit)
	{
		for (SymbolTableItr sit = (*uit)->begin(); sit != (*uit)->end(); ++sit)
		{
			delete *sit;
		}

		delete *uit;
	}


/*	for (SymbolTableItr it = m_usetable.begin(); it != m_usetable.end(); ++it)
	{
		delete *it;
	}*/
}

void PassTwo::HandleCode(const string& codetype, int codeinstr, int mnum)
{
	// we are now building the memory map
	int myaddr = 0 ;
	long ewflag = 0;
	int opcode = 0;
	string symname = "";

	if (codetype.compare(IMMEDIATE) == 0)
	{
		// check for rule 10
		if (codeinstr > 9999)
		{
			ewflag = ewflag | E10Immediate;
			codeinstr = 9999;
		}

		myaddr = codeinstr;
	}
	else if (codetype.compare(RELATIVE) == 0)
	{
		// check if its more than 4 digits rule 11
		if (codeinstr < 9999)
		{
			myaddr = codeinstr % 1000;

			opcode = codeinstr/1000;
			opcode = opcode * 1000;

			// check if its not greater that module size rule 9
			if (myaddr >= m_pPassOne->GetModuleSize(mnum))
			{
				ewflag = ewflag | E9RelativeEx;
				myaddr = 0;
			}
			myaddr = opcode + m_pPassOne->GetModuleBaseAddr(mnum) + myaddr;
		}
		else
		{
			ewflag = ewflag | E11Opcode;
			myaddr = 9999;
		}
	}
	else if (codetype.compare(ABSOLUTE) == 0)
	{
		// check if its more than 4 digits rule 11
		if (codeinstr < 9999)
		{
			myaddr = codeinstr % 1000;
			// check if its not greater that module size rule 8
			if (myaddr > MACHINESIZE)
			{
				ewflag = ewflag | E8AbsoluteEx;
				myaddr = codeinstr - myaddr;
			}
			else
				myaddr = codeinstr;
		}
		else
		{
			ewflag = ewflag | E11Opcode;
			myaddr = 9999;
		}
	}
	else if (codetype.compare(EXTERNAL) == 0)
	{
		// check and mark all the symbols that have been used (another way is to mark each symbol in the use map with an
		// error of not used and then remove it if used)
		// check if the indexes work and so on
		// check if its more than 4 digits rule 11
		if (codeinstr < 9999)
		{
			int useidx = codeinstr % 1000;
			// check if the index is within use list rule 6
			if (useidx >= (int)m_usetable2[mnum]->size())
			{
				// use the external as immediate
				ewflag = ewflag | E6ExternalEx;
				myaddr = codeinstr;
			}
			else
			{
				myaddr = codeinstr/1000;
				myaddr = myaddr * 1000;
				myaddr = myaddr + (*m_usetable2[mnum])[useidx]->SymAddress;


				// take care of rule 7
				if ((*m_usetable2[mnum])[useidx]->SymErr & W7SymUseNotUsed)
					(*m_usetable2[mnum])[useidx]->SymErr = (*m_usetable2[mnum])[useidx]->SymErr ^ W7SymUseNotUsed; // remove the not used flag
				// take care of rule 4
				SymbolInfo *pinfo = m_pPassOne->GetSymbolInfo((*m_usetable2[mnum])[useidx]->SymName);
				if (pinfo != 0 && pinfo->SymErr & W4SymDefNotUsed)
				{
					pinfo->SymErr = pinfo->SymErr ^ W4SymDefNotUsed;  // it was used so remove the flag
				}

				symname = (*m_usetable2[mnum])[useidx]->SymName;
				ewflag = ewflag | (*m_usetable2[mnum])[useidx]->SymErr;
			}
		}
		else
		{
			ewflag = ewflag | E11Opcode;
			myaddr = 9999;
		}
	}

	m_MemoryMap.push_back(myaddr);
	m_ErrMap.push_back(ewflag);
	m_SymName.push_back(symname);
}
int PassTwo::HandleUseCount(int mnum)
{
	int nCount = ParseTokens::HandleUseCount(mnum);

	if (mnum > ((int)m_usetable2.size()-1))
	{
		SymbolTable *p = new SymbolTable;
		m_usetable2.push_back(p);
	}

	return nCount;
}

string PassTwo::HandleUseSymbol(int mnum)
{
	SymbolInfo* pdefsym = 0;
	SymbolInfo* pusesym = 0;

	string symbol = ParseTokens::HandleUseSymbol(mnum);

	pdefsym = m_pPassOne->GetSymbolInfo(symbol);

	if (pdefsym == 0)
	{
		// error symbol not defined **IMPROVE** check if the address is 0 or relative 0 and needs to be resloved
		pusesym = new SymbolInfo(symbol, 0, 0, E3SymNotDef | W7SymUseNotUsed); // assume it will not be used
	}
	else
	{
		pusesym = new SymbolInfo(pdefsym);
		pusesym->SymErr = pusesym->SymErr | W7SymUseNotUsed; // assume it will not be used
		//pusesym->ModNum = mnum;
	}
		
/*	if (mnum > ((int)m_usetable2.size()-1))
	{
		SymbolTable *p = new SymbolTable;
		m_usetable2.push_back(p);
	}
*/	
	m_usetable2[mnum]->push_back(pusesym);
	
	//m_usetable.push_back(pusesym);

	return symbol;
}

void PassTwo::Print()
{
	cout << "Memory Map" << endl;
		
	int nCount = 0;
	long lFlag;
	for (MemMapItr it = m_MemoryMap.begin(); it != m_MemoryMap.end(); ++it)
	{
		cout << setw(3) << setfill('0') << nCount << ": " << setw(4) << setfill('0') << *it;
		
		lFlag = m_ErrMap[nCount];
		if (lFlag & m_lMemoryMapErrs)
		{
			PrintErrorsAndWarnings(m_lMemoryMapErrs, lFlag, m_SymName[nCount]);
		}
		cout << endl;
		nCount++;
	}

	cout << endl;
	// print all the warrnings
	// SymbolInfo* PassOne::GetNextSymbolInfobyModandErr(int mod, long err)
	nCount = 0;
	SymbolInfo *pInfo = 0;
	for (UseTableItr uit = m_usetable2.begin(); uit != m_usetable2.end(); ++uit)
	{
		pInfo = m_pPassOne->GetSymbolInfobyModandErr(nCount, W4SymDefNotUsed);
		while (pInfo)
		{
			PrintErrorsAndWarnings(W4SymDefNotUsed, pInfo);
			pInfo = m_pPassOne->GetNextSymbolInfobyModandErr(nCount, W4SymDefNotUsed);
		}


		for (SymbolTableItr sit = (*uit)->begin(); sit != (*uit)->end(); ++sit)
		{
			PrintErrorsAndWarnings(W7SymUseNotUsed, nCount, *sit);
		}


		nCount++;
	}

	cout << endl;
}



int main(int argc, char* argv[])
{
	if (argc < 2)
	{
		cout << "Error no file name" << endl;
		return 0;
	}

	try
	{
		// the PassOne class handles Pass One
		PassOne firstPass(argv[1]);	

		// Do the first pass (Pass One)
		firstPass.Parse();

		firstPass.Print();

		// the PassTwo class handles Pass Two
		PassTwo secondPass(argv[1], &firstPass);

		// Do the second pass (Pass Two)
		secondPass.Parse();

		secondPass.Print();
	}
	catch(...)
	{
		//cout << "exception" << endl;
	}
	


	//system("pause");
	return 0;
}

