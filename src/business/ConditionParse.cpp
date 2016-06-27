#include "ConditionParse.h"
#include "SapLogHelper.h"

unsigned getRightIndex(const char* str_, unsigned len_, unsigned in_ = 0)
{
    unsigned left = 0;
    unsigned right = 0;
    for(; in_ < len_; in_++)
    {
        if (str_[in_] == '(')
        {
            left++;
        }
        else if (str_[in_] == ')')
        {
            if (left == ++right)
            {
                break;
            }
        }
    }
    return in_;
}

bool isOneParamCompare(const string& expr_)
{
    return expr_.find("=") == string::npos &&
        expr_.find("<") == string::npos &&
        expr_.find(">") == string::npos;
}
bool isParam(const string& expr_)
{
    return *(expr_.c_str()) == '$'|| *(expr_.c_str()) == '%';
}

string trimSpace(const string& expr_)
{
    string out;
    if (!expr_.empty())
    {
        string::size_type index_begin = expr_.find_first_not_of(' ');
        string::size_type index_end = expr_.find_last_not_of(' ');
        out.assign( expr_.c_str() + index_begin, index_end - index_begin + 1);
    }
    return out;
}

void Parse::setNodeValue(const string& expr_, CheckNode& node_)
{
    SS_XLOG(XLOG_TRACE,"Parse::%s, expr_[%s]\n", __FUNCTION__, expr_.c_str());
    string node_expr = expr_;

    if (isOneParamCompare(node_expr))
    {
        node_.use_two_param_compare       = false;
        node_.one_compare.expr.is_param   = isParam(node_expr);
        if (node_.one_compare.expr.is_param)
        {
            node_.one_compare.expr.param_name = trimSpace(node_expr);
            node_.one_compare.expr.param_index = _param_names.size();
            _param_names.push_back(node_.one_compare.expr.param_name);
        }
        else
        {
            node_.one_compare.expr.type = getValueType(node_expr);
            node_.one_compare.expr.value = node_expr;
            if (node_.one_compare.expr.type == VALUETYPE_INT)
            {
                node_.one_compare.expr.int_value = atoi(node_expr.c_str());
            }
        }
    }
    else    // two parameter compare
    {
        string l_expr;
        string r_expr;
        CompareOperator c_oper = COPER_EQUAL;
        string::size_type index_oper = string::npos;

        if ((index_oper = node_expr.find("==")) != string::npos)
        {
            l_expr.assign(node_expr.c_str(), index_oper);
            r_expr.assign(node_expr.c_str() + index_oper + 2);
            c_oper = COPER_EQUAL;
        }
        else if ((index_oper = node_expr.find("!=")) != string::npos)
        {
            l_expr.assign(node_expr.c_str(), index_oper);
            r_expr.assign(node_expr.c_str() + index_oper + 2);
            c_oper = COPER_NOTEQUAL;
        }
        else if ((index_oper = node_expr.find("<")) != string::npos)
        {
            l_expr.assign(node_expr.c_str(), index_oper);
            r_expr.assign(node_expr.c_str() + index_oper + 1);
            c_oper = COPER_LESS;
        }
        else if ((index_oper = node_expr.find(">")) != string::npos)
        {
            l_expr.assign(node_expr.c_str(), index_oper);
            r_expr.assign(node_expr.c_str() + index_oper + 1);
            c_oper = COPER_BIG;
        }

        l_expr = trimSpace(l_expr);
        r_expr = trimSpace(r_expr);

        if (!l_expr.empty())
        {
            node_.use_two_param_compare = true;
            node_.two_compare.c_oper = c_oper;
            node_.two_compare.l_expr.is_param = isParam(l_expr);
            node_.two_compare.r_expr.is_param = isParam(r_expr);
            node_.two_compare.l_expr.value = l_expr;
            node_.two_compare.r_expr.value = r_expr;
            node_.two_compare.l_expr.type = VALUETYPE_STRING;
            node_.two_compare.r_expr.type = VALUETYPE_STRING;

            if (node_.two_compare.l_expr.is_param)
            {
                node_.two_compare.l_expr.param_name = l_expr;
                node_.two_compare.l_expr.param_index = _param_names.size();
                _param_names.push_back(node_.two_compare.l_expr.param_name);
            }
            else
            {
                node_.two_compare.l_expr.type = getValueType(l_expr);
                if (node_.two_compare.l_expr.type == VALUETYPE_INT)
                {
                    node_.two_compare.l_expr.int_value = atoi(l_expr.c_str());
                }
                else if (node_.two_compare.l_expr.value == "NULL")
                {
                    node_.two_compare.l_expr.is_null = true;
                }
            }

            if (node_.two_compare.r_expr.is_param)
            {
                node_.two_compare.r_expr.param_name = r_expr;
                node_.two_compare.r_expr.param_index = _param_names.size();
                _param_names.push_back(node_.two_compare.r_expr.param_name);
            }
            else
            {
                node_.two_compare.r_expr.type = getValueType(r_expr);
                if (node_.two_compare.r_expr.type == VALUETYPE_INT)
                {
                    node_.two_compare.r_expr.int_value = atoi(r_expr.c_str());
                }
                else if (node_.two_compare.r_expr.value == "NULL")
                {
                    node_.two_compare.r_expr.is_null = true;
                }
            }

            // set both as same type 
            if ((!node_.two_compare.l_expr.is_param && node_.two_compare.l_expr.type == VALUETYPE_STRING) ||
                (!node_.two_compare.r_expr.is_param && node_.two_compare.r_expr.type == VALUETYPE_STRING))
            {
                node_.two_compare.l_expr.type = VALUETYPE_STRING;
                node_.two_compare.r_expr.type = VALUETYPE_STRING;
            }
        } // !l_expr.empty()
    } // isOneParamCompare(node_expr)
}

int Parse::setNode(const string& expr_, CheckNode& node_)
{
    SS_XLOG(XLOG_TRACE,"Parse::%s, expr_[%s]\n",__FUNCTION__, expr_.c_str());
    string expr = expr_;
    string node_expr;
    string left_expr;
    RelationshipOperator rs_oper = ROPER_AND;
    bool is_muti_node;
    bool is_not_result;

    if (getNodeExpression(expr, node_expr, rs_oper, left_expr, is_muti_node, is_not_result) != 0)
    {
        SS_XLOG(XLOG_ERROR,"Parse::%s, getNodeExpression error. expr[%s]\n",__FUNCTION__, expr_.c_str());
        return -1;
    }
    
    if (is_muti_node)
    {
        MutiCheckNode m_node_new;
        m_node_new.oper = rs_oper;
        m_node_new.node.result_is_not = is_not_result;
        if (setNode(node_expr, m_node_new.node) != 0)
        {
            return -1;
        }
        node_.muti_node.push_back(m_node_new);

        node_.use_two_param_compare = false;
        node_.one_compare.expr.is_param = false;
        node_.one_compare.expr.type = VALUETYPE_INT;
        node_.one_compare.expr.int_value = 1;
        node_.one_compare.expr.value = "1";
    }
    else
    {
        setNodeValue(node_expr, node_);
    }

    expr = trimSpace(left_expr);

    while (!expr.empty())
    {
        if (getNodeExpression(expr, node_expr, rs_oper, left_expr, is_muti_node, is_not_result) != 0)
        {
            SS_XLOG(XLOG_ERROR,"Parse::%s, getNodeExpression error. expr[%s]\n",__FUNCTION__, expr.c_str());
            return -1;
        }
        
        MutiCheckNode m_node_new;
        m_node_new.oper = rs_oper;
        m_node_new.node.result_is_not = is_not_result;
        if (setNode(node_expr, m_node_new.node) != 0)
        {
            return -1;
        }
        node_.muti_node.push_back(m_node_new);

        expr = trimSpace(left_expr);
    }

    return 0;
}

int Parse::getNodeExpression(const string& expr_, string& node_expr_, RelationshipOperator& rs_oper_, string& left_expr_, bool& is_muti_node_, bool& is_not_result)  const
{
    string expr = trimSpace(expr_);

    rs_oper_ = ROPER_AND;
    if (strncmp("&&", expr.c_str(), 2) == 0)
    {
        rs_oper_ = ROPER_AND;
        expr.erase(0, 2);
        expr = trimSpace(expr);
    }
    else if (strncmp("||", expr.c_str(), 2) == 0)
    {
        rs_oper_ = ROPER_OR;
        expr.erase(0, 2);
        expr = trimSpace(expr);
    }

    is_not_result = false;
    if (*expr.c_str() == '!')
    {
        is_not_result = true;
        expr.erase(0, 1);
    }

    if (strncmp("(", expr.c_str(), 1) == 0)
    {
        string::size_type index = expr.find(")");
        if (index == string::npos)
        {
            SS_XLOG(XLOG_WARNING,"Parse::%s, can't find ')'. expr[%s]\n", __FUNCTION__, expr.c_str());
            return -1;
        }

        index = getRightIndex(expr.c_str(), expr.size());
        if (*(expr.c_str() + index) != ')')
        {
            SS_XLOG(XLOG_WARNING,"Parse::%s, can't find ')'. expr[%s]\n", __FUNCTION__, expr.c_str());
            return -1;
        }

        node_expr_.assign(expr.c_str() + 1, index - 1);
        left_expr_.assign(expr.c_str() + index + 1);

        is_muti_node_ = true;
    }
    else
    {
        string::size_type index = string::npos;
        string::size_type index_and = expr.find("&&");
        string::size_type index_or  = expr.find("||");

        if (index_and != string::npos && index_or == string::npos)
        {
            index = index_and;
        }
        else if (index_or != string::npos && index_and == string::npos)
        {
            index = index_or;
        }
        else if (index_or != string::npos && index_and != string::npos)
        {
            index = index_and > index_or ? index_or : index_and;
        }

        if (index != string::npos)
        {
            node_expr_.assign(expr.c_str(), index);
            left_expr_.assign(expr.c_str() + index);
        }
        else
        {
            node_expr_ = expr;
            left_expr_.clear();
        }

        is_muti_node_ = false;
    }
    SS_XLOG(XLOG_TRACE,"Parse::%s, expr_[%s], node_expr_[%s], rs_oper_[%d], left_expr_[%s], is_muti_node_[%s], is_not_result[%s]\n",
        __FUNCTION__, expr_.c_str(), node_expr_.c_str(), rs_oper_, left_expr_.c_str(), is_muti_node_ ? "T" : "F", is_not_result ? "T" : "F");

    return 0;
}

ValueType Parse::getValueType(const string& expr_) const
{
    SS_XLOG(XLOG_TRACE,"Parse::%s, expr_[%s]\n",__FUNCTION__, expr_.c_str());

    ValueType type = VALUETYPE_INT;

    unsigned len = expr_.length();
    for (unsigned i = 0; i < len; i++)
    {
        if (!isdigit(*(expr_.c_str() + i)))
        {
            if (i == 0 && *(expr_.c_str() + i) == '-')
            {
                continue;
            }
            else if (*(expr_.c_str() + i) == '.')
            {
                if (type == VALUETYPE_INT && i > 0)
                {
                    type = VALUETYPE_FLOAT;
                }
                else
                {
                    type = VALUETYPE_STRING;
                    break;
                }
            }
            else
            {
                type = VALUETYPE_STRING;
                break;
            }
        }
    }

    return type;
}

bool Parse::getNodeResult(const CheckNode& node_, const vector<ParameterValue>& params_) const
{
    SS_XLOG(XLOG_TRACE,"Parse::%s, params_ size[%u]\n",__FUNCTION__, params_.size());
    bool result = false;

    if (node_.use_two_param_compare)
    {
        ValueType type = node_.two_compare.l_expr.type;
        const void* value_l = node_.two_compare.l_expr.value.c_str();
        const void* value_r = node_.two_compare.r_expr.value.c_str();
        int len_l = node_.two_compare.l_expr.value.length();
        int len_r = node_.two_compare.r_expr.value.length();

        if (node_.two_compare.l_expr.is_param)
        {
            const ParameterValue& pvalue_l = params_[node_.two_compare.l_expr.param_index];
            type = pvalue_l.type;
            value_l = pvalue_l.value;
            len_l = pvalue_l.len;
        }
        if (node_.two_compare.r_expr.is_param)
        {
            const ParameterValue& pvalue_r = params_[node_.two_compare.r_expr.param_index];
            type = pvalue_r.type;
            value_r = pvalue_r.value;
            len_r = pvalue_r.len;
        }

        if (node_.two_compare.l_expr.is_null)
        {
            result = !value_r || len_r < 1 || strncmp((const char*)value_r, "NULL", 4) == 0;
            result = node_.two_compare.c_oper == COPER_EQUAL ? result : !result;

            SS_XLOG(XLOG_TRACE,"Parse::%s, value left is null, value right [%p] size[%d]\n",
                __FUNCTION__, value_r, len_r);
        }
        else if (node_.two_compare.r_expr.is_null)
        {
            result = !value_l || len_l < 1 || strncmp((const char*)value_l, "NULL", 4) == 0;
            result = node_.two_compare.c_oper == COPER_EQUAL ? result : !result;

            SS_XLOG(XLOG_TRACE,"Parse::%s, value right is null, value left [%p] size[%d]\n",
                __FUNCTION__, value_l, len_l);
        }
        else if (value_l && value_r)
        {
            if (type == VALUETYPE_INT)
            {
                if (!node_.two_compare.l_expr.is_param)
                {
                    value_l = &(node_.two_compare.l_expr.int_value);
                }
                if (!node_.two_compare.r_expr.is_param)
                {
                    value_r = &(node_.two_compare.r_expr.int_value);
                }

                switch (node_.two_compare.c_oper)
                {
                case COPER_EQUAL:
                    result = *(int*)value_l == *(int*)value_r;
                    break;
                case COPER_NOTEQUAL:
                    result = *(int*)value_l != *(int*)value_r;
                    break;
                case COPER_LESS:
                    result = *(int*)value_l < *(int*)value_r;
                    break;
                case COPER_BIG:
                    result = *(int*)value_l > *(int*)value_r;
                    break;
                default:
                    result = false;
                    break;
                }

                SS_XLOG(XLOG_TRACE,"Parse::%s, value left[%d], value right[%d]\n",
                    __FUNCTION__, *(int*)value_l, *(int*)value_r);
            }
            else
            {
                switch (node_.two_compare.c_oper)
                {
                case COPER_EQUAL:
                    result = len_l == len_r ? strncmp((const char*)value_l, (const char*)value_r, len_l) == 0 : false;
                    break;
                case COPER_NOTEQUAL:
                    result = len_l != len_r ? true : strncmp((const char*)value_l, (const char*)value_r, len_l) != 0;
                    break;
                default:
                    result = false;
                    break;
                }

                SS_XLOG(XLOG_TRACE,"Parse::%s, value left[%s], value right[%s]\n",
                    __FUNCTION__, value_l, value_r);
            }
        }
        else
        {
            SS_XLOG(XLOG_DEBUG,"Parse::%s, value left[%p], value right[%p]\n",
                __FUNCTION__, value_l, value_r);
        }

        SS_XLOG(XLOG_DEBUG,"Parse::%s, value left[%s], value right[%s], type[%d], operation[%d], result[%s]\n",
            __FUNCTION__, node_.two_compare.l_expr.value.c_str(), node_.two_compare.r_expr.value.c_str(), type, node_.two_compare.c_oper, result ? "T" : "F");
    }
    else    // ! node_.use_two_param_compare
    {
        ValueType type = node_.one_compare.expr.type;
        const void* value_o = node_.one_compare.expr.value.c_str();
        int len_o = node_.one_compare.expr.value.length();
        result = true;

        if (node_.one_compare.expr.is_param)
        {
            const ParameterValue& pvalue_o = params_[node_.one_compare.expr.param_index];
            type = pvalue_o.type;
            value_o = pvalue_o.value;
            len_o = pvalue_o.len;
        }

        if (type == VALUETYPE_INT && value_o)
        {
            result = *(int*)value_o;
        }
        else
        {
            if (len_o < 1 ||
                value_o == NULL ||
                (len_o == 1 && *(const char*)value_o == '0'))
            {
                result = false;
            }
        }

        SS_XLOG(XLOG_DEBUG,"Parse::%s, value[%s], result[%s]\n",
            __FUNCTION__, node_.one_compare.expr.value.c_str(), result ? "T" : "F");
    }

    return node_.result_is_not ? !result : result;
}

Parse::Parse(){}
Parse::~Parse(){}

int Parse::setExpression(const string& expr_)
{
    if (setNode(expr_, _description) == 0)
    {
        //Dump();
        return 0;
    }
    return -1;
}

bool Parse::getResult(const CheckNode& node_, const vector<ParameterValue>& params_) const
{
    SS_XLOG(XLOG_TRACE,"Parse::%s, params_ size[%u]\n",__FUNCTION__, params_.size());
    ResultNode result_node;
    vector<ResultNode> check_list;

    result_node.isChecked = true;
    result_node.result = getNodeResult(node_, params_);
    check_list.push_back(result_node);

    vector<MutiCheckNode>::const_iterator itr = node_.muti_node.begin();
    for (; itr != node_.muti_node.end(); ++itr)
    {
        if (itr->oper == ROPER_AND)
        {
            vector<ResultNode>::reference ref_last_node = check_list.back();

            bool result_before = ref_last_node.isChecked ? 
                ref_last_node.result : getResult(*ref_last_node.node, params_);

            ref_last_node.result = !result_before ? false : getResult(itr->node, params_);
            ref_last_node.isChecked = true;
        }
        else if (itr->oper == ROPER_OR)
        {
            result_node.isChecked = false;
            result_node.node = &(itr->node);
            check_list.push_back(result_node);
        }
    }

    bool result = false;
    vector<ResultNode>::iterator itr_tc = check_list.begin();
    for (; itr_tc != check_list.end(); ++itr_tc)
    {
        result = itr_tc->isChecked ? itr_tc->result : getResult(*itr_tc->node, params_);
        if (result)
        {
            break;
        }
    }

    return result;
}

bool Parse::getResult(const vector<ParameterValue>& params_) const
{
    if (params_.size() != _param_names.size())
    {
        SS_XLOG(XLOG_WARNING,"Parse::%s, the paramerter size is error. params size[%u], need param size[%u]\n",
            __FUNCTION__, params_.size(), _param_names.size());
        return false;
    }
    return getResult(_description, params_);
}

const vector<string>& Parse::getParaNames() const
{
    return _param_names;
}

void Parse::Dump() const
{
    SS_XLOG(XLOG_DEBUG,"Parse::%s, parameter size[%u]\n", __FUNCTION__, _param_names.size());
    vector<string>::const_iterator itr_param = _param_names.begin();
    for(; itr_param != _param_names.end(); ++itr_param)
    {
        SS_XLOG(XLOG_TRACE,"Parse::%s, param[%s]\n",__FUNCTION__, itr_param->c_str());
    }
}

/*int main()
{
    //printf("%s\n", 1 && 1 || 1 && 0 ? "yes" : "no");
    Parse p;
    //string exp = "($a == 3) && (3.0 < $c && ($d == abc) || $b == abc)";
    string exp = "($p1 > 3 && $p2 < $p1) || ($p3 == 5 && $p4) || ($p6 && (($p7 == 8 && $p8) && $p9)) && $p10 > 0";
    exp = "($p1 > 3 && $p2 > $p1) || (($p3 == 5 && $p4) && ($p6 && (($p7 == 8 && $p8) || $p9)) && $p10 > 0)";
    //exp = "$p6 || ($p7 == 8)";
    //exp = "($p1 > 3 && $p2 > $p1) || ($p3 != 5 && $p4) || ($p6 && (($p7 == 8 && $p8) || $p9)) || $p10 > 0";
    //exp = "1 && 1 || 1 && 0";
    map<string, string> values;
    values["p1"] = "3";
    values["p2"] = "2";
    values["p3"] = "3";
    values["p4"] = "1";
    values["p5"] = "1";
    values["p6"] = "1";
    values["p7"] = "7";
    values["p8"] = "1";
    values["p9"] = "0";
    values["p10"] = "0";

//     exp = "($p1 > 3 && $p2 > $p1) || ($p3 != 5 && $p4) || ($p6 && (($p7 == 8 && $p8) || $p9)) || $p10 > 0";
//     map<string, string> values;
//     values["p1"] = "4";
//     values["p2"] = "2";
//     values["p3"] = "5";
//     values["p4"] = "1";
//     values["p5"] = "1";
//     values["p6"] = "0";
//     values["p7"] = "8";
//     values["p8"] = "1";
//     values["p9"] = "0";
//     values["p10"] = "0";

    p.setExpression(exp);
    p.setParam(values);

    printf("result = [%s]\n", p.getResult() ? "TRUE" : "FALSE");

    system("PAUSE");

    return 0;
}*/

