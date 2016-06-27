#ifndef _I_TIMER_SERVICE_H_
#define _I_TIMER_SERVICE_H_
extern "C"
{
typedef void (*pFnCallback)(void);
void OnRegisterCallback(pFnCallback fn, unsigned interval) ; 
}
#endif 

