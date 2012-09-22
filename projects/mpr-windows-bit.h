/*
    bit.h -- Build It Configuration Header for windows-x86

    This header is generated by Bit during configuration. To change settings, re-run
    configure or define variables in your Makefile to override these default values.
 */

/* Settings */
#ifndef BIT_ASSERT
    #define BIT_ASSERT 1
#endif
#ifndef BIT_BUILD_NUMBER
    #define BIT_BUILD_NUMBER "0"
#endif
#ifndef BIT_COMPANY
    #define BIT_COMPANY "Embedthis"
#endif
#ifndef BIT_DEPTH
    #define BIT_DEPTH 1
#endif
#ifndef BIT_FLOAT
    #define BIT_FLOAT 1
#endif
#ifndef BIT_HAS_DYN_LOAD
    #define BIT_HAS_DYN_LOAD 1
#endif
#ifndef BIT_HAS_LIB_EDIT
    #define BIT_HAS_LIB_EDIT 0
#endif
#ifndef BIT_HAS_MMU
    #define BIT_HAS_MMU 1
#endif
#ifndef BIT_HAS_UNNAMED_UNIONS
    #define BIT_HAS_UNNAMED_UNIONS 1
#endif
#ifndef BIT_MANAGER
    #define BIT_MANAGER "manager"
#endif
#ifndef BIT_MINIMAL
    #define BIT_MINIMAL "doxygen,dsi,man,man2html,pmaker,ssl"
#endif
#ifndef BIT_OPTIONAL
    #define BIT_OPTIONAL "doxygen,dsi,ejs,http,man,man2html,md5,matrixssl,openssl,ssl,utest"
#endif
#ifndef BIT_PRODUCT
    #define BIT_PRODUCT "mpr"
#endif
#ifndef BIT_REQUIRED
    #define BIT_REQUIRED "winsdk,compiler,link,dumpbin,rc"
#endif
#ifndef BIT_TITLE
    #define BIT_TITLE "Multithreaded Portable Runtime"
#endif
#ifndef BIT_VERSION
    #define BIT_VERSION "4.0.1"
#endif

/* Prefixes */
#ifndef BIT_CFG_PREFIX
    #define BIT_CFG_PREFIX "C:/Program Files (x86)/Multithreaded Portable Runtime"
#endif
#ifndef BIT_BIN_PREFIX
    #define BIT_BIN_PREFIX "C:/Program Files (x86)/Multithreaded Portable Runtime/bin"
#endif
#ifndef BIT_INC_PREFIX
    #define BIT_INC_PREFIX "C:/Program Files (x86)/Multithreaded Portable Runtime/inc"
#endif
#ifndef BIT_LOG_PREFIX
    #define BIT_LOG_PREFIX "C:/Program Files (x86)/Multithreaded Portable Runtime/logs"
#endif
#ifndef BIT_PRD_PREFIX
    #define BIT_PRD_PREFIX "C:/Program Files (x86)/Multithreaded Portable Runtime"
#endif
#ifndef BIT_SPL_PREFIX
    #define BIT_SPL_PREFIX "C:/Program Files (x86)/Multithreaded Portable Runtime/tmp"
#endif
#ifndef BIT_SRC_PREFIX
    #define BIT_SRC_PREFIX "C:/Program Files (x86)/Multithreaded Portable Runtime/src"
#endif
#ifndef BIT_VER_PREFIX
    #define BIT_VER_PREFIX "C:/Program Files (x86)/Multithreaded Portable Runtime"
#endif
#ifndef BIT_WEB_PREFIX
    #define BIT_WEB_PREFIX "C:/Program Files (x86)/Multithreaded Portable Runtime/web"
#endif

/* Suffixes */
#ifndef BIT_EXE
    #define BIT_EXE ".exe"
#endif
#ifndef BIT_SHLIB
    #define BIT_SHLIB ".lib"
#endif
#ifndef BIT_SHOBJ
    #define BIT_SHOBJ ".dll"
#endif
#ifndef BIT_LIB
    #define BIT_LIB ".a"
#endif
#ifndef BIT_OBJ
    #define BIT_OBJ ".obj"
#endif

/* Profile */
#ifndef BIT_CONFIG_CMD
    #define BIT_CONFIG_CMD "bit -d -q -platform windows-x86 -without all -profile vs -configure . -gen vs"
#endif
#ifndef BIT_MPR_PRODUCT
    #define BIT_MPR_PRODUCT 1
#endif
#ifndef BIT_PROFILE
    #define BIT_PROFILE "vs"
#endif

/* Miscellaneous */
#ifndef BIT_MAJOR_VERSION
    #define BIT_MAJOR_VERSION 4
#endif
#ifndef BIT_MINOR_VERSION
    #define BIT_MINOR_VERSION 0
#endif
#ifndef BIT_PATCH_VERSION
    #define BIT_PATCH_VERSION 1
#endif
#ifndef BIT_VNUM
    #define BIT_VNUM 400000001
#endif

/* Packs */
#ifndef BIT_PACK_CC
    #define BIT_PACK_CC 1
#endif
#ifndef BIT_PACK_DOXYGEN
    #define BIT_PACK_DOXYGEN 0
#endif
#ifndef BIT_PACK_DSI
    #define BIT_PACK_DSI 0
#endif
#ifndef BIT_PACK_DUMPBIN
    #define BIT_PACK_DUMPBIN 0
#endif
#ifndef BIT_PACK_EJS
    #define BIT_PACK_EJS 1
#endif
#ifndef BIT_PACK_HTTP
    #define BIT_PACK_HTTP 1
#endif
#ifndef BIT_PACK_LINK
    #define BIT_PACK_LINK 1
#endif
#ifndef BIT_PACK_MAN
    #define BIT_PACK_MAN 0
#endif
#ifndef BIT_PACK_MAN2HTML
    #define BIT_PACK_MAN2HTML 0
#endif
#ifndef BIT_PACK_MATRIXSSL
    #define BIT_PACK_MATRIXSSL 0
#endif
#ifndef BIT_PACK_MD5
    #define BIT_PACK_MD5 1
#endif
#ifndef BIT_PACK_OPENSSL
    #define BIT_PACK_OPENSSL 0
#endif
#ifndef BIT_PACK_PMAKER
    #define BIT_PACK_PMAKER 0
#endif
#ifndef BIT_PACK_RC
    #define BIT_PACK_RC 1
#endif
#ifndef BIT_PACK_SSL
    #define BIT_PACK_SSL 0
#endif
#ifndef BIT_PACK_UTEST
    #define BIT_PACK_UTEST 1
#endif
#ifndef BIT_PACK_WINSDK
    #define BIT_PACK_WINSDK 1
#endif
