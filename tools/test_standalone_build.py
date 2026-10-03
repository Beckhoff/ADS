#!/usr/bin/env python3
"""Check Makefile staging and Debian profile selection without TwinCAT or a PLC."""

import os
from pathlib import Path
import shutil
import subprocess
import tempfile
import unittest


SOURCE = Path(__file__).resolve().parents[1]


class StandaloneBuildTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name)
        shutil.copy(SOURCE / "Makefile", self.root)
        shutil.copytree(SOURCE / "debian", self.root / "debian")
        self.commands = self.root / "commands"
        self.commands.mkdir()
        self.env = dict(os.environ, PATH=f"{self.commands}:{os.environ['PATH']}")
        for name in ("WITH_TWINCAT_ROUTER", "TCADSDLL_INCLUDE", "TCADSDLL_LIB",
                     "NINJAFLAGS", "DEB_BUILD_PROFILES", "MAKEFLAGS", "MFLAGS"):
            self.env.pop(name, None)
        for name, script in {
            "meson": '''#!/bin/sh
printf '%s\\n' "$@" > meson-args
mkdir -p build
printf standalone > build/adstool
printf library > build/libAdsLib.a
router=1
for arg do
    if [ "$arg" = "-Dtcadsdll_include=" ]; then router=0; fi
done
if [ "$router" = 1 ]; then printf router > build/tcadstool; fi
''',
            "ninja": '#!/bin/sh\nprintf "%s\\n" "$@" > ninja-args\n',
            "dh": '#!/bin/sh\nprintf "%s\\n" "${WITH_TWINCAT_ROUTER-unset}"\n',
        }.items():
            command = self.commands / name
            command.write_text(script)
            command.chmod(0o755)
        for name in ("doc/build/man/adstool.1", "AdsLib/AdsLib.h"):
            path = self.root / name
            path.parent.mkdir(parents=True, exist_ok=True)
            path.write_text(name)

    def run_make(self, *args, env=None):
        return subprocess.run(
            ["make", "--no-print-directory", "--silent", *args], cwd=self.root,
            env=env or self.env, text=True, capture_output=True, check=True,
        ).stdout

    def assert_staged(self, router):
        stage = self.root / "stage/usr"
        self.assertEqual((stage / "bin/adstool").read_text(), "standalone")
        self.assertEqual((stage / "bin/tcadstool").exists(), router)
        self.assertTrue((stage / "lib/libAdsLib.a").is_file())
        self.assertTrue((stage / "include/AdsLib/AdsLib.h").is_file())
        self.assertTrue((stage / "share/man/man1/adstool.1").is_file())
        patterns = (self.root / "debian/adstool.install").read_text().splitlines()
        files = [path for pattern in patterns for path in stage.parent.glob(pattern)]
        self.assertEqual(
            sorted(path.name for path in files),
            ["adstool", "adstool.1", "tcadstool"] if router else ["adstool", "adstool.1"],
        )

    def test_default_build_and_install_keep_both_tools(self):
        self.run_make("install", f"DESTDIR={self.root / 'stage'}")
        args = (self.root / "meson-args").read_text().splitlines()
        self.assertIn("-Dtcadsdll_include=/usr/include", args)
        self.assertIn("-Dtcadsdll_lib=/usr/lib", args)
        self.assert_staged(router=True)

    def test_standalone_build_and_install_need_no_router_artifact(self):
        self.run_make("install", "WITH_TWINCAT_ROUTER=0", "NINJAFLAGS=-j1",
                      f"DESTDIR={self.root / 'stage'}")
        args = (self.root / "meson-args").read_text().splitlines()
        self.assertIn("-Dtcadsdll_include=", args)
        self.assertIn("-Dtcadsdll_lib=", args)
        self.assertFalse((self.root / "build/tcadstool").exists())
        self.assertIn("-j1", (self.root / "ninja-args").read_text().splitlines())
        self.assert_staged(router=False)

    def test_custom_twincat_paths_preserve_spaces(self):
        self.run_make("build", "TCADSDLL_INCLUDE=/opt/TwinCAT ADS/include",
                      "TCADSDLL_LIB=/opt/TwinCAT ADS/lib")
        args = (self.root / "meson-args").read_text().splitlines()
        self.assertIn("-Dtcadsdll_include=/opt/TwinCAT ADS/include", args)
        self.assertIn("-Dtcadsdll_lib=/opt/TwinCAT ADS/lib", args)

    def test_invalid_router_option_is_rejected_before_building(self):
        with self.assertRaises(subprocess.CalledProcessError):
            self.run_make("build", "WITH_TWINCAT_ROUTER=false")
        self.assertFalse((self.root / "build").exists())

    def test_debian_profile_selects_standalone_among_other_profiles(self):
        env = dict(self.env, DEB_BUILD_PROFILES="nocheck pkg.adstool.standalone")
        self.assertEqual(self.run_make("-f", "debian/rules", "build", env=env).strip(), "0")

    def test_unrelated_debian_profile_keeps_default(self):
        env = dict(self.env, DEB_BUILD_PROFILES="pkg.adstool.standalone-extra")
        env.pop("WITH_TWINCAT_ROUTER", None)
        self.assertEqual(self.run_make("-f", "debian/rules", "build", env=env).strip(), "unset")

    def test_debian_dependency_is_required_only_without_profile(self):
        admindir = self.root / "dpkg-db"
        admindir.mkdir()
        (admindir / "status").touch()
        for profile, needs_router in (("", True), ("pkg.adstool.standalone", False)):
            result = subprocess.run(
                ["dpkg-checkbuilddeps", f"--admindir={admindir}",
                 *([f"-P{profile}"] if profile else [])], cwd=self.root,
                env=dict(self.env, DEB_BUILD_PROFILES=""), text=True, capture_output=True,
            )
            self.assertNotEqual(result.returncode, 2, result.stderr)
            self.assertEqual("libadscomm-dev" in result.stderr, needs_router, result.stderr)


if __name__ == "__main__":
    unittest.main()
