from Plugins.Plugin import PluginDescriptor

def autostart(reason, **kwargs):
	from Plugins.SystemPlugins.ServiceHisilicon import servicehisilicon

def Plugins(**kwargs):
	return [
		PluginDescriptor(where = PluginDescriptor.WHERE_AUTOSTART, needsRestart = True, fnc = autostart)
	]
