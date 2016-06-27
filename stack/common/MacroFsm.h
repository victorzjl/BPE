#ifndef _FSM_MACRO_H__
#define _FSM_MACRO_H__
#include <cstdlib>

#define END_EVENT_ID -1
#define END_STATE_ID -1
#define BEGIN_FSM_STATE_TABLE(T,state_stable) static CFSM<T>::STATE_TABLE state_stable={
#define BEGIN_STATE(id,name,enter_func,exit_func,default_func) {id,name,enter_func,exit_func,default_func,{
#define STATE_EVENT_ITEM(func,event,state) {func,event,state},
#define STATE_EVENT_ITEM_SELF(func,event) {func,event,END_STATE_ID},
#define END_STATE(id) {NULL,END_EVENT_ID,END_STATE_ID}}},
#define END_FSM_STATE_TABLE(state_stable) {END_STATE_ID,NULL,NULL,NULL,NULL,NULL}};

namespace sdo{
namespace common{

typedef int FSM_EVENT_ID;
typedef struct event_param_st
{
    FSM_EVENT_ID id;
    void * data;
}FSM_EVENT;

class CFSM_EVENT_STACK
{
	int const static MAX_STACK_NUMBER=10;
private:

	FSM_EVENT data[MAX_STACK_NUMBER];
	int put;
	int get;
public:
	CFSM_EVENT_STACK():put(0),get(0){}
	void PostEvent(const FSM_EVENT & event)
	{
		data[put++]=event;
		if(put==MAX_STACK_NUMBER) 
			put=0;
	}
	int GetEvent(FSM_EVENT ** event)
	{
		if(get==put)
			return -1;
		*event=&(data[get++]);
		if(get==MAX_STACK_NUMBER)
			get=0;
		return 0;
	}
};

template  <typename T> 
class CFSM
{
	int const static SINGLE_STATE_MAX_EVENT=20;
	typedef int FSM_STATE_ID;
	typedef void (T::*FSM_FUNC)(void *);
	typedef struct state_event_st
	{
		FSM_FUNC func;
		FSM_EVENT_ID event;
		FSM_STATE_ID state;
	}FSM_STATE_EVENT;
	typedef struct state_st
	{
		FSM_STATE_ID id;
		char *name;
		FSM_FUNC enter_func;
		FSM_FUNC exit_func;
		FSM_FUNC default_func;
		FSM_STATE_EVENT event_table[SINGLE_STATE_MAX_EVENT]; 
	}FSM_STATE;
public:
	typedef FSM_STATE STATE_TABLE[];
	typedef FSM_STATE * PTR_STATE_TABLE;
	CFSM(T *pObj,PTR_STATE_TABLE table=0,FSM_STATE_ID state=0,FSM_FUNC func=0):
		m_pObj(pObj),
		state_id(state),
		default_func(func),
		state_tables(table)
	{
	}
	FSM_STATE_ID GetState()const{return state_id;}
	char *GetStateName() const{return state_tables[state_id].name;}
	void PostEventRun(int type,void *data)
	{
		FSM_EVENT event;
		event.id=type;
		event.data=data;
		m_stack.PostEvent(event);
		Run_();
	}
	void PostEvent(int type,void *data)
	{
		FSM_EVENT event;
		event.id=type;
		event.data=data;
		m_stack.PostEvent(event);
	}
private:
	void Run_()
	{
	    FSM_STATE *state=&(state_tables[state_id]);
		FSM_EVENT *pEvGet=NULL;
	    int i=0;
		while(m_stack.GetEvent(&pEvGet)==0)
		{
			while(state->event_table[i].event!=END_EVENT_ID)
			{
				if(state->event_table[i].event==pEvGet->id)
					break;
				i++;
			}
			if(state->event_table[i].event!=END_EVENT_ID)
			{
				 if(state->event_table[i].func)
				 {
					(m_pObj->*(state->event_table[i].func))(pEvGet->data);
				 }
			}
			else if(state->default_func)
			{
				(m_pObj->*(state->default_func))(pEvGet->data);
			}
			else if(default_func)
			{
				(m_pObj->*(default_func))(pEvGet->data);
			}
			if(state->id!=state->event_table[i].state&&state->event_table[i].state!=END_STATE_ID)
			{
				if(state->exit_func ) 
				{
					(m_pObj->*(state->exit_func))(pEvGet->data);
				}
			}
		        
			if(state->id!=state->event_table[i].state&&state->event_table[i].state!=END_STATE_ID)
			{
				state_id=state->event_table[i].state;
				if(state_tables[state->event_table[i].state].enter_func) 
				{
					(m_pObj->*(state_tables[state->event_table[i].state].enter_func))(pEvGet->data);
				}
			}
		}
	}
private:
	T *m_pObj;
	FSM_STATE_ID state_id;
	FSM_FUNC default_func;
	PTR_STATE_TABLE state_tables;
	CFSM_EVENT_STACK m_stack;
};
}
}
#endif

