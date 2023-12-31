#!/usr/bin/env python3
# Copyright 2022 The ChromiumOS Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Tests for sign_uefi.py.

This is run as part of `make runtests`, or `make runtestscripts` if you
want something a little faster.
"""

from pathlib import Path
import tempfile
import unittest
from unittest import mock

import sign_uefi


class Test(unittest.TestCase):
    """Test sign_uefi.py."""

    @mock.patch("sign_uefi.inject_vbpubk")
    @mock.patch.object(sign_uefi.Signer, "sign_efi_file")
    def test_successful_sign(self, mock_sign, mock_inject_vbpubk):
        with tempfile.TemporaryDirectory() as tmp_dir:
            tmp_dir = Path(tmp_dir)

            # Get key paths.
            key_dir = tmp_dir / "keys"
            db_dir = key_dir / "db"
            db_children_dir = db_dir / "db.children"

            # Get target paths.
            target_dir = tmp_dir / "boot"
            syslinux_dir = target_dir / "syslinux"
            efi_boot_dir = target_dir / "efi/boot"

            # Make test dirs.
            syslinux_dir.mkdir(parents=True)
            efi_boot_dir.mkdir(parents=True)
            db_children_dir.mkdir(parents=True)

            # Make key files.
            (db_dir / "db.pem").touch()
            (db_children_dir / "db_child.pem").touch()
            (db_children_dir / "db_child.rsa").touch()

            # Make EFI files.
            (efi_boot_dir / "bootia32.efi").touch()
            (efi_boot_dir / "bootx64.efi").touch()
            (efi_boot_dir / "testia32.efi").touch()
            (efi_boot_dir / "testx64.efi").touch()
            (efi_boot_dir / "crdybootia32.efi").touch()
            (efi_boot_dir / "crdybootx64.efi").touch()
            (syslinux_dir / "vmlinuz.A").touch()
            (syslinux_dir / "vmlinuz.B").touch()
            (target_dir / "vmlinuz-5.10.156").touch()
            (target_dir / "vmlinuz").symlink_to(target_dir / "vmlinuz-5.10.156")

            # Set an EFI glob that matches only some of the EFI files.
            efi_glob = "test*.efi"

            # Sign, but with the actual signing mocked out.
            sign_uefi.sign_target_dir(target_dir, key_dir, efi_glob)

            # Check that the correct list of files got signed.
            self.assertEqual(
                mock_sign.call_args_list,
                [
                    # The test*.efi files match the glob,
                    # the boot*.efi files don't.
                    mock.call(efi_boot_dir / "testia32.efi"),
                    mock.call(efi_boot_dir / "testx64.efi"),
                    # Two crdyboot files.
                    mock.call(efi_boot_dir / "crdybootia32.efi"),
                    mock.call(efi_boot_dir / "crdybootx64.efi"),
                    # Two syslinux kernels.
                    mock.call(syslinux_dir / "vmlinuz.A"),
                    mock.call(syslinux_dir / "vmlinuz.B"),
                    # One kernel in the target dir.
                    mock.call(target_dir / "vmlinuz-5.10.156"),
                ],
            )

            # Check that `inject_vbpubk` was called on both the crdyboot
            # executables.
            self.assertEqual(
                mock_inject_vbpubk.call_args_list,
                [
                    mock.call(efi_boot_dir / "crdybootia32.efi", key_dir),
                    mock.call(efi_boot_dir / "crdybootx64.efi", key_dir),
                ],
            )

    @mock.patch("sign_uefi.subprocess.run")
    def test_inject_vbpubk(self, mock_run):
        with tempfile.TemporaryDirectory() as tmp_dir:
            tmp_dir = Path(tmp_dir)

            key_dir = tmp_dir / "key_dir"
            uefi_key_dir = key_dir / "uefi"
            uefi_key_dir.mkdir(parents=True)

            efi_file = tmp_dir / "test_efi_file"
            sign_uefi.inject_vbpubk(efi_file, uefi_key_dir)

            # Check that the expected command runs.
            self.assertEqual(
                mock_run.call_args_list,
                [
                    mock.call(
                        [
                            "sudo",
                            "objcopy",
                            "--update-section",
                            f'.vbpubk={uefi_key_dir / "../kernel_subkey.vbpubk"}',
                            efi_file,
                        ],
                        check=True,
                    )
                ],
            )


if __name__ == "__main__":
    unittest.main()
