r"""Black-box tests for dud.exe.

    set DUD_EXE=path\to\dud.exe   (default: x64\Release\dud.exe)
    python test_dud.py
"""
import os
import subprocess
import tempfile
import time
import unittest

EXE = os.environ.get("DUD_EXE", os.path.join(os.path.dirname(__file__), "x64", "Release", "dud.exe"))
DAY = 86400


def run(*args):
    # dud ends with system("PAUSE"); feed it a newline.
    return subprocess.run([EXE, *args], input="\n", capture_output=True, text=True, timeout=60).stdout


def row(out, path):
    """Parse the text-table row for path -> (files, dirs, size_mb, days, flag)."""
    line = next(l for l in out.splitlines() if l.startswith(path))
    cols = line[len(path):].split()
    num = lambda s: float(s.replace(",", ""))
    return int(num(cols[0])), int(num(cols[1])), num(cols[2]), int(num(cols[3])), cols[4] if len(cols) > 4 else ""


def sparse_file(path, size):
    """Create a file reporting `size` bytes without using the disk space."""
    open(path, "wb").close()
    subprocess.run(["fsutil", "sparse", "setflag", path], check=True, capture_output=True)
    os.truncate(path, size)


class DudTest(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory()
        self.root = self.tmp.name

    def tearDown(self):
        self.tmp.cleanup()

    def test_help_prints_usage_and_scans_nothing(self):
        out = run("--help")
        self.assertIn("dud.exe {--help|--csv}", out)
        self.assertNotIn("Using", out)

    def test_csv_flag_switches_format_and_is_not_a_path(self):
        out = run("--csv", self.root)
        self.assertIn(f'"{self.root}",', out)
        self.assertNotIn("--csv", out)

    def test_counts_and_sizes(self):
        os.makedirs(os.path.join(self.root, "a", "b"))
        for name, n in [("x", 1000), (os.path.join("a", "y"), 2000), (os.path.join("a", "b", "z"), 3000)]:
            with open(os.path.join(self.root, name), "wb") as f:
                f.write(b"\0" * n)
        files, dirs, size_mb, _, _ = row(run(self.root), self.root)
        self.assertEqual((files, dirs), (3, 2))
        self.assertAlmostEqual(size_mb, (6000 + 2 * 512) / 2**20, places=2)  # each dir counts 512 B

    def test_size_over_4gb_does_not_wrap(self):
        sparse_file(os.path.join(self.root, "big"), 5 * 2**30)
        self.assertEqual(row(run(self.root), self.root)[2], 5120.0)

    def test_days_stale_uses_newest_entry(self):
        old, new = os.path.join(self.root, "old"), os.path.join(self.root, "new")
        open(old, "w").close()
        open(new, "w").close()
        now = time.time()
        os.utime(old, (now - 400 * DAY, now - 400 * DAY))
        os.utime(new, (now - 10 * DAY, now - 10 * DAY))
        self.assertEqual(row(run(self.root), self.root)[3], 10)

    def test_archive_flag_needs_both_stale_and_big(self):
        big = os.path.join(self.root, "big")
        sparse_file(big, 600 * 2**20)  # > 500 MB threshold
        os.utime(big, (time.time() - 400 * DAY,) * 2)
        self.assertEqual(row(run(self.root), self.root)[4], "ARCHIVE")
        os.utime(big)  # fresh again
        self.assertEqual(row(run(self.root), self.root)[4], "")

    def test_no_args_is_an_error(self):
        r = subprocess.run([EXE], input="\n", capture_output=True, text=True, timeout=60)
        self.assertIn("Missing arguments", r.stderr)

    def test_unexpanded_wildcard_is_skipped_with_warning(self):
        # setargv leaves a pattern that matches nothing as-is, so the '*' reaches dud.
        pat = os.path.join(self.root, "nomatch*")
        out = run(pat)
        self.assertIn("unexpended wildcard", out)
        self.assertFalse(any(l.startswith(pat) for l in out.splitlines()))

    def test_missing_path_is_silently_skipped(self):
        missing = os.path.join(self.root, "nope")
        out = run(missing, self.root)
        self.assertFalse(any(l.startswith(missing) for l in out.splitlines()))
        row(out, self.root)  # the real path still gets its row

    def test_every_path_gets_a_row(self):
        paths = [os.path.join(self.root, f"d{i:02}") for i in range(12)]  # more paths than threads
        for p in paths:
            os.mkdir(p)
        out = run(*paths)
        for p in paths:
            self.assertEqual(row(out, p)[:3], (0, 0, 0.0))

    def test_empty_dir(self):
        self.assertEqual(row(run(self.root), self.root)[:3], (0, 0, 0.0))

    def test_subdirectory_mtime_counts_toward_staleness(self):
        sub, f = os.path.join(self.root, "sub"), os.path.join(self.root, "sub", "f")
        os.mkdir(sub)
        open(f, "w").close()
        now = time.time()
        os.utime(f, (now - 400 * DAY,) * 2)
        os.utime(sub, (now - 3 * DAY,) * 2)
        self.assertEqual(row(run(self.root), self.root)[3], 3)

    def test_staleness_threshold_is_strictly_greater(self):
        big = os.path.join(self.root, "big")
        sparse_file(big, 600 * 2**20)
        os.utime(big, (time.time() - 365 * DAY,) * 2)
        self.assertEqual(row(run(self.root), self.root)[3:], (365, ""))
        os.utime(big, (time.time() - 366 * DAY,) * 2)
        self.assertEqual(row(run(self.root), self.root)[3:], (366, "ARCHIVE"))

    def test_stale_but_small_is_not_archived(self):
        f = os.path.join(self.root, "f")
        open(f, "w").close()
        os.utime(f, (time.time() - 400 * DAY,) * 2)
        self.assertEqual(row(run(self.root), self.root)[4], "")

    def test_csv_row_fields(self):
        os.mkdir(os.path.join(self.root, "d"))
        with open(os.path.join(self.root, "d", "f"), "wb") as f:
            f.write(b"x" * 10)
        line = next(l for l in run("--csv", self.root).splitlines() if l.startswith('"'))
        self.assertEqual(line, f'"{self.root}",1,1,0,0,')  # MB is integer-divided in CSV


if __name__ == "__main__":
    unittest.main()
