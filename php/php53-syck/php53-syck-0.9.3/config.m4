dnl $Id: config.m4 264 2007-07-08 19:57:08Z indeyets $
dnl config.m4 for extension syck

PHP_ARG_WITH(syck, for syck support,
[  --with-syck[=DIR]       Include syck support])

if test "$PHP_SYCK" != "no"; then
  # --with-syck -> check with-path
  SEARCH_PATH="/sw/local /sw /usr/local /usr"
  SEARCH_FOR="/include/syck.h"
  if test -r $PHP_SYCK/; then # path given as parameter
    SYCK_DIR=$PHP_SYCK
  else # search default path list
    AC_MSG_CHECKING([for syck files in default path])
    for i in $SEARCH_PATH ; do
      if test -r $i/$SEARCH_FOR; then
        SYCK_DIR=$i
        AC_MSG_RESULT(found in $i)
      fi
    done
  fi

  if test -z "$SYCK_DIR"; then
    AC_MSG_RESULT([not found])
    AC_MSG_ERROR([Please reinstall the syck distribution])
  fi

  # --with-syck -> add include path
  PHP_ADD_INCLUDE($SYCK_DIR/include)

  # --with-syck -> chech for lib and symbol presence
  LIBNAME=syck
  LIBSYMBOL=syck_new_parser

  PHP_CHECK_LIBRARY($LIBNAME,$LIBSYMBOL,
  [
    PHP_ADD_LIBRARY_WITH_PATH($LIBNAME, $SYCK_DIR/lib, SYCK_SHARED_LIBADD)
    AC_DEFINE(HAVE_SYCKLIB,1,[ ])
  ],[
    AC_MSG_ERROR([wrong syck lib version or lib not found])
  ],[
    -L$SYCK_DIR/lib
  ])

  PHP_NEW_EXTENSION(syck, phpext.c, $ext_shared)
  PHP_ADD_EXTENSION_DEP(syck, hash)
  PHP_ADD_EXTENSION_DEP(syck, spl)

  PHP_SUBST(SYCK_SHARED_LIBADD)
fi
