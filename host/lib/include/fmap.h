/* Copyright 2011 The ChromiumOS Authors
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef VBOOT_REFERENCE_FMAP_H_
#define VBOOT_REFERENCE_FMAP_H_

#include <inttypes.h>
#include <stddef.h>

/* FMAP structs. See http://code.google.com/p/flashmap/wiki/FmapSpec */
#define FMAP_NAMELEN 32
#define FMAP_SIGNATURE "__FMAP__"
#define FMAP_SIGNATURE_SIZE 8
#define FMAP_SEARCH_STRIDE 4
#define FMAP_VER_MAJOR 1
typedef struct _FmapHeader {
	/* Avoid endian issues in signature... */
	char        fmap_signature[FMAP_SIGNATURE_SIZE];
	uint8_t     fmap_ver_major;
	uint8_t     fmap_ver_minor;
	uint64_t    fmap_base;
	uint32_t    fmap_size;
	char        fmap_name[FMAP_NAMELEN];
	uint16_t    fmap_nareas;
} __attribute__((packed)) FmapHeader;

enum fmap_flags {
	FMAP_AREA_STATIC	= 1 << 0,
	FMAP_AREA_COMPRESSED	= 1 << 1,
	FMAP_AREA_RO		= 1 << 2,
	/* Should be preserved on update or rollback. */
	FMAP_AREA_PRESERVE	= 1 << 3,
};

typedef struct _FmapAreaHeader {
	uint32_t area_offset;
	uint32_t area_size;
	char     area_name[FMAP_NAMELEN];
	uint16_t area_flags;
} __attribute__((packed)) FmapAreaHeader;


/* Find and point to the FMAP header within the buffer */
FmapHeader *fmap_find(uint8_t *ptr, size_t size);

/* Search for an area by name, return pointer to its beginning */
uint8_t *fmap_find_by_name(uint8_t *ptr, size_t size,
			   /* optional, will call fmap_find() if NULL */
			   FmapHeader *fmap,
			   /* The area name to search for */
			   const char *name,
			   /* optional, return pointer to entry if not NULL */
			   FmapAreaHeader **ah);

#endif  /* VBOOT_REFERENCE_FMAP_H_ */
