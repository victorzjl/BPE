#ifndef _SAP_ADMIN_COMMAND_H_
#define _SAP_ADMIN_COMMAND_H_
#include <boost/unordered_map.hpp>
#include <string>
#include <map>
#include "SocConfUnit.h"

using std::map;
using std::string;
class CSapAdminCommand
{
	typedef string (CSapAdminCommand::*pfuncCommand)(const string &strVariant);
public:
	static CSapAdminCommand * Instance();
	int LoadManagerSetting(const string & strPath);
	string DoAdminCommand(const string &strCommand);
	void Dump()const;
private:
	string DoHelp_(const string &strVariant);
	string DoShowSessions_(const string &strVariant);
	string DoSendCommand_(const string &strVariant);
	string DoReloadConfig_(const string &strVariant);
private:
	CSapAdminCommand();
	static CSapAdminCommand * sm_instance;
	boost::unordered_map<string,pfuncCommand> m_mapFuncs;
	map<int,SSapCommandTest *> m_mapSapCommand;
};
#endif
