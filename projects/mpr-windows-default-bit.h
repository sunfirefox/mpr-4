/*
    bit.h -- Build It Configuration Header for windows-x86-default

    This header is generated by Bit during configuration. To change settings, re-run
    configure or define variables in your Makefile to override these default values.
 */


/* Settings */
#ifndef BIT_BUILD_NUMBER
    #define BIT_BUILD_NUMBER "0"
#endif
#ifndef BIT_COMPANY
    #define BIT_COMPANY "Embedthis"
#endif
#ifndef BIT_DEBUG
    #define BIT_DEBUG 1
#endif
#ifndef BIT_DEPTH
    #define BIT_DEPTH 1
#endif
#ifndef BIT_DISCOVER
    #define BIT_DISCOVER "doxygen,dsi,est,man,man2html,md5,utest"
#endif
#ifndef BIT_HAS_DYN_LOAD
    #define BIT_HAS_DYN_LOAD 1
#endif
#ifndef BIT_HAS_LIB_EDIT
    #define BIT_HAS_LIB_EDIT 0
#endif
#ifndef BIT_HAS_LIB_RT
    #define BIT_HAS_LIB_RT 0
#endif
#ifndef BIT_HAS_MMU
    #define BIT_HAS_MMU 1
#endif
#ifndef BIT_HAS_UNNAMED_UNIONS
    #define BIT_HAS_UNNAMED_UNIONS 1
#endif
#ifndef BIT_MPR_LOGGING
    #define BIT_MPR_LOGGING 1
#endif
#ifndef BIT_MPR_MANAGER
    #define BIT_MPR_MANAGER "manager"
#endif
#ifndef BIT_MPR_TRACING
    #define BIT_MPR_TRACING 1
#endif
#ifndef BIT_PRODUCT
    #define BIT_PRODUCT "mpr"
#endif
#ifndef BIT_REQUIRED
    #define BIT_REQUIRED "winsdk,compiler,lib,link,dumpbin,rc"
#endif
#ifndef BIT_SSL
    #define BIT_SSL 1
#endif
#ifndef BIT_SYNC
    #define BIT_SYNC "bitos,est"
#endif
#ifndef BIT_TITLE
    #define BIT_TITLE "Multithreaded Portable Runtime"
#endif
#ifndef BIT_VERSION
    #define BIT_VERSION "4.3.0"
#endif
#ifndef BIT_WITHOUT_ALL
    #define BIT_WITHOUT_ALL "doxygen,dsi,est,man,man2html,pmaker"
#endif
#ifndef BIT_WITHOUT_DEFAULT
    #define BIT_WITHOUT_DEFAULT "doxygen,dsi,man,man2html,pmaker"
#endif

/* Prefixes */
#ifndef BIT_CFG_PREFIX
    #define BIT_CFG_PREFIX "C:/Program Files/Multithreaded Portable Runtime"
#endif
#ifndef BIT_BIN_PREFIX
    #define BIT_BIN_PREFIX "C:/Program Files/Multithreaded Portable Runtime/bin"
#endif
#ifndef BIT_INC_PREFIX
    #define BIT_INC_PREFIX "C:/Program Files/Multithreaded Portable Runtime/inc"
#endif
#ifndef BIT_LOG_PREFIX
    #define BIT_LOG_PREFIX "C:/Program Files/Multithreaded Portable Runtime/log"
#endif
#ifndef BIT_PRD_PREFIX
    #define BIT_PRD_PREFIX "C:/Program Files/Multithreaded Portable Runtime"
#endif
#ifndef BIT_SPL_PREFIX
    #define BIT_SPL_PREFIX "C:/Program Files/Multithreaded Portable Runtime/tmp"
#endif
#ifndef BIT_SRC_PREFIX
    #define BIT_SRC_PREFIX "C:/Program Files/Multithreaded Portable Runtime/src"
#endif
#ifndef BIT_VER_PREFIX
    #define BIT_VER_PREFIX "C:/Program Files/Multithreaded Portable Runtime"
#endif
#ifndef BIT_WEB_PREFIX
    #define BIT_WEB_PREFIX "C:/Program Files/Multithreaded Portable Runtime/web"
#endif
#ifndef BIT_UBIN_PREFIX
    #define BIT_UBIN_PREFIX "C:/Program Files/Multithreaded Portable Runtime/bin"
#endif
#ifndef BIT_MAN_PREFIX
    #define BIT_MAN_PREFIX "C:/Program Files/Multithreaded Portable Runtime/man"
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
    #define BIT_LIB ".lib"
#endif
#ifndef BIT_OBJ
    #define BIT_OBJ ".obj"
#endif

/* Profile */
#ifndef BIT_CONFIG_CMD
    #define BIT_CONFIG_CMD "bit -d -q -platform windows-x86-default --without default -configure . -gen vs"
#endif
#ifndef BIT_MPR_PRODUCT
    #define BIT_MPR_PRODUCT 1
#endif
#ifndef BIT_PROFILE
    #define BIT_PROFILE "default"
#endif

/* Miscellaneous */
#ifndef BIT_MAJOR_VERSION
    #define BIT_MAJOR_VERSION 4
#endif
#ifndef BIT_MINOR_VERSION
    #define BIT_MINOR_VERSION 3
#endif
#ifndef BIT_PATCH_VERSION
    #define BIT_PATCH_VERSION 0
#endif
#ifndef BIT_VNUM
    #define BIT_VNUM 400030000
#endif

/* Packs */
#ifndef BIT_PACK_CC
    #define BIT_PACK_CC 1
#endif
#ifndef BIT_PACK_DEFAULT
    #define BIT_PACK_DEFAULT 0
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
#ifndef BIT_PACK_EST
    #define BIT_PACK_EST 1
#endif
#ifndef BIT_PACK_LIB
    #define BIT_PACK_LIB 1
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
#ifndef BIT_PACK_MOCANA
    #define BIT_PACK_MOCANA 0
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
#ifndef BIT_PACK_UTEST
    #define BIT_PACK_UTEST 1
#endif
#ifndef BIT_PACK_WINSDK
    #define BIT_PACK_WINSDK 1
#endif
#ifndef BIT_PACK_COMPILER_PATH
    #define BIT_PACK_COMPILER_PATH "cl.exe"
#endif
#ifndef BIT_PACK_EST_PATH
    #define BIT_PACK_EST_PATH "/Users/mob/git/mpr/src/deps/est"
#endif
#ifndef BIT_PACK_LIB_PATH
    #define BIT_PACK_LIB_PATH "lib.exe"
#endif
#ifndef BIT_PACK_LINK_PATH
    #define BIT_PACK_LINK_PATH "link.exe"
#endif
#ifndef BIT_PACK_MD5_PATH
    #define BIT_PACK_MD5_PATH "/sbin/md5"
#endif
#ifndef BIT_PACK_RC_PATH
    #define BIT_PACK_RC_PATH "rc.exe"
#endif
#ifndef BIT_PACK_UTEST_PATH
    #define BIT_PACK_UTEST_PATH "/Users/mob/git/ejs/macosx-x64-debug/bin/utest"
#endif
#ifndef BIT_PACK_WINSDK_PATH
    #define BIT_PACK_WINSDK_PATH "$(SDK)"
#endif
