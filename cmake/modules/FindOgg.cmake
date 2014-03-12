INCLUDE(FindPackageHandleStandardArgs)

FIND_PATH(OGG_INCLUDE_DIR ogg/ogg.h HINTS $ENV{OGG_DIR})
FIND_LIBRARY(OGG_LIBRARY ogg HINTS $ENV{OGG_DIR})
MARK_AS_ADVANCED(OGG_LIBRARY)

FIND_PACKAGE_HANDLE_STANDARD_ARGS(
  Ogg
  REQUIRED_VARS
  OGG_LIBRARY
  OGG_INCLUDE_DIR
)

