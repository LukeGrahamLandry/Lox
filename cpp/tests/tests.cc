#include <sstream>
#include <iostream>

using namespace std;

void startTest(const string& name);
void endTest(const string& name, bool result);
std::string exec(string cmd);

void run(const string& name, const string& expectedOutput) {
    string cmd = "./out/lox ./tests/case/" + name;
    startTest(name);
    string result = exec(cmd);

    bool success = result == expectedOutput;
    if (!success){
        cout << "Expected output: " << endl << expectedOutput << endl;
        cout << "Actual output: " << endl << result << endl;
    }
    endTest(name, success);
}

int passed = 0;
int total = 0;
vector<string> failed;

int main(){
    run(
        "strings.lox",
 "h\no\nhe\nllo\nhello\nell\nell\ntrue\n"
    );

    run(
        "functions.lox",
 "3\nouter\nreturn from outer\ncreate inner closure\nvalue\ndone\n"
    );

    run(
        "class.lox",
"Toast\nToast instance\ngrape\ngrape\ndone\nEnjoy water\nEnjoy water2\nhi\nEnjoy water3\n"
    );

    run(
        "super.lox",
 "Dunk in the fryer.\nFinish with icing\nA method\nA method\nB method\n"
    );

    cout << "---\n";
    for (const string& n : failed) {
        cout << "FAIL " << n << endl;
    }
    cout << "Passed " << passed << " of " << total << " tests." << endl;
}

std::string exec(string cmd) {
    std::array<char, 128> buffer;
    std::string result;
    std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd.c_str(), "r"), pclose);
    if (!pipe) {
        throw std::runtime_error("popen() failed!");
    }
    while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
        result += buffer.data();
    }
    return result;
}

void startTest(const string& name){
    total++;
    cout << "START TEST " << total << ": " << name << "." << endl;
}

void endTest(const string& name, bool result){
    if (result) cout << "PASS";
    else cout << "FAIL";
    cout << " TEST " << total << ": " << name << "." << endl;
    passed += result;
    if (!result) {
        failed.push_back(name);
    }
}
