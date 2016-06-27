#ifndef __STATE__MACHINE__BPE__
#define __STATE__MACHINE__BPE__
#include "tinyxml/ex_tinyxml.h"
#include <map>
#include <string>
using std::string;
using std::map;

typedef map<int, int> MAPEvents;
typedef map<int, MAPEvents > MAPStateEvents;
typedef map<string, MAPStateEvents> MAPStateMachine;
typedef map<int ,string> MAPNStates;
typedef map<string,int> MAPStrStates;
typedef map<string, MAPNStates> MAPMachineNStates;
typedef map<string, MAPStrStates> MAPMachineStrStates;
class CStateMachine
{
public:
	static CStateMachine *Instance();
	int LoadFiles(const string &strAdditionalPath);
	int ExcuteStateMachine(const string &strMachineName, int nState, int nEvent);
	string GetMachineStateName(const string &strMachineName, int nState);
	int GetMachineState(const string &strMachineName,const string & strState);
	void Dump();
private:
	CStateMachine(){}
	int LoadMachineFile_(const string &strFile);
	int LoadVariant_(sdo::util::TiXmlElement * pDef, string & strName, int & nValue);
	
private:
	
	static CStateMachine *m_pInstance;	
	MAPStateMachine m_machine;
	MAPMachineNStates m_nStates;
	MAPMachineStrStates m_strStates;
};
#endif

