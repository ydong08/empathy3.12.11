dnl configure-time options for telepathy-account-widgets

dnl TPAW_ARG_VALGRIND

AC_DEFUN([TPAW_ARG_VALGRIND],
[
  dnl valgrind inclusion
  AC_ARG_ENABLE(valgrind,
    AC_HELP_STRING([--enable-valgrind],[enable valgrind checking and run-time detection]),
    [
      case "${enableval}" in
        yes|no) enable="${enableval}" ;;
        *)   AC_MSG_ERROR(bad value ${enableval} for --enable-valgrind) ;;
      esac
    ],
    [enable=no])

  TPAW_VALGRIND($enable, [2.1])
])
