from config import config, ConfigSelection, ConfigSubsection, ConfigOnOff, ConfigSlider
from enigma import eRFmod
from Components.SystemInfo import SystemInfo

# CHECK ME.
RFMOD_CHANNEL_MIN = 21
RFMOD_CHANNEL_MAX = 69 + 1

class RFmod:
	def __init__(self):
		pass

	def setFunction(self, value):
		eRFmod.getInstance().setFunction(not value)
	def setTestmode(self, value):
		eRFmod.getInstance().setTestmode(value)
	def setSoundFunction(self, value):
		eRFmod.getInstance().setSoundFunction(not value)
	def setSoundCarrier(self, value):
		eRFmod.getInstance().setSoundCarrier(value)
	def setChannel(self, value):
		eRFmod.getInstance().setChannel(value)
	def setFinetune(self, value):
		eRFmod.getInstance().setFinetune(value)

def InitRFmod():
	detected = eRFmod.getInstance().detected()
	SystemInfo["RfModulator"] = detected
	if detected:
		config.rfmod = ConfigSubsection()
		config.rfmod.enable = ConfigOnOff(default=False)
		config.rfmod.test = ConfigOnOff(default=False)
		config.rfmod.sound = ConfigOnOff(default=True)
		config.rfmod.soundcarrier = ConfigSelection(choices=[("4500","4.5 MHz"), ("5500", "5.5 MHz"), ("6000", "6.0 MHz"), ("6500", "6.5 MHz")], default="5500")
		config.rfmod.channel = ConfigSelection(default = "36", choices = ["%d" % x for x in range(RFMOD_CHANNEL_MIN, RFMOD_CHANNEL_MAX)])
		config.rfmod.finetune = ConfigSlider(default=5, limits=(1, 10))

		iRFmod = RFmod()

		def setFunction(configElement):
			iRFmod.setFunction(configElement.getValue());
		def setTestmode(configElement):
			iRFmod.setTestmode(configElement.getValue());
		def setSoundFunction(configElement):
			iRFmod.setSoundFunction(configElement.getValue());
		def setSoundCarrier(configElement):
			iRFmod.setSoundCarrier(configElement.index);
		def setChannel(configElement):
			iRFmod.setChannel(int(configElement.getValue()));
		def setFinetune(configElement):
			iRFmod.setFinetune(configElement.getValue() - 5);

		# this will call the "setup-val" initial
		config.rfmod.enable.addNotifier(setFunction);
		config.rfmod.test.addNotifier(setTestmode);
		config.rfmod.sound.addNotifier(setSoundFunction);
		config.rfmod.soundcarrier.addNotifier(setSoundCarrier);
		config.rfmod.channel.addNotifier(setChannel);
		config.rfmod.finetune.addNotifier(setFinetune);
