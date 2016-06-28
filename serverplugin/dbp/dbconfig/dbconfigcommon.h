#ifndef  _DB_CONFIG_COMMON_H_
#define _DB_CONFIG_COMMON_H_

#include <vector>
#include <map>
#include <string>
using namespace std;

void HanleNIL(string& s);

typedef enum
{
	DBP_DT_STRING,
	DBP_DT_INT,
	DBP_DT_LONG,
	DBP_DT_FLOAT,
	DBP_DT_STRUCT,
	DBP_DT_TLV,
	DBP_DT_ARRAY,
	DBP_DT_TLV_ARRAY,
	DBP_DT_SYSTEMSTRING,   //前4字节是长度
} EDBPDataType;

typedef enum
{
	DBO_READ,
	DBO_WRITE
} EDBOperType;

typedef enum
{
	BDT_INT,
	BDT_STRING,
	BDT_ARRAY
} EBindDataType;

typedef struct _tagBindDataDesc
{
	EBindDataType eDataType;
	bool bIsHash;
	string sHashName;
	bool bIsArrayElement;
	bool   bSQL;
	bool   bDefault;
	string sDefaultVal;
	bool   bExtend;
	_tagBindDataDesc() : bIsHash(false),bIsArrayElement(false),bDefault(false),bExtend(false){bSQL=false;}
} SBindDataDesc;

typedef struct _tagServiceType
{
	int nTypeCode;
	string sTypeName;
	EDBPDataType eType;
	vector<_tagServiceType> vecSubType; //for struct,tlv
	int nStrLen;
	string sPathFullName; //for struct
	int nElementCode;	  //for array
	string sElementName;
} SServiceType;

typedef struct _tagParam
{
	string sTypeName;
	string sBindName;
	string sParamName; //use for debug only
	int nTypeCode;	
	bool bIsDivide;
	string sHashName;
	bool   bSQL;
	bool   bDefault;
	string sDefaultVal;
	bool   bExtend;
	_tagParam() : bIsDivide(false),bDefault(false),bExtend(false){bSQL=false;}
} SParam;


typedef struct _tagComposeSQL
{
	string sPrefix;
	vector<string> vecDetail;
	string sSuffix;
} SComposeSQL;


typedef struct _tagProcedure
{
	string sProcName;
} SProcedure;

/* divide related */
typedef enum
{
	EDT_FULLSCAN,
	EDT_NODIVIDE,
	EDT_DIVIDE
} EDivideType;

typedef struct _tagDivideDesc
{
	EDivideType eDivideType;
	int nHashParamCode;
	EDBPDataType eParamType;
	int nElementCode;
	_tagDivideDesc() : eDivideType(EDT_NODIVIDE),nHashParamCode(-1),eParamType(DBP_DT_INT) {}
} SDivideDesc;

typedef enum
{
	OP_NOTHING=0,
	OP_NULL=1,
	OP_NOTNULL=2,
} EOPType;

typedef struct _tagPartSql
{
	int    dwConditon;
	string strVariable;
	string strSql;
}SPartSql;

typedef struct _tagMsgAttri
{
	int nMsgId;
	string sMsgName;

	EDBOperType eOperType;
	
	vector<string> exeSQLs; //simple sql
	vector<vector<SPartSql> > exePartSQLs; //mul-part SQL
	vector<int> paraCounts; //参数个数
	vector<SComposeSQL> exeComposeSQLs;
	vector<SProcedure> exeSPs;
	
	vector<SParam> requestParams;
	vector<SParam> responeParams;
	
	bool bMultArray; //
	bool bIsFullScan;
	SDivideDesc divideDesc;	//to speedup

	_tagMsgAttri() : eOperType(DBO_READ),bMultArray(false),bIsFullScan(false){}
} SMsgAttri;

typedef struct _tagServiceDesc
{
	int nServiceId;
	string sServiceName;
	bool  bReadOnly;  //是否仅支持读操作
	map<string,SServiceType> mapServiceType; //key:typename
	map<int,SMsgAttri> mapMsgAttri; //key:msgid
} SServiceDesc;



typedef struct _tagLastErrorDesc
{
	int nErrNo;
	string sErrMsg;
	_tagLastErrorDesc() : nErrNo(0),sErrMsg(""){}
} SLastErrorDesc;






#endif 
