#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <libsig_comp.h>

#include <lib/actions/action.h>
#include <lib/driver/rc.h>
#include <lib/base/ioprio.h>
#include <lib/base/ebase.h>
#include <lib/base/eenv.h>
#include <lib/base/eerror.h>
#include <lib/base/init.h>
#include <lib/base/init_num.h>
#include <lib/gdi/gmaindc.h>
#include <lib/gdi/glcddc.h>
#include <lib/gdi/grc.h>
#include <lib/gdi/epng.h>
#include <lib/gdi/font.h>
#include <lib/gui/ebutton.h>
#include <lib/gui/elabel.h>
#include <lib/gui/elistboxcontent.h>
#include <lib/gui/ewidget.h>
#include <lib/gui/ewidgetdesktop.h>
#include <lib/gui/ewindow.h>
#include <lib/gui/evideo.h>
#include <lib/python/connections.h>
#include <lib/python/python.h>
#include <lib/python/pythonconfig.h>

#include "bsod.h"
#include "version_info.h"

#include <gst/gst.h>

#ifdef OBJECT_DEBUG
int object_total_remaining;

void object_dump()
{
	printf("%d items left\n", object_total_remaining);
}
#endif

static eWidgetDesktop *wdsk, *lcddsk;

static int prev_ascii_code;

int getPrevAsciiCode()
{
	int ret = prev_ascii_code;
	prev_ascii_code = 0;
	return ret;
}

void keyEvent(const eRCKey &key)
{
	static eRCKey last(0, 0, 0);
	static int num_repeat;

	ePtr<eActionMap> ptr;
	eActionMap::getInstance(ptr);
	/*eDebug("key.code : %02x \n", key.code);*/

	if ((key.code == last.code) && (key.producer == last.producer) && key.flags & eRCKey::flagRepeat)
		num_repeat++;
	else
	{
		num_repeat = 0;
		last = key;
	}

	if (num_repeat == 4)
	{
		ptr->keyPressed(key.producer->getIdentifier(), key.code, eRCKey::flagLong);
		num_repeat++;
	}

	if (key.flags & eRCKey::flagAscii)
	{
		prev_ascii_code = key.code;
		ptr->keyPressed(key.producer->getIdentifier(), 510 /* faked KEY_ASCII */, 0);
	}
	else
		ptr->keyPressed(key.producer->getIdentifier(), key.code, key.flags);
}

/************************************************/
#include <unistd.h>
#include <lib/components/scan.h>
#include <lib/dvb/idvb.h>
#include <lib/dvb/dvb.h>
#include <lib/dvb/db.h>
#include <lib/dvb/dvbtime.h>
#include <lib/dvb/epgcache.h>

class eMain: public eApplication, public Object
{
	eInit init;
	ePythonConfigQuery config;

	ePtr<eDVBDB> m_dvbdb;
	ePtr<eDVBResourceManager> m_mgr;
	ePtr<eDVBLocalTimeHandler> m_locale_time_handler;
	ePtr<eEPGCache> m_epgcache;

public:
	eMain()
	{
		init.setRunlevel(eAutoInitNumbers::main);
		/* TODO: put into init */
		m_dvbdb = new eDVBDB();
		m_mgr = new eDVBResourceManager();
		m_locale_time_handler = new eDVBLocalTimeHandler();
		m_epgcache = new eEPGCache();
		m_mgr->setChannelList(m_dvbdb);
	}

	~eMain()
	{
		m_dvbdb->saveServicelist();
		m_mgr->releaseCachedChannel();
	}
};

int exit_code;

int main(int argc, char **argv)
{
#ifdef MEMLEAK_CHECK
	atexit(DumpUnfreed);
#endif

#ifdef OBJECT_DEBUG
	atexit(object_dump);
#endif

	printf("Distro:  %s\n", DISTRO);
	printf("Version: %s\n", IMAGEVERSION);
	printf("Build:   %s\n", IMAGEBUILD);
	printf("Brand:   %s\n", MACHINE_BRAND);
	printf("Boxtype: %s\n", BOXTYPE);
	printf("Machine: %s\n", MACHINE_NAME);
	printf("Drivers: %s\n", DRIVERDATE);

	gst_init(&argc, &argv);

	// set pythonpath if unset
	setenv("PYTHONPATH", eEnv::resolve("${libdir}/enigma2/python").c_str(), 0);
	printf("PYTHONPATH: %s\n", getenv("PYTHONPATH"));

	bsodLogInit();

	ePython python;
	eMain main;

#if 1
	ePtr<gMainDC> my_dc;
	gMainDC::getInstance(my_dc);

	//int double_buffer = my_dc->haveDoubleBuffering();

	ePtr<gLCDDC> my_lcd_dc;
	gLCDDC::getInstance(my_lcd_dc);


	/* ok, this is currently hardcoded for arabic. */
	/* some characters are wrong in the regular font, force them to use the replacement font */
	for (int i = 0x60c; i <= 0x66d; ++i)
		eTextPara::forceReplacementGlyph(i);
	eTextPara::forceReplacementGlyph(0xfdf2);
	for (int i = 0xfe80; i < 0xff00; ++i)
		eTextPara::forceReplacementGlyph(i);

	eWidgetDesktop dsk(my_dc->size());
	eWidgetDesktop dsk_lcd(my_lcd_dc->size());

	dsk.setStyleID(0);
	dsk_lcd.setStyleID(my_lcd_dc->size().width() == 96 ? 2 : 1);

/*	if (double_buffer)
	{
		eDebug(" - double buffering found, enable buffered graphics mode.");
		dsk.setCompositionMode(eWidgetDesktop::cmBuffered);
	} */

	wdsk = &dsk;
	lcddsk = &dsk_lcd;

	dsk.setDC(my_dc);
	dsk_lcd.setDC(my_lcd_dc);

	dsk.setBackgroundColor(gRGB(0,0,0,0xFF));
#endif

		/* redrawing is done in an idle-timer, so we have to set the context */
	dsk.setRedrawTask(main);
	dsk_lcd.setRedrawTask(main);


	eDebug("Loading spinners...");

	{
		int i;
#define MAX_SPINNER 64
		ePtr<gPixmap> wait[MAX_SPINNER];
		for (i=0; i<MAX_SPINNER; ++i)
		{
			char filename[64];
			std::string rfilename;
			snprintf(filename, sizeof(filename), "${datadir}/enigma2/spinner/wait%d.png", i + 1);
			rfilename = eEnv::resolve(filename);
			loadPNG(wait[i], rfilename.c_str());

			if (!wait[i])
			{
				if (!i)
					eDebug("failed to load %s! (%m)", rfilename.c_str());
				else
					eDebug("found %d spinner!\n", i);
				break;
			}
		}
		if (i)
			my_dc->setSpinner(eRect(ePoint(100, 100), wait[0]->size()), wait, i);
		else
			my_dc->setSpinner(eRect(100, 100, 0, 0), wait, 1);
	}

	gRC::getInstance()->setSpinnerDC(my_dc);

	eRCInput::getInstance()->keyEvent.connect(slot(keyEvent));

	printf("executing main\n");

	bsodCatchSignals();

	setIoPrio(IOPRIO_CLASS_BE, 3);

	/* start at full size */
	eVideoWidget::setFullsize(true);

	//	python.execute("mytest", "__main__");
	python.execFile(eEnv::resolve("${libdir}/enigma2/python/mytest.py").c_str());

	/* restore both decoders to full size */
	eVideoWidget::setFullsize(true);

	if (exit_code == 5) /* python crash */
	{
		eDebug("(exit code 5)");
		bsodFatal(0);
	}

	dsk.paint();
	dsk_lcd.paint();

	{
		gPainter p(my_lcd_dc);
		p.resetClip(eRect(ePoint(0, 0), my_lcd_dc->size()));
		p.clear();
		p.flush();
	}

	return exit_code;
}

eWidgetDesktop *getDesktop(int which)
{
	return which ? lcddsk : wdsk;
}

eApplication *getApplication()
{
	return eApp;
}

void quitMainloop(int exitCode)
{
	FILE *f = fopen("/proc/stb/fp/was_timer_wakeup", "w");
	if (f)
	{
		fprintf(f, "%d", 0);
		fclose(f);
	}
	else
	{
		int fd = open("/dev/dbox/fp0", O_WRONLY);
		if (fd >= 0)
		{
			if (ioctl(fd, 10 /*FP_CLEAR_WAKEUP_TIMER*/) < 0)
				eDebug("FP_CLEAR_WAKEUP_TIMER failed (%m)");
			close(fd);
		}
		else
			eDebug("open /dev/dbox/fp0 for wakeup timer clear failed!(%m)");
	}
	exit_code = exitCode;
	eApp->quit(0);
}

static void sigterm_handler(int num)
{
	quitMainloop(128 + num);
}

void runMainloop()
{
	struct sigaction act;

	act.sa_handler = sigterm_handler;
	act.sa_flags = SA_RESTART;

	if (sigemptyset(&act.sa_mask) == -1)
		perror("sigemptyset");
	if (sigaction(SIGTERM, &act, 0) == -1)
		perror("SIGTERM");

	eApp->runLoop();
}

const char *getEnigmaVersionString()
{
	std::string date = enigma2_date;
	return std::string(date).c_str();
}

const char *getDistro()
{
	return DISTRO;
}

const char *getMachineBrand()
{
	FILE *boxtype_file;
	char boxtype_name[20];

	// for OEM resellers
	if((boxtype_file = fopen("/proc/stb/info/boxtype", "r")) != NULL)
	{
		fgets(boxtype_name, sizeof(boxtype_name), boxtype_file);
		fclose(boxtype_file);

		if((strcmp(boxtype_name, "ini-1000\n") == 0)  || (strcmp(boxtype_name, "ini-3000\n") == 0) || (strcmp(boxtype_name, "ini-5000\n") == 0) || (strcmp(boxtype_name, "ini-7000\n") == 0) || (strcmp(boxtype_name, "ini-7012\n") == 0))
		{
			return "UNiBOX";
		}
		else if((strcmp(boxtype_name, "ini-1000sv\n") == 0) || (strcmp(boxtype_name, "ini-5000sv\n") == 0))
		{
			return "Miraclebox";
		}
		else if((strcmp(boxtype_name, "ini-1000ru\n") == 0) || (strcmp(boxtype_name, "ini-5000ru\n") == 0))
		{
			return "Sezam";
		}
		else if((strcmp(boxtype_name, "ini-1000de\n") == 0))
		{
			return "GI";
		}
		else if((strcmp(boxtype_name, "xp1000s\n") == 0))
		{
			return "Octagon";
		}
		else if(strcmp(boxtype_name, "odinm7\n") == 0)
		{
			if(strcmp(BOXTYPE, "odinm6") == 0)
			{
				return "TELESTAR";
			}
		}
		else
		{
			return MACHINE_BRAND;
		}
	}
	return MACHINE_BRAND; // to avoid if no /proc/stb/info/boxtype
}

const char *getMachineName()
{
	FILE *boxtype_file;
	char boxtype_name[20];

	// for OEM resellers
	if((boxtype_file = fopen("/proc/stb/info/boxtype", "r")) != NULL)
	{
		fgets(boxtype_name, sizeof(boxtype_name), boxtype_file);
		fclose(boxtype_file);

		if(strcmp(boxtype_name, "ini-1000\n") == 0) 
		{
			return "HD-e";
		}
		else if(strcmp(boxtype_name, "ini-3000\n") == 0) 
		{
			return "HD-1";
		}
		else if(strcmp(boxtype_name, "ini-5000\n") == 0) 
		{
			return "HD-2";
		}
		else if(strcmp(boxtype_name, "ini-7000\n") == 0) 
		{
			return "HD-3";
		}
		else if(strcmp(boxtype_name, "ini-7012\n") == 0) 
		{
			return "HD-3";
		}
		else if(strcmp(boxtype_name, "ini-1000sv\n") == 0) 
		{
			return "Premium Mini";
		}
		else if(strcmp(boxtype_name, "ini-5000sv\n") == 0) 
		{
			return "Premium Twin";
		}
		else if(strcmp(boxtype_name, "ini-1000ru\n") == 0) 
		{
			return "HD-1000";
		} 
		else if(strcmp(boxtype_name, "ini-5000ru\n") == 0) 
		{
			return "HD-5000";
		}
		else if(strcmp(boxtype_name, "ini-9000ru\n") == 0) 
		{
			return "HD-9000";
		}
		else if(strcmp(boxtype_name, "ini-1000de\n") == 0) 
		{
			return "XpeedLX";
		}
		else if(strcmp(boxtype_name, "xp1000s\n") == 0) 
		{
			return "SF8 HD";
		}
		else if(strcmp(boxtype_name, "odinm7\n") == 0)
		{
			if(strcmp(BOXTYPE, "odinm6") == 0)
			{
				return "STARSAT-LX";
			}
		}
		else
		{
			return MACHINE_NAME;
		}
	}
	return MACHINE_NAME; // to avoid if no /proc/stb/info/boxtype
}

const char *getImageVersionString()
{
	return IMAGEVERSION;
}

const char *getBuildVersionString()
{
	return IMAGEBUILD;
}

const char *getDriverDateString()
{
	return DRIVERDATE;
}

const char *getBoxType()
{
	return BOXTYPE;
}

#include <malloc.h>

void dump_malloc_stats(void)
{
	struct mallinfo mi = mallinfo();
	eDebug("MALLOC: %d total", mi.uordblks);
}
