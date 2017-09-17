AM_CPPFLAGS =                                           \
	$(ERROR_CFLAGS)					\
	-DDATADIR=\""$(datadir)"\"              	\
	-DPKGDATADIR=\""$(real_pkgdatadir)"\"		\
	-DICONDIR=\""$(icondir)"\"			\
	-DG_LOG_DOMAIN=\"tp-account-widgets\"		\
	-DGCR_API_SUBJECT_TO_CHANGE			\
	$(TPAW_CFLAGS)					\
	$(UDEV_CFLAGS)					\
	$(CHEESE_CFLAGS)				\
	$(WARN_CFLAGS)					\
	$(DISABLE_DEPRECATED)

BUILT_SOURCES = 					\
	tpaw-account-widgets-resources.c		\
	tpaw-account-widgets-resources.h		\
	$(NULL)

libtp_account_widgets_sources =            	\
	tpaw-account-settings.c			\
	tpaw-account-widget.c			\
	tpaw-account-widget-irc.c		\
	tpaw-account-widget-private.h		\
	tpaw-account-widget-sip.c		\
	tpaw-avatar-chooser.c			\
	tpaw-builder.c				\
	tpaw-calendar-button.c			\
	tpaw-camera-monitor.c			\
	tpaw-connection-managers.c		\
	tpaw-contactinfo-utils.c		\
	tpaw-debug.c				\
	tpaw-keyring.c				\
	tpaw-irc-network-chooser.c		\
	tpaw-irc-network-chooser-dialog.c	\
	tpaw-irc-network-dialog.c		\
	tpaw-irc-network-manager.c		\
	tpaw-irc-network.c			\
	tpaw-irc-server.c			\
	tpaw-live-search.c			\
	tpaw-pixbuf-utils.c			\
	tpaw-protocol.c				\
	tpaw-string-parser.c			\
	tpaw-time.c				\
	tpaw-user-info.c			\
	tpaw-utils.c				\
	$(NULL)

libtp_account_widgets_headers =			\
	tpaw-account-settings.h			\
	tpaw-account-widget.h			\
	tpaw-account-widget-irc.h		\
	tpaw-account-widget-sip.h		\
	tpaw-avatar-chooser.h			\
	tpaw-builder.h				\
	tpaw-calendar-button.h			\
	tpaw-camera-monitor.h			\
	tpaw-connection-managers.h		\
	tpaw-contactinfo-utils.h		\
	tpaw-debug.h				\
	tpaw-gsettings.h			\
	tpaw-images.h				\
	tpaw-keyring.h				\
	tpaw-irc-network-chooser-dialog.h	\
	tpaw-irc-network-chooser.h		\
	tpaw-irc-network-dialog.h		\
	tpaw-irc-network-manager.h		\
	tpaw-irc-network.h			\
	tpaw-irc-server.h			\
	tpaw-live-search.h			\
	tpaw-pixbuf-utils.h			\
	tpaw-protocol.h				\
	tpaw-string-parser.h			\
	tpaw-time.h				\
	tpaw-user-info.h			\
	tpaw-utils.h				\
	$(NULL)

nocheck_sources = \
	cheese-camera-device-monitor.c		\
	cheese-camera-device-monitor.h		\
	totem-subtitle-encoding.c		\
	totem-subtitle-encoding.h		\
	$(NULL)

noinst_LTLIBRARIES = libtp-account-widgets.la

libtp_account_widgets_la_SOURCES =			\
	$(libtp_account_widgets_sources)		\
	$(libtp_account_widgets_headers)		\
	$(nocheck_sources)				\
	$(NULL)

# these are sources that depend on Ubuntu Online Accounts
uoa_sources = \
	tpaw-uoa-utils.c \
	tpaw-uoa-utils.h \
	$(NULL)

if HAVE_UOA
libtp_account_widgets_la_SOURCES += $(uoa_sources)
EXTRA_DIST =
else
EXTRA_DIST = $(uoa_sources)
endif

# do not distribute generated files
nodist_libtp_account_widgets_la_SOURCES = \
	$(BUILT_SOURCES)

check_c_sources = \
    $(libtp_account_widgets_sources) \
    $(libtp_account_widgets_headers) \
    $(uoa_sources) \
    $(NULL)
include $(top_srcdir)/tools/check-coding-style.mk
check-local: check-coding-style

account_widgets_dtd_files =		\
	tpaw-irc-networks.dtd		\
	$(NULL)

ircnetworksdir = $(real_pkgdatadir)
ircnetworks_DATA =		\
	irc-networks.xml

account_widgets_ui_files =			\
	tpaw-account-widget-generic.ui		\
	tpaw-account-widget-jabber.ui		\
	tpaw-account-widget-msn.ui		\
	tpaw-account-widget-sip.ui		\
	tpaw-account-widget-local-xmpp.ui	\
	tpaw-account-widget-irc.ui		\
	tpaw-account-widget-icq.ui		\
	tpaw-account-widget-yahoo.ui		\
	tpaw-account-widget-groupwise.ui	\
	tpaw-account-widget-aim.ui		\
	$(NULL)

account_widgets_resource_files = $(shell $(GLIB_COMPILE_RESOURCES) --generate-dependencies --sourcedir=$(srcdir) $(srcdir)/tpaw-account-widgets.gresource.xml)

tpaw-account-widgets-resources.c: tpaw-account-widgets.gresource.xml $(account_widgets_resource_files)
	$(AM_V_GEN)$(GLIB_COMPILE_RESOURCES) --target=$@ --sourcedir=$(srcdir) --generate-source $<

tpaw-account-widgets-resources.h: tpaw-account-widgets.gresource.xml $(account_widgets_resource_files)
	$(AM_V_GEN)$(GLIB_COMPILE_RESOURCES) --target=$@ --sourcedir=$(srcdir) --generate-header $<

EXTRA_DIST +=					\
	tpaw-account-widgets.gresource.xml	\
	$(account_widgets_dtd_files)		\
	$(account_widgets_ui_files)		\
	$(ircnetworks_DATA)			\
	$(NULL)

CLEANFILES =				\
	$(BUILT_SOURCES)		\
	$(NULL)
