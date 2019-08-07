plugindir = $(libdir)/pragha/plugins/sample
plugin_LTLIBRARIES = libsample.la

libsample_la_SOURCES =      \
	pragha-sample-plugin.c

libsample_la_CFLAGS = \
	$(PRAGHA_PLUGINS_CFLAGS)

libsample_la_LIBADD = \
	$(PRAGHA_PLUGINS_LIBS)


plugin_DATA = sample.plugin
EXTRA_DIST = $(plugin_DATA)