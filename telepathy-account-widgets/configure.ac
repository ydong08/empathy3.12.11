AC_INIT([telepathy-account-widgets],
        [0.1], dnl Version doesn't really make sense for a submodule
        [http://bugzilla.gnome.org/browse.cgi?product=empathy])

AC_PREREQ([2.64])
AC_COPYRIGHT([
  Copyright (C) 2003-2007 Imendio AB
  Copyright (C) 2007-2013 Collabora Ltd.
])

# Minimal version required

# Hard deps
GLIB_REQUIRED=2.33.3
AC_DEFINE(GLIB_VERSION_MIN_REQUIRED, GLIB_VERSION_2_30, [Ignore post 2.30 deprecations])
AC_DEFINE(GLIB_VERSION_MAX_ALLOWED, GLIB_VERSION_2_34, [Prevent post 2.34 APIs])

GTK_REQUIRED=3.5.1
AC_DEFINE(GDK_VERSION_MIN_REQUIRED, GDK_VERSION_3_4, [Ignore post 3.4 deprecations])
AC_DEFINE(GDK_VERSION_MAX_ALLOWED, GDK_VERSION_3_4, [Prevent post 3.4 APIs])

TELEPATHY_GLIB_REQUIRED=0.22.0
AC_DEFINE(TP_VERSION_MIN_REQUIRED, TP_VERSION_0_22, [Ignore post 0.22 deprecations])
AC_DEFINE(TP_VERSION_MAX_ALLOWED, TP_VERSION_0_22, [Prevent post 0.22 APIs])
AC_DEFINE(TP_SEAL_ENABLE, 1, [Prevent to use sealed variables])
AC_DEFINE(TP_DISABLE_SINGLE_INCLUDE, 1, [Disable single include header])

LIBSECRET_REQUIRED=0.5

# Optional deps
LIBACCOUNTS_REQUIRED=1.4
LIBSIGNON_REQUIRED=1.8
MC_PLUGINS_REQUIRED=5.13.1

# Enable extra flags for warnings if this is an unreleased version.
AC_ARG_ENABLE(unreleased_checks,
            AS_HELP_STRING([--enable-unreleased-checks=@<:@no/yes@:>@],
            [enable extra checks for unreleased versions]),,
            [enable_unreleased_checks=no])

# Use --enable-maintainer-mode to disable deprecated symbols,
# disable single include and enable GSEAL.
# If this is not a released tp-account-widgets, maintainer mode is forced.
# FIXME: don't disable deprecated symbols until bgo #636654 has been fixed
#if test "x$enable_unreleased_checks" = "xyes" ; then
#    enable_maintainer_mode="yes"
#fi
#GNOME_MAINTAINER_MODE_DEFINES

AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_SRCDIR([configure.ac])
AC_CONFIG_AUX_DIR(.)

AM_INIT_AUTOMAKE([1.11 no-define -Wno-portability tar-ustar no-dist-gzip dist-xz])

# Support silent build rules. Disable
# by either passing --disable-silent-rules to configure or passing V=1
# to make
AM_SILENT_RULES([yes])

# Check for programs
AC_PROG_CC
AC_HEADER_STDC
AC_PROG_MKDIR_P
AM_PATH_GLIB_2_0
AC_PATH_XTRA

# Initialize libtool
LT_PREREQ([2.2])
LT_INIT

GLIB_GSETTINGS

GLIB_COMPILE_RESOURCES=`$PKG_CONFIG gio-2.0 --variable=glib_compile_resources`
AC_SUBST(GLIB_COMPILE_RESOURCES)
GLIB_GENMARSHAL=`$PKG_CONFIG glib-2.0 --variable=glib_genmarshal`
AC_SUBST(GLIB_GENMARSHAL)

TPAW_ARG_VALGRIND


# -----------------------------------------------------------
# Error flags
# -----------------------------------------------------------
AS_COMPILER_FLAG(-Wall, ERROR_CFLAGS="-Wall", ERROR_CFLAGS="")
AS_COMPILER_FLAG(-Werror, werror=yes, werror=no)

AC_ARG_ENABLE(Werror,
  AS_HELP_STRING([--disable-Werror],
                 [compile without -Werror (normally enabled in development builds)]),
    werror=$enableval, :)

AS_COMPILER_FLAG(-Wextra, wextra=yes, wextra=no)
AS_COMPILER_FLAG(-Wno-missing-field-initializers,
        wno_missing_field_initializers=yes,
        wno_missing_field_initializers=no)
AS_COMPILER_FLAG(-Wno-unused-parameter,
        wno_unused_parameter=yes,
        wno_unused_parameter=no)

if test "x$enable_unreleased_checks" = "xyes"; then
    if test x$werror = xyes; then
        ERROR_CFLAGS="$ERROR_CFLAGS -Werror"
    fi
    if test x$wextra = xyes -a \
        x$wno_missing_field_initializers = xyes -a \
        x$wno_unused_parameter = xyes; then
        ERROR_CFLAGS="$ERROR_CFLAGS -Wextra -Wno-missing-field-initializers -Wno-unused-parameter"
    fi
fi

AS_COMPILER_FLAG(-Wdeclaration-after-statement, ERROR_CFLAGS="$ERROR_CFLAGS -Wdeclaration-after-statement")
AS_COMPILER_FLAG(-Wshadow, ERROR_CFLAGS="$ERROR_CFLAGS -Wshadow")
AS_COMPILER_FLAG(-Wmissing-prototypes, ERROR_CFLAGS="$ERROR_CFLAGS -Wmissing-prototypes")
AS_COMPILER_FLAG(-Wmissing-declarations, ERROR_CFLAGS="$ERROR_CFLAGS -Wmissing-declarations")
AS_COMPILER_FLAG(-Wstrict-prototypes, ERROR_CFLAGS="$ERROR_CFLAGS -Wstrict-prototypes")

AC_SUBST(ERROR_CFLAGS)

AC_ARG_ENABLE(gprof,
            AS_HELP_STRING([--enable-gprof=@<:@no/yes@:>@],
            [build with support for gprof]),,
            [enable_gprof=no])
if test "x$enable_gprof" != "xno" ; then
    CFLAGS="$CFLAGS -pg"
    LDFLAGS="$LDFLAGS -pg"
fi

AC_SUBST(LDFLAGS)

# -----------------------------------------------------------
# Allow to overwrite where to install data files
# -----------------------------------------------------------
AC_ARG_WITH([pkgdatadir],
            AS_HELP_STRING([--with-pkgdatadir],
                           [set a different directory where to install data files]),,
            with_pkgdatadir=)

if test x"$with_pkgdatadir" != x"" ; then
    real_pkgdatadir="${with_pkgdatadir}"
else
    real_pkgdatadir="${datarootdir}/${PACKAGE_NAME}"
fi
AC_SUBST(real_pkgdatadir)

AC_ARG_WITH([icondir],
            AS_HELP_STRING([--with-icondir],
                           [set a different directory where to install icons]),,
            with_icondir=)

if test x"$with_icondir" != x"" ; then
    icondir="${with_icondir}"
else
    icondir="${real_pkgdatadir}/icons"
fi
AC_SUBST(icondir)

# -----------------------------------------------------------
# Disable the installation of tp-aw's GSettings schema.
# This can be needed to avoid multiple components shipping
# the same schema.
# See https://bugzilla.gnome.org/show_bug.cgi?id=706801
# -----------------------------------------------------------
AC_ARG_ENABLE(settings,
  AS_HELP_STRING([--disable-settings],
                 [compile without installing GSettings schema]),
  enable_settings=$enableval, enable_settings=yes )

if test x$enable_settings = xyes; then
  AC_DEFINE(ENABLE_SETTINGS, [], [Enable GSettings schema])
fi

AM_CONDITIONAL(ENABLE_SETTINGS, test "x$enable_settings" = "xyes")

# -----------------------------------------------------------
# Pkg-Config dependency checks
# -----------------------------------------------------------

PKG_CHECK_MODULES(TPAW,
[
   dbus-glib-1
   glib-2.0 >= $GLIB_REQUIRED
   gio-2.0 >= $GLIB_REQUIRED
   libsecret-1 >= $LIBSECRET_REQUIRED
   gobject-2.0
   libxml-2.0
   telepathy-glib >= $TELEPATHY_GLIB_REQUIRED
   x11
   gtk+-3.0 >= $GTK_REQUIRED
])

# -----------------------------------------------------------
# UOA configuration bits
# -----------------------------------------------------------
TPAW_UOA_PROVIDER="im.telepathy.Account.Storage.UOA"
AC_DEFINE_UNQUOTED(TPAW_UOA_PROVIDER, "$TPAW_UOA_PROVIDER",
   [Name of provider for accounts imported from libaccounts])
AC_SUBST(TPAW_UOA_PROVIDER)

# -----------------------------------------------------------
# Enable debug
# -----------------------------------------------------------

AC_ARG_ENABLE(debug,
  AS_HELP_STRING([--disable-debug],
                 [compile without debug code]),
    enable_debug=$enableval, enable_debug=yes )

if test x$enable_debug = xyes; then
  AC_DEFINE(ENABLE_DEBUG, [], [Enable debug code])
fi

# -----------------------------------------------------------
# Language Support
# -----------------------------------------------------------
IT_PROG_INTLTOOL([0.50.0])

AC_ARG_WITH([gettext-package],
            AS_HELP_STRING([--with-gettext-package],
                           [set a different GETTEXT_PACKAGE to avoid conflicts]),,
            with_gettext_package=)

if test -n "$with_gettext_package"; then
    GETTEXT_PACKAGE="$with_gettext_package"
else
    GETTEXT_PACKAGE=tpaw
fi
AC_SUBST(GETTEXT_PACKAGE)
AC_DEFINE_UNQUOTED(GETTEXT_PACKAGE,"$GETTEXT_PACKAGE",[Gettext package name])

# -----------------------------------------------------------
# gudev
# -----------------------------------------------------------
AC_ARG_ENABLE(gudev,
              AS_HELP_STRING([--enable-gudev=@<:@no/yes/auto@:>@],
                             [build with gudev support]), ,
                             enable_gudev=auto)

if test "x$enable_gudev" != "xno"; then

   PKG_CHECK_MODULES(UDEV, [gudev-1.0],
        have_gudev="yes", have_gudev="no")

   if test "x$have_gudev" = "xyes"; then
      AC_DEFINE(HAVE_UDEV, 1, [Define if you have gudev])
   fi
else
   have_gudev=no
fi

if test "x$enable_gudev" = "xyes" -a "x$have_gudev" != "xyes"; then
   AC_MSG_ERROR([Could not find gudev dependencies:

$UDEV_PKG_ERRORS])
fi
AM_CONDITIONAL(HAVE_UDEV, test "x$have_gudev" = "xyes")

if test x${have_gudev} = x"yes" ; then
  REQUIRE_UDEV=", gudev-1.0"
else
  REQUIRE_UDEV=""
fi
AC_SUBST(REQUIRE_UDEV)

# -----------------------------------------------------------
# Cheese (optional dependency for avatar selection)
# -----------------------------------------------------------
AC_ARG_WITH([cheese],
            AS_HELP_STRING([--with-cheese],
                           [enable cheese webcam support]),,
            with_cheese=auto)

if test x"$with_cheese" != x"no" ; then
   PKG_CHECK_MODULES(CHEESE, cheese-gtk >= CHEESE_GTK_REQUIRED, [have_cheese=yes], [have_cheese=no])
   if test x${have_cheese} = xyes; then
     AC_DEFINE(HAVE_CHEESE, 1, [Define to 1 to enable cheese webcam support])
   fi
   if test x${with_cheese} = xyes && test x${have_cheese} = xno; then
     AC_MSG_ERROR([Cheese configured but not found:

$CHEESE_PKG_ERRORS])
   fi
else
   have_cheese=no
fi
AM_CONDITIONAL(BUILD_CHEESE, test x${have_cheese} = xyes)

if test x${have_cheese} = x"yes" ; then
  REQUIRE_CHEESE=", cheese-gtk"
else
  REQUIRE_CHEESE=""
fi
AC_SUBST(REQUIRE_CHEESE)

# -----------------------------------------------------------
# ubuntu-online-accounts support
# -----------------------------------------------------------
AC_ARG_ENABLE(ubuntu-online-accounts,
              AS_HELP_STRING([--enable-ubuntu-online-accounts=@<:@no/yes/auto@:>@],
                             [build Ubuntu Online Accounts plugins]), ,
                             enable_ubuntu_online_accounts=auto)

if test "x$enable_ubuntu_online_accounts" != "xno"; then
   PKG_CHECK_MODULES(UOA,
    [
       account-plugin
       mission-control-plugins >= $MC_PLUGINS_REQUIRED
       libaccounts-glib >= $LIBACCOUNTS_REQUIRED
       libsignon-glib >= $LIBSIGNON_REQUIRED
    ], have_uoa="yes", have_uoa="no")

   # provider plugin dir
   AC_MSG_CHECKING([Accounts provider plugin dir])
   ACCOUNTS_PROVIDER_PLUGIN_DIR=`pkg-config --variable=provider_plugindir account-plugin`

   AC_MSG_RESULT([$ACCOUNTS_PROVIDER_PLUGIN_DIR])
   AC_SUBST(ACCOUNTS_PROVIDER_PLUGIN_DIR)

   # app plugin dir
   AC_MSG_CHECKING([Accounts provider app plugin dir])
   ACCOUNTS_APP_PLUGIN_DIR=`pkg-config --variable=application_plugindir account-plugin`

   AC_MSG_RESULT([$ACCOUNTS_APP_PLUGIN_DIR])
   AC_SUBST(ACCOUNTS_APP_PLUGIN_DIR)

   # provider files dir
   AC_MSG_CHECKING([Accounts provider files dir])
   ACCOUNTS_PROVIDER_FILES_DIR=`pkg-config --variable=providerfilesdir libaccounts-glib`

   AC_MSG_RESULT([$ACCOUNTS_PROVIDER_FILES_DIR])
   AC_SUBST(ACCOUNTS_PROVIDER_FILES_DIR)

   # service files dir
   AC_MSG_CHECKING([Accounts service files dir])
   ACCOUNTS_SERVICE_FILES_DIR=`pkg-config --variable=servicefilesdir libaccounts-glib`

   AC_MSG_RESULT([$ACCOUNTS_SERVICE_FILES_DIR])
   AC_SUBST(ACCOUNTS_SERVICE_FILES_DIR)

   # application files dir
   AC_MSG_CHECKING([Accounts applications files dir])
   ACCOUNTS_APPLICATION_FILES_DIR=`pkg-config --variable=applicationfilesdir libaccounts-glib`

   AC_MSG_RESULT([$ACCOUNTS_APPLICATION_FILES_DIR])
   AC_SUBST(ACCOUNTS_APPLICATION_FILES_DIR)

else
   have_uoa=no
fi

if test "x$enable_ubuntu_online_accounts" = "xyes" -a "x$have_uoa" != "xyes"; then
   AC_MSG_ERROR([Could not find Ubuntu Online Accounts dependencies:

$UOA_PKG_ERRORS])
fi

if test "x$have_uoa" = "xyes"; then
   AC_DEFINE(HAVE_UOA, 1, [Define to 1 to build Ubuntu Online Accounts plugins])
fi

AM_CONDITIONAL(HAVE_UOA, test "x$have_uoa" = "xyes")

if test "x$have_uoa" = "xyes" -o "x$have_goa" = "xyes"; then
   AC_MSG_CHECKING([Mission Control plugins dir])
   MISSION_CONTROL_PLUGINS_DIR=${libdir}/mission-control-plugins.`pkg-config --variable=MCP_ABI_VERSION mission-control-plugins`

   AC_MSG_RESULT([$MISSION_CONTROL_PLUGINS_DIR])
   AC_SUBST(MISSION_CONTROL_PLUGINS_DIR)
fi

# -----------------------------------------------------------
# Coding style checks
# -----------------------------------------------------------
AC_ARG_ENABLE(coding-style-checks,
  AS_HELP_STRING([--disable-coding-style-checks],
                 [do not check coding style using grep]),
    [ENABLE_CODING_STYLE_CHECKS=$enableval],
    [ENABLE_CODING_STYLE_CHECKS=yes])

AC_SUBST([ENABLE_CODING_STYLE_CHECKS])
# -----------------------------------------------------------

AC_CONFIG_FILES([
   Makefile
   telepathy-account-widgets-uninstalled.pc
   data/Makefile
   data/icons/Makefile
   po/Makefile.in
   tp-account-widgets/Makefile
   tools/Makefile
])
AC_OUTPUT

echo "
Configure summary:

	Compiler....................:  ${CC}
	Compiler Flags..............:  ${CFLAGS} ${ERROR_CFLAGS}
	Linker Flags................:  ${LDFLAGS}
	Prefix......................:  ${prefix}
	Data directory .............:  ${real_pkgdatadir}
	Icon directory .............:  ${icondir}
	GSettings schema support....:  ${enable_settings}
	Coding style checks.........:  ${ENABLE_CODING_STYLE_CHECKS}
	Unreleased version checks...:  ${enable_unreleased_checks}
	GETTEXT_PACKAGE.............:  ${GETTEXT_PACKAGE}

    Features:
	Cheese webcam support ......:  ${have_cheese}
	Camera monitoring...........:  ${have_gudev}
	Ubuntu Online plugins.......:  ${have_uoa}
"
