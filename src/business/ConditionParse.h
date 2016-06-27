#ifndef __APARSE_H__
#define __APARSE_H__

#include <string>
#include <vector>
#include <map>

using namespace std;

struct NumElement;
struct Expression;
struct MutiCheckNode;

typedef enum CompareOperator
{
    COPER_EQUAL,
    COPER_NOTEQUAL,
    COPER_LESS,
    COPER_BIG,
    COPER_NUMBER
};

typedef enum ValueType
{
    VALUETYPE_INT = 0,
    VALUETYPE_FLOAT,
    VALUETYPE_DOUBLE,
    VALUETYPE_STRING,
    VALUETYPE_BOOL,
    VALUETYPE_NUMBER
};

typedef enum RelationshipOperator
{
    ROPER_AND,
    ROPER_OR,
    ROPER_NUMBER
};

struct Expression
{
    bool is_param;
    string param_name;
    unsigned param_index;

    ValueType type;
    string value;
    int int_value;
    bool is_null;

    Expression():is_param(false), param_name(""), param_index(0), type(VALUETYPE_STRING), value("1"), int_value(0), is_null(false){}
};

struct TowParamCompare
{
    Expression      l_expr;
    Expression      r_expr;
    CompareOperator c_oper;

    TowParamCompare() : c_oper(COPER_EQUAL){}
};

struct OneParamCompare
{
    Expression      expr;
};

struct CheckNode
{
    bool result;
    bool result_is_not;

    bool use_two_param_compare;
    OneParamCompare one_compare;
    TowParamCompare two_compare;

    vector<MutiCheckNode> muti_node;

    CheckNode() : result_is_not(false), use_two_param_compare(false){}
};


struct MutiCheckNode
{
    CheckNode            node;
    RelationshipOperator oper;

    MutiCheckNode() : oper(ROPER_AND){}
};

struct ResultNode
{
    bool             isChecked;
    bool             result;
    const CheckNode* node;

    ResultNode () : isChecked(false), result(false), node(NULL){}
};

struct ParameterValue
{
    ValueType   type;
    const void* value;
    int         len;

    ParameterValue() : type(VALUETYPE_STRING), value(NULL), len(0){}
};

class Parse
{
public:
    Parse();
    ~Parse();

    int setExpression(const string& expr_);
    bool getResult(const vector<ParameterValue>& params_) const;
    const vector<string>& getParaNames() const;

    void Dump() const;

private:
    int setNode(const string& expr_, CheckNode& node_);
    void setNodeValue(const string& expr_, CheckNode& node_);
    int getNodeExpression(const string& expr_, string& node_expr_, 
        RelationshipOperator& rs_oper_, string& left_expr_, 
        bool& is_muti_node_, bool& is_not_result) const;
    ValueType getValueType(const string& expr_) const;

    bool getNodeResult(const CheckNode& node_, const vector<ParameterValue>& params_) const;
    bool getResult(const CheckNode& node_, const vector<ParameterValue>& params_) const;

private:
    CheckNode _description;
    vector<string> _param_names;
};

#endif
