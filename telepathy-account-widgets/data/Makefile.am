SUBDIRS = icons

gsettings_SCHEMAS_real = org.gnome.telepathy-account-widgets.gschema.xml

if ENABLE_SETTINGS
gsettings_SCHEMAS = $(gsettings_SCHEMAS_real)
else
gsettings_SCHEMAS =
endif

@GSETTINGS_RULES@

# We need to compile schemas at make time
# to run from source tree
gschemas.compiled: $(gsettings_SCHEMAS:.xml=.valid)
	$(AM_V_GEN) $(GLIB_COMPILE_SCHEMAS) --targetdir=. .

if ENABLE_SETTINGS
all-local: gschemas.compiled
endif

EXTRA_DIST = 				\
	$(schemas_DATA)			\
	$(gsettings_SCHEMAS_real)	\
	$(NULL)

DISTCLEANFILES = 		\
	$(schemas_DATA)		\
	gschemas.compiled	\
	$(NULL)
