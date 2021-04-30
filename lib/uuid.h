#ifndef _UUID_H
#define _UUID_H

#include <stdint.h>

/* A Universally Unique IDentifier (UUID) compliant with RFC 4122.
 *
 * Each of the parts is stored in host byte order, but the parts themselves are
 * ordered from left to right.  That is, (parts[0] >> 24) is the first 8 bits
 * of the UUID when output in the standard form, and (parts[3] & 0xff) is the
 * final 8 bits. */
struct uuid {
    uint32_t parts[4];
};

#endif
