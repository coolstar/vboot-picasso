/* Copyright 2010 The ChromiumOS Authors
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/* Shared code for tests.
 */

#include "2sysincludes.h"

#include "tlcl.h"
#include "tlcl_tests.h"

const char* resilient_startup = NULL;

uint32_t TlclStartupIfNeeded(void) {
	static const char* null_getenv = "some string";  /* a unique address */
	uint32_t result = TlclStartup();
	if (resilient_startup == NULL) {
		resilient_startup = getenv("TLCL_RESILIENT_STARTUP");
		if (resilient_startup == NULL) {
			resilient_startup = null_getenv;
		}
	}
	if (resilient_startup == null_getenv) {
		return result;
	}
	return result == TPM_E_INVALID_POSTINIT ? TPM_SUCCESS : result;
}
