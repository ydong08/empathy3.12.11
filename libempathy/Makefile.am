include $(top_srcdir)/tools/flymake.mk

AM_CPPFLAGS =                                           \
	$(ERROR_CFLAGS)					\
	-I$(top_srcdir)/extensions                      \
	-I$(top_builddir)                               \
	-DDATADIR=\""$(datadir)"\"              	\
	-DLOCALEDIR=\""$(datadir)/locale"\"		\
	-DG_LOG_DOMAIN=\"empathy\"			\
	-DGCR_API_SUBJECT_TO_CHANGE			\
	$(EMPATHY_CFLAGS)				\
	$(TPAW_CFLAGS)					\
	$(GEOCLUE_CFLAGS)				\
	$(GEOCODE_CFLAGS)				\
	$(NETWORK_MANAGER_CFLAGS)			\
	$(CONNMAN_CFLAGS)				\
	$(GOA_CFLAGS)					\
	$(UOA_CFLAGS)					\
	$(WARN_CFLAGS)					\
	$(DISABLE_DEPRECATED)

BUILT_SOURCES = 					\
	empathy-dtd-resources.c				\
	empathy-dtd-resources.h				\
	empathy-enum-types.h				\
	empathy-enum-types.c

libempathy_headers =				\
	action-chain-internal.h			\
	empathy-auth-factory.h			\
	empathy-bus-names.h			\
	empathy-chatroom-manager.h		\
	empathy-chatroom.h			\
	empathy-client-factory.h \
	empathy-connection-aggregator.h		\
	empathy-contact-groups.h		\
	empathy-contact.h			\
	empathy-debug.h				\
	empathy-ft-factory.h			\
	empathy-ft-handler.h			\
	empathy-gsettings.h			\
	empathy-presence-manager.h				\
	empathy-individual-manager.h		\
	empathy-location.h			\
	empathy-message.h			\
	empathy-pkg-kit.h		\
	empathy-request-util.h			\
	empathy-sasl-mechanisms.h		\
	empathy-server-sasl-handler.h		\
	empathy-server-tls-handler.h		\
	empathy-status-presets.h		\
	empathy-tls-verifier.h			\
	empathy-tp-chat.h			\
	empathy-types.h				\
	empathy-utils.h

libempathy_handwritten_source =				\
	$(libempathy_headers)				\
	action-chain.c					\
	empathy-auth-factory.c				\
	empathy-chatroom-manager.c			\
	empathy-chatroom.c				\
	empathy-client-factory.c \
	empathy-connection-aggregator.c		\
	empathy-contact-groups.c			\
	empathy-contact.c				\
	empathy-debug.c					\
	empathy-ft-factory.c				\
	empathy-ft-handler.c				\
	empathy-presence-manager.c					\
	empathy-individual-manager.c			\
	empathy-message.c				\
	empathy-pkg-kit.c		\
	empathy-request-util.c				\
	empathy-sasl-mechanisms.c			\
	empathy-server-sasl-handler.c			\
	empathy-server-tls-handler.c			\
	empathy-status-presets.c			\
	empathy-tls-verifier.c				\
	empathy-tp-chat.c				\
	empathy-utils.c

# these are sources that depend on GOA
goa_sources = \
	empathy-goa-auth-handler.c \
	empathy-goa-auth-handler.h \
	$(NULL)

# these are sources that depend on Ubuntu Online Accounts
uoa_sources = \
	empathy-uoa-auth-handler.c \
	empathy-uoa-auth-handler.h \
	$(NULL)

pkglib_LTLIBRARIES = libempathy.la

# libempathy's API is not stable and will never be, so use -release to make the
# SONAME of the plugin library change with every Empathy release.
libempathy_la_LDFLAGS = \
   -no-undefined \
   -release $(VERSION) \
   $(NULL)

libempathy_la_SOURCES = \
	$(libempathy_handwritten_source) \
	$(NULL)

libempathy_la_LIBADD =		\
	$(top_builddir)/extensions/libemp-extensions.la \
	$(GCR_LIBS) \
	$(EMPATHY_LIBS) \
	$(TPAW_LIBS) \
	$(GEOCLUE_LIBS) \
	$(GEOCODE_LIBS) \
	$(NETWORK_MANAGER_LIBS) \
	$(CONNMAN_LIBS) \
	$(UDEV_LIBS) \
	$(GOA_LIBS) \
	$(UOA_LIBS) \
	$(LIBM)

dtd_resources_files =				\
	empathy-status-presets.dtd		\
	empathy-contact-groups.dtd		\
	empathy-chatroom-manager.dtd		\
	$(NULL)

dtd_resource_files: $(shell $(GLIB_COMPILE_RESOURCES) --sourcedir=$(srcdir) --generate-dependencies $(srcdir)/empathy-dtd.gresource.xml)

empathy-dtd-resources.c: empathy-dtd.gresource.xml $(dtd_resource_files)
	$(AM_V_GEN)$(GLIB_COMPILE_RESOURCES) --target=$@ --sourcedir=$(srcdir) --generate-source $<

empathy-dtd-resources.h: empathy-dtd.gresource.xml $(dtd_resource_files)
	$(AM_V_GEN)$(GLIB_COMPILE_RESOURCES) --target=$@ --sourcedir=$(srcdir) --generate-header $<

EXTRA_DIST = 				\
	empathy-dtd.gresource.xml	\
	$(dtd_resources_files)		\
	$(NULL)

CLEANFILES = 				\
	$(BUILT_SOURCES)		\
	stamp-empathy-enum-types.h

if HAVE_GOA
libempathy_la_SOURCES += $(goa_sources)
else
EXTRA_DIST += $(goa_sources)
endif

if HAVE_UOA
libempathy_la_SOURCES += $(uoa_sources)
else
EXTRA_DIST += $(uoa_sources)
endif

# do not distribute generated files
nodist_libempathy_la_SOURCES =\
	$(BUILT_SOURCES)

check_c_sources = \
    $(libempathy_handwritten_source) \
    $(goa_sources) \
    $(uoa_sources) \
    $(NULL)

include $(top_srcdir)/tools/check-coding-style.mk
check-local: check-coding-style

empathy-enum-types.h: stamp-empathy-enum-types.h
	$(AM_V_GEN)true
stamp-empathy-enum-types.h: $(libempathy_headers) Makefile
	$(AM_V_GEN)(cd $(srcdir) \
	&& glib-mkenums \
			--fhead "#ifndef __LIBEMPATHY_ENUM_TYPES_H__\n" \
			--fhead "#define __LIBEMPATHY_ENUM_TYPES_H__ 1\n\n" \
			--fhead "#include <glib-object.h>\n\n" \
			--fhead "G_BEGIN_DECLS\n\n" \
			--ftail "G_END_DECLS\n\n" \
			--ftail "#endif /* __LIBEMPATHY_ENUM_TYPES_H__ */\n" \
			--fprod '#include "@filename@"\n' \
			--eprod "#define EMPATHY_TYPE_@ENUMSHORT@ @enum_name@_get_type()\n" \
			--eprod "GType @enum_name@_get_type (void);\n" \
		$(libempathy_headers) ) > xgen-gth \
	&& (cmp -s xgen-gth empathy-enum-type.h || cp xgen-gth empathy-enum-types.h) \
	&& rm -f xgen-gth \
	&& echo timestamp > $(@F)

empathy-enum-types.c: $(libempathy_headers) Makefile
	$(AM_V_GEN)(cd $(srcdir) \
	&& glib-mkenums \
			--fhead "#include <config.h>\n" \
			--fhead "#include <glib-object.h>\n" \
			--fhead "#include \"empathy-enum-types.h\"\n\n" \
			--fprod "\n/* enumerations from \"@filename@\" */" \
			--vhead "static const G@Type@Value _@enum_name@_values[] = {" \
			--vprod "  { @VALUENAME@, \"@VALUENAME@\", \"@valuenick@\" }," \
			--vtail "  { 0, NULL, NULL }\n};\n\n" \
			--vtail "GType\n@enum_name@_get_type (void)\n{\n" \
			--vtail "  static GType type = 0;\n\n" \
			--vtail "  if (!type)\n" \
			--vtail "    type = g_@type@_register_static (\"@EnumName@\", _@enum_name@_values);\n\n" \
			--vtail "  return type;\n}\n\n" \
		$(libempathy_headers) ) > xgen-gtc \
	&& cp xgen-gtc $(@F) \
	&& rm -f xgen-gtc
