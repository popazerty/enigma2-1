#include <lib/components/tuxtopendroidpp.h>
#include <lib/base/init.h>
#include <lib/base/init_num.h>
#include <lib/driver/rc.h>
#include <lib/gdi/lcd.h>
#include <lib/gdi/fb.h>
#include <lib/gui/ewidget.h>
#include <lib/gui/ewidgetdesktop.h>
#include <lib/python/python.h>

extern "C" int tuxtxt_run_ui(int pid, int demux);
extern "C" int tuxtxt_init();
extern "C" void tuxtxt_start(int tpid, int demux);
extern "C" int tuxtxt_stop();
extern "C" void tuxtxt_close();

eAutoInitP0<eTuxtopendroidpp> init_eTuxtopendroidpp(eAutoInitNumbers::lowlevel, "Tuxtxt");
eTuxtopendroidpp *eTuxtopendroidpp::instance = NULL;

eTuxtopendroidpp::eTuxtopendroidpp() : pid(0), enableTtCaching(false), uiRunning(false), messagePump(eApp, 0)
{
	CONNECT(messagePump.recv_msg, eTuxtopendroidpp::recvEvent);
	pthread_mutex_init( &cacheChangeLock, 0 );
	if (!instance)
		instance=this;
}

eTuxtopendroidpp::~eTuxtopendroidpp()
{
	if (instance==this)
		instance=0;
	kill();
	pthread_mutex_destroy( &cacheChangeLock );
}

void eTuxtopendroidpp::recvEvent(const int &evt)
{
	uiRunning = false;
	eRCInput::getInstance()->unlock();
	eDBoxLCD::getInstance()->unlock();
	eDBoxLCD::getInstance()->update();
	fbClass::getInstance()->unlock();
	/* emit */appClosed();
}

int eTuxtopendroidpp::startUi()
{
	if (fbClass::getInstance()->lock() >= 0)
	{
		eDBoxLCD::getInstance()->lock();
		eRCInput::getInstance()->lock();
		pthread_mutex_lock( &cacheChangeLock );
		uiRunning = true;
		pthread_mutex_unlock( &cacheChangeLock );
		run();
	}
	else
	{
		/* emit */appClosed();
	}
	return 0;
}

void eTuxtopendroidpp::thread()
{
	hasStarted();
	tuxtxt_run_ui(pid, demux);
}

void eTuxtopendroidpp::thread_finished()
{
	messagePump.send(0);
}

void eTuxtopendroidpp::initCache()
{
	tuxtxt_init();
}

void eTuxtopendroidpp::freeCache()
{
	pthread_mutex_lock( &cacheChangeLock );
	if ( !uiRunning )
	{
		tuxtxt_close();
		pid = 0;
	}
	pthread_mutex_unlock( &cacheChangeLock );
}

void eTuxtopendroidpp::startCaching( int tpid, int tdemux)
{
	pid = tpid;
	demux = tdemux;
	if (enableTtCaching)
		tuxtxt_start(pid, demux);
}

void eTuxtopendroidpp::stopCaching()
{
	pthread_mutex_lock( &cacheChangeLock );
	if ( !uiRunning )
		tuxtxt_stop();

	pthread_mutex_unlock( &cacheChangeLock );
}

void eTuxtopendroidpp::setEnableTtCachingOnOff( int onoff )
{
	if (onoff && !enableTtCaching)		// Switch caching on
	{
		enableTtCaching = true;
		if (pid)
		{
			initCache();
			startCaching(pid, demux);
		}
	}
	else if (!onoff && enableTtCaching)	// Switch caching off
	{
		enableTtCaching = false;
		int savePid = pid;
		freeCache();
		pid = savePid;
	}
}
