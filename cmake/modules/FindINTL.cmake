INCLUDE(FindPackageHandleStandardArgs)

IF (NOT INTL_INCLUDE_DIR)
    FIND_PATH(INTL_INCLUDE_DIR libintl.h HINTS $ENV{INTL_DIR})
ENDIF()
IF (NOT INTL_LIBRARIES)
    FIND_LIBRARY(INTL_LIBRARIES intl HINTS $ENV{INTL_DIR})
ENDIF()

MARK_AS_ADVANCED(INTL_INCLUDE_DIR)
MARK_AS_ADVANCED(INTL_LIBRARIES)

FIND_PACKAGE_HANDLE_STANDARD_ARGS(
    INTL
    DEFAULT_MSG
    INTL_LIBRARIES
    INTL_INCLUDE_DIR
)

