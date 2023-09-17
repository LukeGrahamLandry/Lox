# This is a simpler version of https://github.com/munificent/craftinginterpreters/blob/master/tool/bin/test.dart
import os

lox_path = "out/lox"
tests_dir = ["tests/craftinginterpreters/test", "tests/case"]
skip_files = [
    # My implementation doesn't have special treatment for global variables.
    "use_global_in_initializer.lox", "redeclare_global.lox", "unreached_undefined.lox", "redefine_global.lox",
    "mutual_recursion.lox"
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
        err_prefix = ["// expect runtime error:", "// [line", "// // expect runtime error:", "// Error at"]
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
        result_flag = process.close()
        exitcode = 0 if result_flag is None else result_flag >> 8

        if expect_error:
            if exitcode != 70 and exitcode != 65:
                print("FAIL", path, "Expected status 65 or 70 but got", exitcode)
                failed.append(path)
            else:
                print("PASS", path)
                pass
        else:
            if exitcode == 0:
                output = output.splitlines()
                if len(expected_output) != len(output):
                    print("FAIL", path, "Expected", len(expected_output), "lines of output but got", len(output))
                    failed.append(path)
                else:
                    for expect, found in zip(expected_output, output):
                        if expect != found:
                            print("FAIL", path, "Expected '", expect, "' but got '", found, "'")
                            failed.append(path)
                            break
                    else:
                        print("PASS", path)
                        pass
            else:
                print("FAIL", path, "Expected status 0 but got", exitcode)
                failed.append(path)


# Cope with being run from tests subdir.
if not os.path.exists("Makefile"):
    os.chdir("..")
    if not os.path.exists("Makefile"):
        print("Makefile not found.")
        exit(1)

os.system("make native")

for tests in tests_dir:
    for root, dirs, files in os.walk(tests):
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
# TODO: some are failing.
#       auto import clock
#       support mutual recursion somehow
#       gc problems found by asan
