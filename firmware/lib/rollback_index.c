/* Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Functions for querying, manipulating and locking rollback indices
 * stored in the TPM NVRAM.
 */

#include "2sysincludes.h"
#include "2common.h"
#include "2crc8.h"
#include "rollback_index.h"
#include "tlcl.h"
#include "tss_constants.h"
#include "vboot_api.h"

#ifndef offsetof
#define offsetof(A,B) __builtin_offsetof(A,B)
#endif

/*
 * Provide protoypes for functions not in the header file. These prototypes
 * fix -Wmissing-prototypes warnings.
 */
uint32_t ReadSpaceFirmware(RollbackSpaceFirmware *rsf);
uint32_t WriteSpaceFirmware(RollbackSpaceFirmware *rsf);
uint32_t ReadSpaceKernel(RollbackSpaceKernel *rsk);
uint32_t WriteSpaceKernel(RollbackSpaceKernel *rsk);

#ifdef FOR_TEST
/*
 * Compiling for unit test, so we need the real implementations of
 * rollback functions.  The unit test mocks the underlying tlcl
 * functions, so this is ok to run on the host.
 */
#undef CHROMEOS_ENVIRONMENT
#undef DISABLE_ROLLBACK_TPM
#endif

#define RETURN_ON_FAILURE(tpm_command) do {				\
		uint32_t result_;					\
		if ((result_ = (tpm_command)) != TPM_SUCCESS) {		\
			VB2_DEBUG("Rollback: %08x returned by " #tpm_command \
				  "\n", (int)result_);			\
			return result_;					\
		}							\
	} while (0)


uint32_t TPMClearAndReenable(void)
{
	VB2_DEBUG("TPM: Clear and re-enable\n");
	RETURN_ON_FAILURE(TlclForceClear());
	RETURN_ON_FAILURE(TlclSetEnable());
	RETURN_ON_FAILURE(TlclSetDeactivated(0));

	return TPM_SUCCESS;
}

uint32_t SafeWrite(uint32_t index, const void *data, uint32_t length)
{
	uint32_t result = TlclWrite(index, data, length);
	if (result == TPM_E_MAXNVWRITES) {
		RETURN_ON_FAILURE(TPMClearAndReenable());
		return TlclWrite(index, data, length);
	} else {
		return result;
	}
}

/* Functions to read and write firmware and kernel spaces. */
uint32_t ReadSpaceFirmware(RollbackSpaceFirmware *rsf)
{
	uint32_t r;

	r = TlclRead(FIRMWARE_NV_INDEX, rsf, sizeof(RollbackSpaceFirmware));
	if (TPM_SUCCESS != r)
		return r;

	/*
	 * No CRC in this version, so we'll create one when we write
	 * it. Note that we're marking this as version 2, not
	 * ROLLBACK_SPACE_FIRMWARE_VERSION, because version 2 just
	 * added the CRC. Later versions will need to set default
	 * values for any extra fields explicitly (probably here).
	 */
	if (rsf->struct_version < 2) {
		/* Danger Will Robinson! Danger! */
		rsf->struct_version = 2;
		return TPM_SUCCESS;
	}

	if (rsf->crc8 != vb2_crc8(rsf, offsetof(RollbackSpaceFirmware, crc8))) {
		VB2_DEBUG("TPM: bad CRC\n");
		return TPM_E_CORRUPTED_STATE;
	}

	return TPM_SUCCESS;
}

uint32_t WriteSpaceFirmware(RollbackSpaceFirmware *rsf)
{
	/* All writes should use struct_version 2 or greater. */
	if (rsf->struct_version < 2)
		rsf->struct_version = 2;
	rsf->crc8 = vb2_crc8(rsf, offsetof(RollbackSpaceFirmware, crc8));

	return SafeWrite(FIRMWARE_NV_INDEX, rsf, sizeof(RollbackSpaceFirmware));
}

vb2_error_t SetVirtualDevMode(int val)
{
	RollbackSpaceFirmware rsf;

	VB2_DEBUG("TPM: Entering\n");
	if (TPM_SUCCESS != ReadSpaceFirmware(&rsf))
		return VBERROR_TPM_FIRMWARE_SETUP;

	VB2_DEBUG("TPM: flags were 0x%02x\n", rsf.flags);
	if (val)
		rsf.flags |= FLAG_VIRTUAL_DEV_MODE_ON;
	else
		rsf.flags &= ~FLAG_VIRTUAL_DEV_MODE_ON;
	/*
	 * NOTE: This doesn't update the FLAG_LAST_BOOT_DEVELOPER bit.  That
	 * will be done on the next boot.
	 */
	VB2_DEBUG("TPM: flags are now 0x%02x\n", rsf.flags);

	if (TPM_SUCCESS != WriteSpaceFirmware(&rsf))
		return VBERROR_TPM_SET_BOOT_MODE_STATE;

	VB2_DEBUG("TPM: Leaving\n");
	return VB2_SUCCESS;
}

uint32_t ReadSpaceKernel(RollbackSpaceKernel *rsk)
{
	uint32_t r;

	r = TlclRead(KERNEL_NV_INDEX, rsk, sizeof(RollbackSpaceKernel));
	if (TPM_SUCCESS != r)
		return r;

	/*
	 * No CRC in this version, so we'll create one when we write
	 * it. Note that we're marking this as version 2, not
	 * ROLLBACK_SPACE_KERNEL_VERSION, because version 2 just added
	 * the CRC. Later versions will need to set default values for
	 * any extra fields explicitly (probably here).
	 */
	if (rsk->struct_version < 2) {
		/* Danger Will Robinson! Danger! */
		rsk->struct_version = 2;
		return TPM_SUCCESS;
	}

	if (rsk->crc8 != vb2_crc8(rsk, offsetof(RollbackSpaceKernel, crc8))) {
		VB2_DEBUG("TPM: bad CRC\n");
		return TPM_E_CORRUPTED_STATE;
	}

	return TPM_SUCCESS;
}

uint32_t WriteSpaceKernel(RollbackSpaceKernel *rsk)
{
	/* All writes should use struct_version 2 or greater. */
	if (rsk->struct_version < 2)
		rsk->struct_version = 2;
	rsk->crc8 = vb2_crc8(rsk, offsetof(RollbackSpaceKernel, crc8));

	return SafeWrite(KERNEL_NV_INDEX, rsk, sizeof(RollbackSpaceKernel));
}

#ifdef DISABLE_ROLLBACK_TPM
/* Dummy implementations which don't support TPM rollback protection */

uint32_t RollbackKernelRead(uint32_t* version)
{
	*version = 0;
	return TPM_SUCCESS;
}

uint32_t RollbackKernelWrite(uint32_t version)
{
	return TPM_SUCCESS;
}

uint32_t RollbackKernelLock(int recovery_mode)
{
	return TPM_SUCCESS;
}

uint32_t RollbackFwmpRead(struct RollbackSpaceFwmp *fwmp)
{
	memset(fwmp, 0, sizeof(*fwmp));
	return TPM_SUCCESS;
}

#else

uint32_t RollbackKernelRead(uint32_t* version)
{
	RollbackSpaceKernel rsk;

	/*
	 * Read the kernel space and verify its permissions.  If the kernel
	 * space has the wrong permission, or it doesn't contain the right
	 * identifier, we give up.  This will need to be fixed by the
	 * recovery kernel.  We have to worry about this because at any time
	 * (even with PP turned off) the TPM owner can remove and redefine a
	 * PP-protected space (but not write to it).
	 */
	RETURN_ON_FAILURE(ReadSpaceKernel(&rsk));
#ifndef TPM2_MODE
	/*
	 * TODO(vbendeb): restore this when it is defined how the kernel space
	 * gets protected.
	 */
	{
		uint32_t perms, uid;

		RETURN_ON_FAILURE(TlclGetPermissions(KERNEL_NV_INDEX, &perms));
		memcpy(&uid, &rsk.uid, sizeof(uid));
		if (TPM_NV_PER_PPWRITE != perms ||
		    ROLLBACK_SPACE_KERNEL_UID != uid)
			return TPM_E_CORRUPTED_STATE;
	}
#endif
	memcpy(version, &rsk.kernel_versions, sizeof(*version));
	VB2_DEBUG("TPM: RollbackKernelRead %x\n", (int)*version);
	return TPM_SUCCESS;
}

uint32_t RollbackKernelWrite(uint32_t version)
{
	RollbackSpaceKernel rsk;
	uint32_t old_version;
	RETURN_ON_FAILURE(ReadSpaceKernel(&rsk));
	memcpy(&old_version, &rsk.kernel_versions, sizeof(old_version));
	VB2_DEBUG("TPM: RollbackKernelWrite %x --> %x\n",
		  (int)old_version, (int)version);
	memcpy(&rsk.kernel_versions, &version, sizeof(version));
	return WriteSpaceKernel(&rsk);
}

uint32_t RollbackKernelLock(int recovery_mode)
{
	static int kernel_locked = 0;
	uint32_t r;

	if (recovery_mode || kernel_locked)
		return TPM_SUCCESS;

	r = TlclLockPhysicalPresence();
	if (TPM_SUCCESS == r)
		kernel_locked = 1;
	return r;
}

uint32_t RollbackFwmpRead(struct RollbackSpaceFwmp *fwmp)
{
	union {
		/*
		 * Use a union for buf and fwmp, rather than making fwmp a
		 * pointer to a bare uint8_t[] buffer.  This ensures fwmp will
		 * be aligned if necesssary for the target platform.
		 */
		uint8_t buf[FWMP_NV_MAX_SIZE];
		struct RollbackSpaceFwmp fwmp;
	} u;
	uint32_t r;

	/* Clear destination in case error or FWMP not present */
	memset(fwmp, 0, sizeof(*fwmp));

	/* Try to read entire 1.0 struct */
	r = TlclRead(FWMP_NV_INDEX, u.buf, sizeof(u.fwmp));
	if (TPM_E_BADINDEX == r) {
		/* Missing space is not an error; use defaults */
		VB2_DEBUG("TPM: no FWMP space\n");
		return TPM_SUCCESS;
	} else if (TPM_SUCCESS != r) {
		VB2_DEBUG("TPM: read returned 0x%x\n", r);
		return r;
	}

	/*
	 * Struct must be at least big enough for 1.0, but not bigger
	 * than our buffer size.
	 */
	if (u.fwmp.struct_size < sizeof(u.fwmp) ||
	    u.fwmp.struct_size > sizeof(u.buf))
		return TPM_E_STRUCT_SIZE;

	/*
	 * If space is bigger than we expect, re-read so we properly
	 * compute the CRC.
	 */
	if (u.fwmp.struct_size > sizeof(u.fwmp)) {
		r = TlclRead(FWMP_NV_INDEX, u.buf, u.fwmp.struct_size);
		if (TPM_SUCCESS != r)
			return r;
	}

	/* Verify CRC */
	if (u.fwmp.crc != vb2_crc8(u.buf + 2, u.fwmp.struct_size - 2)) {
		VB2_DEBUG("TPM: bad CRC\n");
		return TPM_E_CORRUPTED_STATE;
	}

	/* Verify major version is compatible */
	if ((u.fwmp.struct_version >> 4) !=
	    (ROLLBACK_SPACE_FWMP_VERSION >> 4))
		return TPM_E_STRUCT_VERSION;

	/*
	 * Copy to destination.  Note that if the space is bigger than
	 * we expect (due to a minor version change), we only copy the
	 * part of the FWMP that we know what to do with.
	 *
	 * If this were a 1.1+ reader and the source was a 1.0 struct,
	 * we would need to take care of initializing the extra fields
	 * added in 1.1+.  But that's not an issue yet.
	 */
	memcpy(fwmp, &u.fwmp, sizeof(*fwmp));
	return TPM_SUCCESS;
}

#endif /* DISABLE_ROLLBACK_TPM */
