Multithreaded Portable Runtime (MPR)
====================================

The Multithreaded Portable Runtime provides a high performance, multithreaded, cross-platform abstraction to hide per operating/system dependencies. Implemented in "C", it can be used in both C and C++ programs. It implements an interface that is sufficient to support event based programs and does not try to be a complete cross-platform API.

Key Directories
---------------

* test            - MPR unit tests.
* utils           - Utility programs.


Key Files
---------
* Makefile      - MPR Makefile.
* manager.c     - Source for the Manager (watchdog) program.
* mpr.h         - Primary MPR header.
* async.c       - Windows async select
* atomic.c      - Atomic operations
* buf.c         - The buffer class. Used for expandable ring queues.
* cache.c       - Safe embedded routines including safe string handling.
* cmd.c         - Command execution
* cond.c        - Multithread condition variables
* crypt.c       - Basic cryptography
* disk.c        - Disk file support
* dispatcher.c  - Event dispatchers
* encode.c      - Encode / decoders
* event.c       - Eventing
* file.c        - File handling
* fs.c          - Virtual file system.
* hash.c        - The Hash class. Used for general hash indexing.
* kqueue.c      - Macosx KQueue event queue
* list.c        - List support.
* lock.c        - Multithread locking
* log.c         - MPR logging and debug trace class.
* mem.c         - Memmory allocator
* mime.c        - Mime type support
* mixed.c       - Mixed string / unicode routines
* module.c      - Module loader
* mpr.c         - Mpr general code
* path.c        - File pathname handling
* poll.c        - Linux poll() support
* printf.c      - Enhanced printf
* rom.c         - ROM-based file support
* select.c      - Posix select event queue
* signal.c      - Posix signal handling
* socket.c      - Socket handling
* string.c      - String handling
* test.c        - Unit test harness
* thread.c      - Multithread and thread pool
* time.c        - Time and date handling
* unix.c        - Unix specific code
* vxWorks.c     - Vxworks specific code
* wait.c        - Socket wait handling
* wide.c        - Unicode support
* win.c         - Windows specific code
* wince.c       - Windows CE specific code
* xml.c         - XML parser

--------------------------------------------------------------------------------

Copyright (c) 2003-2012 Embedthis Software, LLC. All Rights Reserved.
Embedthis and AppWeb are trademarks of Embedthis Software, LLC. Other 
brands and their products are trademarks of their respective holders.

See LICENSE.md for software license details.
