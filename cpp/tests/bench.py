import os

lox_path = "out/lox"
tests_dir = ["tests/craftinginterpreters/test/benchmark"]

# Cope with being run from tests subdir.
if not os.path.exists("Makefile"):
    os.chdir("..")
    if not os.path.exists("Makefile"):
        print("Makefile not found.")
        exit(1)

os.system("make native")

for tests in tests_dir:
    for root, dirs, files in os.walk(tests):
        for filename in files:
            if not filename.endswith(".lox"):
                continue

            path = root + "/" + filename
            print("RUN", path)

            process = os.popen(lox_path + " " + path)
            output = process.read()
            result_flag = process.close()
            exitcode = 0 if result_flag is None else result_flag >> 8

            print(output)
