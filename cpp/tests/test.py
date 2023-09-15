# This is a simpler version of https://github.com/munificent/craftinginterpreters/blob/master/tool/bin/test.dart
import os

lox_path = "out/lox"
tests_dir = ["tests/craftinginterpreters/test", "tests/case"]
skip_files = [
    # My implementation doesn't have special treat for global variables.
    "use_global_in_initializer.lox", "redeclare_global.lox", "unreached_undefined.lox", "redefine_global.lox"
]
skip_folders = ["scanning", "expressions", "benchmark"]
failed = []
count = 0


def parse_test_spec(src: list[str]) -> (list[str], bool):
    expected = []
    for line in src:
        comment_start = line.find("//")
        if comment_start == -1:
            continue

        out_prefix = "// expect: "
        err_prefix = ["// expect runtime error:", "// [line"]
        comment = line[comment_start:]
        if comment.startswith(out_prefix):
            expected.append(comment[len(out_prefix):].rstrip())
        else:
            for err in err_prefix:
                if comment.startswith(err):
                    return [], True

    return expected, False


def run_tests(root: str, files: list[str]):
    global count

    for check in skip_folders:
        if root.endswith(check):
            return

    for filename in files:
        if not filename.endswith(".lox") or filename in skip_files:
            continue
        path = root + "/" + filename
        count += 1

        with open(path, "r") as f:
            lines = f.readlines()

        expected_output, expect_error = parse_test_spec(lines)

        process = os.popen(lox_path + " " + path)
        output = process.read()
        status = process.close()

        if expect_error:
            if status == 0:
                print("FAIL", filename, "Expected error but status=0.")
                failed.append(filename)
            else:
                print("PASS", filename)
        else:
            output = output.splitlines()
            if len(expected_output) != len(output):
                print("FAIL", filename, "Expected", len(expected_output), "lines of output but got", len(output))
                failed.append(filename)
            else:
                for expect, found in zip(expected_output, output):
                    if expect != found:
                        print("FAIL", filename, "Expected '", expect, "' but got '", found, "'")
                        failed.append(filename)
                        break
                else:
                    print("PASS", filename)


# Cope with being run from tests subdir.
if not os.path.exists("Makefile"):
    os.chdir("..")
    if not os.path.exists("Makefile"):
        print("Makefile not found.")
        exit(1)

os.system("make native")

for tests in tests_dir:
    for root, dirs, files in os.walk(tests, topdown=False):
        run_tests(root, files)

if len(failed) == 0:
    print("Passed all", count, "tests!")
else:
    print("Failed", len(failed), "of", count, "tests")
    for filename in failed:
        print("  -", filename)
    print("Failed", len(failed), "of", count, "tests")


# TODO: tests for my additions. a?b:c ** break continue
# TODO: capture stderr so it doesnt spam the console so much
# TODO: runner for benches
# TODO: 5 are failing.
#       auto import clock
#       support mutual recursion somehow
#       and/or short-circuiting is broken
#       method_binds_this is broken
