SUBDIRS = mc-plugin cc-plugins

appdir = $(ACCOUNTS_APPLICATION_FILES_DIR)

empathy.application: empathy.application.in
	$(AM_V_GEN)intltool-merge --no-translations -x -u $< $@

app_DATA = empathy.application

EXTRA_DIST = empathy.application.in

DISTCLEANFILES = $(app_DATA)
