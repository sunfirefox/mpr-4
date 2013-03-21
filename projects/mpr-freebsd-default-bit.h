/*
    bit.h -- Build It Configuration Header for freebsd-x86-default

    This header is created by Bit during configuration. To change settings, re-run
    configure or define variables in your Makefile to override these default values.
 */


/* Settings */
#ifndef BIT_BIT
    #define BIT_BIT "0.8.5"
#endif
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
    #define BIT_DISCOVER "doxygen,dsi,man,man2html,md5,ssl,utest"
#endif
#ifndef BIT_HAS_DOUBLE_BRACES
    #define BIT_HAS_DOUBLE_BRACES 1
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
#ifndef BIT_HAS_MTUNE
    #define BIT_HAS_MTUNE 1
#endif
#ifndef BIT_HAS_PAM
    #define BIT_HAS_PAM 0
#endif
#ifndef BIT_HAS_STACK_PROTECTOR
    #define BIT_HAS_STACK_PROTECTOR 1
#endif
#ifndef BIT_HAS_SYNC
    #define BIT_HAS_SYNC 0
#endif
#ifndef BIT_HAS_SYNC_CAS
    #define BIT_HAS_SYNC_CAS 0
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
#ifndef BIT_MPR_THREAD_STACK
    #define BIT_MPR_THREAD_STACK 0
#endif
#ifndef BIT_MPR_TRACING
    #define BIT_MPR_TRACING 1
#endif
#ifndef BIT_PREFIXES
    #define BIT_PREFIXES "embedthis-prefixes"
#endif
#ifndef BIT_PRODUCT
    #define BIT_PRODUCT "mpr"
#endif
#ifndef BIT_PROJECTS_EST
    #define BIT_PROJECTS_EST 1
#endif
#ifndef BIT_PROJECTS_MATRIXSSL
    #define BIT_PROJECTS_MATRIXSSL 0
#endif
#ifndef BIT_PROJECTS_OPENSSL
    #define BIT_PROJECTS_OPENSSL 0
#endif
#ifndef BIT_PROJECTS_SSL
    #define BIT_PROJECTS_SSL 1
#endif
#ifndef BIT_REQUIRES
    #define BIT_REQUIRES "compiler,lib,link"
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
#ifndef BIT_WARN64TO32
    #define BIT_WARN64TO32 0
#endif
#ifndef BIT_WARN_UNUSED
    #define BIT_WARN_UNUSED 0
#endif
#ifndef BIT_WITHOUT_ALL
    #define BIT_WITHOUT_ALL "doxygen,dsi,man,man2html,pmaker,ssl"
#endif
#ifndef BIT_WITHOUT_DEFAULT
    #define BIT_WITHOUT_DEFAULT "doxygen,dsi,man,man2html,pmaker"
#endif

/* Prefixes */
#ifndef BIT_ROOT_PREFIX
    #define BIT_ROOT_PREFIX "/"
#endif
#ifndef BIT_BASE_PREFIX
    #define BIT_BASE_PREFIX "/usr/local"
#endif
#ifndef BIT_DATA_PREFIX
    #define BIT_DATA_PREFIX "/"
#endif
#ifndef BIT_STATE_PREFIX
    #define BIT_STATE_PREFIX "/var"
#endif
#ifndef BIT_APP_PREFIX
    #define BIT_APP_PREFIX "/usr/local/lib/mpr"
#endif
#ifndef BIT_VAPP_PREFIX
    #define BIT_VAPP_PREFIX "/usr/local/lib/mpr/4.3.0"
#endif
#ifndef BIT_BIN_PREFIX
    #define BIT_BIN_PREFIX "/usr/local/bin"
#endif
#ifndef BIT_INC_PREFIX
    #define BIT_INC_PREFIX "/usr/local/include"
#endif
#ifndef BIT_LIB_PREFIX
    #define BIT_LIB_PREFIX "/usr/local/lib"
#endif
#ifndef BIT_MAN_PREFIX
    #define BIT_MAN_PREFIX "/usr/local/share/man"
#endif
#ifndef BIT_SBIN_PREFIX
    #define BIT_SBIN_PREFIX "/usr/local/sbin"
#endif
#ifndef BIT_ETC_PREFIX
    #define BIT_ETC_PREFIX "/etc/mpr"
#endif
#ifndef BIT_WEB_PREFIX
    #define BIT_WEB_PREFIX "/var/www/mpr-default"
#endif
#ifndef BIT_LOG_PREFIX
    #define BIT_LOG_PREFIX "/var/log/mpr"
#endif
#ifndef BIT_SPOOL_PREFIX
    #define BIT_SPOOL_PREFIX "/var/spool/mpr"
#endif
#ifndef BIT_CACHE_PREFIX
    #define BIT_CACHE_PREFIX "/var/spool/mpr/cache"
#endif
#ifndef BIT_SRC_PREFIX
    #define BIT_SRC_PREFIX "mpr-4.3.0"
#endif

/* Suffixes */
#ifndef BIT_EXE
    #define BIT_EXE ""
#endif
#ifndef BIT_SHLIB
    #define BIT_SHLIB ".so"
#endif
#ifndef BIT_SHOBJ
    #define BIT_SHOBJ ".so"
#endif
#ifndef BIT_LIB
    #define BIT_LIB ".a"
#endif
#ifndef BIT_OBJ
    #define BIT_OBJ ".o"
#endif

/* Profile */
#ifndef BIT_CONFIG_CMD
    #define BIT_CONFIG_CMD "bit -d -q -platform freebsd-x86-default -configure . --without default -gen make"
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
#ifndef BIT_PACK_NANOSSL
    #define BIT_PACK_NANOSSL 0
#endif
#ifndef BIT_PACK_OPENSSL
    #define BIT_PACK_OPENSSL 0
#endif
#ifndef BIT_PACK_PMAKER
    #define BIT_PACK_PMAKER 0
#endif
#ifndef BIT_PACK_SSL
    #define BIT_PACK_SSL 1
#endif
#ifndef BIT_PACK_UTEST
    #define BIT_PACK_UTEST 1
#endif
#ifndef BIT_PACK_VXWORKS
    #define BIT_PACK_VXWORKS 0
#endif
#ifndef BIT_PACK_WINSDK
    #define BIT_PACK_WINSDK 0
#endif
