INCLUDE(FindPackageHandleStandardArgs)

IF(NOT LGI_LIBRARIES)
    IF(CMAKE_SIZEOF_VOID_P EQUAL 8) # CG: What the hack
        FIND_LIBRARY(LGI_LIBRARIES
            NAMES corelgilua52.so corelgilua51.so
            HINTS $ENV{LGI_DIR}
            PATH_SUFFIXES lib64/x86_64-linux-gnu/lua/5.3/lgi
                          lib64/lua/5.3/lgi
                          lib64/x86_64-linux-gnu/lua/5.2/lgi
                          lib64/lua/5.2/lgi
                          lib64/x86_64-linux-gnu/lua/5.1/lgi
                          lib64/lua/5.1/lgi
                          lib/x86_64-linux-gnu/lua/5.3/lgi
                          lib/lua/5.3/lgi
                          lib/x86_64-linux-gnu/lua/5.2/lgi
                          lib/lua/5.2/lgi
                          lib/x86_64-linux-gnu/lua/5.1/lgi
                          lib/lua/5.1/lgi
        )
    ELSE()
        FIND_LIBRARY(LGI_LIBRARIES
            NAMES corelgilua52.so corelgilua51.so
            HINTS $ENV{LGI_DIR}
            PATH_SUFFIXES lib/i386-linux-gnu/lua/5.3/lgi
                          lib/lua/5.3/lgi
                          lib/i386-linux-gnu/lua/5.2/lgi
                          lib/lua/5.2/lgi
                          lib/i386-linux-gnu/lua/5.1/lgi
                          lib/lua/5.1/lgi
        )
    ENDIF()
ENDIF()

MARK_AS_ADVANCED(LGI_LIBRARIES)

FIND_PACKAGE_HANDLE_STANDARD_ARGS(
    LGI
    DEFAULT_MSG
    LGI_LIBRARIES
)

