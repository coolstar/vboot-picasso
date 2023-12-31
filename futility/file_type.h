/* Copyright 2013 The ChromiumOS Authors
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef VBOOT_REFERENCE_FILE_TYPE_H_
#define VBOOT_REFERENCE_FILE_TYPE_H_

/* What type of things do I know how to handle? */
enum futil_file_type {
	FILE_TYPE_UNKNOWN,
#define FILE_TYPE(A, B, C, D, E, F) FILE_TYPE_ ## A,
#include "file_type.inc"
#undef FILE_TYPE
	NUM_FILE_TYPES
};

/* Short name for file types */
const char *const futil_file_type_name(enum futil_file_type type);

/* Description of file type */
const char *const futil_file_type_desc(enum futil_file_type type);

/* Print the list of type names and exit with the given value. */
void print_file_types_and_exit(int retval);

/* Lookup a type by name. Return true on success */
int futil_str_to_file_type(const char *str, enum futil_file_type *type);

/*
 * This tries to match the buffer content to one of the known file types.
 */
enum futil_file_type futil_file_type_buf(uint8_t *buf, uint32_t len);

/*
 * This opens a file and tries to match it to one of the known file types.
 * It's not an error if it returns FILE_TYPE_UKNOWN.
 */
enum futil_file_err futil_file_type(const char *filename,
				    enum futil_file_type *type);

/*
 * Call the show() method on a file containing a specific file type.
 * Returns zero on success. It's up to the caller to ensure that only valid
 * file types are invoked.
 */
int futil_file_type_show(enum futil_file_type type, const char *filename);

/*
 * Call the sign() method on a file with specific file type.
 * Returns zero on success. It's up to the caller to ensure that only valid
 * file types are invoked.
 */
int futil_file_type_sign(enum futil_file_type type, const char *filename);

/*
 * Declare the file_type functions. Certain functions are reused for more than
 * one file type, leading to redundant declarations here.
 */
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wredundant-decls"
#define R_(FOO) enum futil_file_type FOO(uint8_t *buf, uint32_t len);
#define S_(FOO) int FOO(const char *name, void *data);
#define NONE
#define FILE_TYPE(A, B, C, D, E, F) D E F
#include "file_type.inc"
#undef FILE_TYPE
#undef NONE
#undef SG_
#undef S_
#undef R_
#pragma GCC diagnostic pop

/* Declared for use inside other show functions. */
int show_fw_preamble_buf(const char *name, uint8_t *buf, uint32_t len,
			 void *data);
int show_vb21_pubkey_buf(const char *name, uint8_t *buf, uint32_t len,
			 void *data);

#endif /* VBOOT_REFERENCE_FILE_TYPE_H_ */
