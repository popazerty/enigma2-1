from Tools.Directories import pathExists, fileExists, resolveFilename, SCOPE_PLUGINS, SCOPE_CURRENT_PLUGIN, SCOPE_CURRENT_SKIN, SCOPE_METADIR
  from Tools.LoadPixmap import LoadPixmap
  from Tools.NumericalTextInput import NumericalTextInput
 
from enigma import eTimer, RT_HALIGN_LEFT, RT_VALIGN_CENTER, eListboxPythonMultiContent, eListbox, gFont, getDesktop, ePicLoad, eRCInput, getPrevAsciiCode, eEnv, iRecordableService, getMachineBrand, getMachineName
  from cPickle import dump, load
  from os import path as os_path, system as os_system, unlink, stat, mkdir, popen, makedirs, listdir, access, rename, remove, W_OK, R_OK, F_OK
  from time import time, gmtime, strftime, localtime
 @@ -39,9 +39,11 @@
  from twisted.web import client
  from twisted.internet import reactor
  
  from ImageBackup import ImageBackup
  from ImageWizard import ImageWizard
  from BackupRestore import BackupSelection, RestoreMenu, BackupScreen, RestoreScreen, getBackupPath, getBackupFilename
  from SoftwareTools import iSoftwareTools
  from Flash_online import FlashOnline
  
  config.plugins.configurationbackup = ConfigSubsection()
  config.plugins.configurationbackup.backuplocation = ConfigText(default = '/media/hdd/', visible_width = 50, fixed_size = False)
 @@ -132,6 +134,8 @@ def __init__(self, session, args = 0):
  			self.list.append(("install-extensions", _("Manage extensions"), _("\nManage extensions or plugins for your receiver" ) + self.oktext, None))
  			self.list.append(("software-update", _("Software update"), _("\nOnline update of your receiver software." ) + self.oktext, None))
  			self.list.append(("software-restore", _("Software restore"), _("\nRestore your receiver with a new firmware." ) + self.oktext, None))
  			self.list.append(("flash-online", _("Flash Online"), _("\nFlash on the fly your receiver." ) + self.oktext, None))
  			self.list.append(("backup-image", _("Backup Image"), _("\nBackup your running %s %s image to HDD or USB.") % (getMachineBrand(), getMachineName()) + self.oktext, None))
  			self.list.append(("system-backup", _("Backup system settings"), _("\nBackup your receiver settings." ) + self.oktext + "\n\n" + self.infotext, None))
  			self.list.append(("system-restore",_("Restore system settings"), _("\nRestore your receiver settings." ) + self.oktext, None))
  			self.list.append(("ipkg-install", _("Install local extension"),  _("\nScan for local extensions and install them." ) + self.oktext, None))
 @@ -263,6 +267,10 @@ def go(self, num = None):
  					self.session.open(ImageWizard)
  				elif (currentEntry == "install-extensions"):
  					self.session.open(PluginManager, self.skin_path)
  				elif (currentEntry == "flash-online"):
  					self.session.open(FlashOnline)
  				elif (currentEntry == "backup-image"):
  					self.session.open(ImageBackup)
  				elif (currentEntry == "system-backup"):
  					self.session.openWithCallback(self.backupDone,BackupScreen, runBackup = True)
  				elif (currentEntry == "system-restore"):
