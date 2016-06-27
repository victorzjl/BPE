/*
 * =====================================================================================
 *
 *       Filename:  stdafx.h
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  12/10/09 10:57:10
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  ZongJinliang (zjl), 
 *        Company:  SNDA
 *
 * =====================================================================================
 */
#include <stdio.h>
#include <iostream>
#include <string>
#include <fstream>
#include "CohStack.h"
#include "CohResponseMsg.h"
#include "UrlCode.h"
#include "CohLogHelper.h"
#include "CohConnection.h"
#include "CohServer.h"
#include "CohRequestMsg.h"
#include "tinyxml/tinystr.h"
#include "tinyxml/tinyxml.h"
#include "Cipher.h"
#include "TimerManager.h"
#include "MsgThread.h"
#include "SmallObject.h"
#include "detail/queue.h"
#include "detail/TimerRunList.h"
#include "detail/_time.h"
#include "MsgQueuePrio.h"
#include "XmlConfigParser.h"
#include "MemoryPool.h"
#include "Timer.h"
#include "LogManager.h"
#include "ILog.h"
#include "SapMessage.h"
#include "SapAgent.h"
#include "SapConnection.h"
#include "SapStack.h"
#include "SapLogHelper.h"
#include "SapRequestHistory.h"
#include "SapCommon.h"
#include "SapConnManager.h"
#include "SapAgentCallback.h"
