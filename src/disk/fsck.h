#ifndef FSCK_H
#define FSCK_H

/* fsck 1.41.4 (27-Jan-2009) manpage says:
 * 0   - No errors
 * 1   - File system errors corrected
 * 2   - System should be rebooted
 * 4   - File system errors left uncorrected
 * 8   - Operational error
 * 16  - Usage or syntax error
 * 32  - Fsck canceled by user request
 * 128 - Shared library error
 */
#define FSCK_OK          0
#define FSCK_NONDESTRUCT 1
#define FSCK_DESTRUCT    2
#define FSCK_UNCORRECTED 4
#define FSCK_ERROR       8
#define FSCK_USAGE       16
#define FSCK_CANCELED    32     /* Aborted with a signal or ^C */

#endif
