#ifndef _CONFIG_H
#define _CONFIG_H

#cmakedefine HAVE_CRYPT_H 1
#cmakedefine HAVE_LOCALE_H 1

#cmakedefine ENABLE_NLS 1

#cmakedefine GETTEXT_PACKAGE ""

#cmakedefine PACKAGE "${APPLICATION_NAME}"
#cmakedefine PACKAGE_VERSION "${APPLICATION_VERSION}"
#cmakedefine VERSION "${APPLICATION_VERSION}"
#cmakedefine LOCALEDIR "${LOCALE_INSTALL_DIR}"
#cmakedefine DATADIR "${SHARE_INSTALL_PREFIX}"
#cmakedefine LIBDIR "${LIB_INSTALL_DIR}"

#endif /* _CONFIG_H */
