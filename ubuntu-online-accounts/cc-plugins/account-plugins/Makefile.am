plugindir = $(ACCOUNTS_PROVIDER_PLUGIN_DIR)

AM_CPPFLAGS =					\
	-I$(top_builddir)			\
	-I$(top_srcdir)/libempathy		\
	-I$(top_srcdir)/libempathy-gtk		\
	-DLOCALEDIR=\""$(datadir)/locale"\"	\
	$(TPAW_CFLAGS)				\
	$(UOA_CFLAGS)				\
	$(WARN_CFLAGS)				\
	$(ERROR_CFLAGS)				\
	$(DISABLE_DEPRECATED)			\
	$(EMPATHY_CFLAGS)

plugin_LTLIBRARIES = libempathy-accounts-plugin.la

libempathy_accounts_plugin_la_LDFLAGS = \
	-module -avoid-version

libempathy_accounts_plugin_la_SOURCES = \
	empathy-accounts-plugin.c \
	empathy-accounts-plugin.h \
	empathy-accounts-plugin-widget.c \
	empathy-accounts-plugin-widget.h

libempathy_accounts_plugin_la_LIBADD = \
	$(TPAW_LIBS) \
	$(UOA_LIBS) \
	$(top_builddir)/libempathy/libempathy.la \
	$(top_builddir)/libempathy-gtk/libempathy-gtk.la
