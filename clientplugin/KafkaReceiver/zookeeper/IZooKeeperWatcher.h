#ifndef _I_ZOO_KEEPER_WATCHER_H_
#define _I_ZOO_KEEPER_WATCHER_H_

class CZooKeeper;

class IZooKeeperWatcher
{
public:
    //A node has been created
	virtual int OnCreatedEvent(CZooKeeper *pZooKeeper, const char *path) { return 0; }
	//A node has been deleted
	virtual int OnDeletedEvent(CZooKeeper *pZooKeeper, const char *path) { return 0; }
	//A node has been changed
	virtual int OnChangedEvent(CZooKeeper *pZooKeeper, const char *path) { return 0; }
	//A change as occurred in the list of children
	virtual int OnChildEvent(CZooKeeper *pZooKeeper, const char *path) { return 0; }
	//A session has been lost
	virtual int OnSessionEvent(CZooKeeper *pZooKeeper, const char *path) { return 0; }
	//A watch has been removed
	virtual int OnNotWatchingEvent(CZooKeeper *pZooKeeper, const char *path) { return 0; }
	//Reconnect successfully
	virtual int OnReConnSuccEvent(CZooKeeper *pZooKeeper) { return 0; }
};

#endif //_I_ZOO_KEEPER_WATCHER_H_
