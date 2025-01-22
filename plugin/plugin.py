from __future__ import absolute_import
from Plugins.Plugin import PluginDescriptor

def autostart(reason, **kwargs):
	from . import servicehisilicon

def Plugins(**kwargs):
	return [
		PluginDescriptor(where = PluginDescriptor.WHERE_AUTOSTART, needsRestart = True, fnc = autostart)
	]
