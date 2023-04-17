import os
from random import randint, randrange

def lox(code):
    filename = "testcase.lox"
    with open(filename, "w") as f:
        f.write(code)

    process = os.popen("./clox -s " + filename)
    output = process.read()
    process.close()

    os.remove(filename)

    return output


def py(code):
    filename = "testcase.py"
    with open(filename, "w") as f:
        f.write(code)

    process = os.popen("python3 " + filename)
    output = process.read()
    process.close()

    os.remove(filename)

    return output


testCount = [0]


def compareToPython(name, lox_code, python_code):
    testCount[0] += 1

    python_result = py(python_code).splitlines()
    lox_result = lox(lox_code).splitlines()

    failed = 0
    total = 0
    for i, line in enumerate(python_result):
        if line.endswith(".0"):
            line = line[:-2]

        if line != lox_result[i]:
            print("[line {}] ({}) != ({})".format(i, line, lox_result[i]))
            failed += 1

        total += 1

    if failed == 0:
        print("PASS TEST", testCount[0], name)
    else:
        print("FAIL TEST", testCount[0], name)
        print("Lox:")
        print(lox_code)
        print("Python:")
        print(python_code)


def simple_arithmetic():
    lox_code = ""
    python_code = ""
    signs = ["+", "-", "*"]
    for i in range(10):
        lox_code += "print 10"
        python_code += "print(10"
        for j in range(20):
            sign = signs[randrange(len(signs))]
            if sign == "+" or sign == "-":
                value = randint(-1000, 1000)
            elif sign == "*":
                value = randint(-10, 10)
            else:
                value = 1

            lox_code += " " + sign + " " + str(value)
            python_code += " " + sign + " " + str(value)

        lox_code += ";\n"
        python_code += ")\n"

    compareToPython("simple_arithmetic", lox_code, python_code)


def safe_division():
    lox_code = ""
    python_code = ""
    for i in range(10):
        code = ""
        result = randrange(-10, 10)
        for i in range(10):
            v = randrange(-10, 10)
            if v == 0:
                v = 2
            result *= v

            code += " / " + str(v)

        lox_code += "print " + str(result) + code + ";\n"
        python_code += "print(" + str(result) + code + ")\n"

    compareToPython("safe_division", lox_code, python_code)


def string_indexing():
    letters = "qwertyuiopasdfghjklzxcvbnm"
    strings = []
    for i in range(10):
        s = ""
        for j in range(randrange(20) + 5):
            s += letters[randrange(len(letters))]
        strings.append(s)

    lox_code = ""
    python_code = ""
    for i in range(5):
        code = "\"" + str(i) + "\""
        for j in range(10):
            s = strings[randrange(len(strings))]
            a = randrange(len(s))
            b = randrange(len(s))
            if a > b:
                a, b = b, a
            if a == b:
                a = 0
                b = randrange(len(s)-2) + 1

            code += " + \"" + s + "\"[" + str(a) + ":" + str(b) + "]"

        lox_code += "print " + code + ";\n"
        python_code += "print(" + code + ")\n"

    compareToPython("string_indexing", lox_code, python_code)


def tests_cc():
    process = os.popen("./clox_tests")
    output = process.read()
    process.close()
    print(output)

os.system("make clean")
os.system("make all")
os.chdir("out")

tests_cc()
simple_arithmetic()
safe_division()
# string_indexing()
