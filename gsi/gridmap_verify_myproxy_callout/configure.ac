AC_INIT([globus_gridmap_verify_myproxy_callout],[1.5])
AM_INIT_AUTOMAKE
AM_PROG_LIBTOOL

MAJOR_VERSION=${PACKAGE_VERSION%%.*}
MINOR_VERSION=${PACKAGE_VERSION##*.}
AGE_VERSION=1

AC_SUBST(MAJOR_VERSION)
AC_SUBST(MINOR_VERSION)
AC_SUBST(AGE_VERSION)

m4_include([dirt.sh])
AC_SUBST(DIRT_TIMESTAMP)
AC_SUBST(DIRT_BRANCH_ID)

PACKAGE_DEPS="globus-gsi-sysconfig >= 1 globus-gssapi-gsi >= 4 globus-gss-assist >= 3 globus-gridmap-callout-error >= 0 globus-gsi-credential >= 6"
AC_SUBST(PACKAGE_DEPS)

PKG_CHECK_MODULES([PACKAGE_DEP], $PACKAGE_DEPS)

AC_CONFIG_FILES(
        globus-gridmap-verify-myproxy-callout-uninstalled.pc
        globus-gridmap-verify-myproxy-callout.pc
        Makefile
	gridmap_verify_myproxy_callout-gsi_authz.conf
	version.h)
AC_OUTPUT