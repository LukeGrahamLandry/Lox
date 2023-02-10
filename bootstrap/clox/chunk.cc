#include "chunk.h"

Chunk::Chunk(){
    code = new ArrayList<byte>();
    constants = new ValueArray();
    lines = new ArrayList<int>();  // run-length encoded: count, line, count, line
}

Chunk::~Chunk(){
    delete code;
    delete constants;
    delete lines;
}

Chunk::Chunk(const Chunk& other){
    code = new ArrayList<byte>(*other.code);
    constants = new ValueArray();
    delete constants->values;
    constants->values = new ArrayList<Value>(*other.constants->values);
    lines = new ArrayList<int>(*other.lines);
}


void Chunk::write(byte byte, int line){
    code->push(byte);

    if (!lines->isEmpty()){
        int prevLine = lines->get(lines->count - 1);
        if (line == prevLine){
            int prevCount = lines->get(lines->count - 2);
            lines->set(lines->count - 2, prevCount + 1);
            return;
        }
    }

    lines->push(1);
    lines->push(line);
}

// This has O(n) but since it will only be called while debugging or when an exception is thrown, it doesn't matter.
int Chunk::getLineNumber(int opcodeOffset){
    if (lines->isEmpty()) return -1;

    int currentTokenIndex = 0;
    for (int i=0;i<lines->count-1;i+=2){
        int count = lines->get(i);
        int lineNumber = lines->get(i + 1);
        currentTokenIndex += count;
        if (opcodeOffset <= currentTokenIndex) return lineNumber;
    }

    return -1;
}

// Adds a constant to the array (without a push op code).
// Ownership of the value's heap memory (if an object) is passed to the chunk.
const_index_t Chunk::addConstant(Value value){
    // TODO: another opcode for reading two bytes for the index for programs long enough to need more than 256
    if (constants->size() >= 256){
        cerr << "Too many constants in chunk." << endl;
        return -1;
    }

    if (IS_NIL(value) || IS_BOOL(value)){
        cerr << "Cannot add singleton value to chunk constants." << endl;
        return -1;
    }

    // Loop through all existing constants to deduplicate.
    for (int i=0;i<constants->size();i++){
        Value constant = constants->get(i);
        if (valuesEqual(value, constant)){
            if (IS_OBJ(value)){ // The Obj struct is allocated on the heap.
                bool sameAddress = AS_OBJ(value) == AS_OBJ(constant);
                if (!sameAddress){
                    switch (AS_OBJ(value)->type) {
                        case OBJ_STRING:
                            FREE(ObjString, AS_STRING(value));
                            break;
                    }
                }
            }
            return i;
        }
    }

    rawAddConstant(value);
    return constants->size() - 1;
}

void Chunk::rawAddConstant(Value value){
    constants->add(value);
}

int Chunk::getCodeSize() {
    return (int) code->count;
}

int Chunk::getConstantsSize(){
    return constants->size();
}

unsigned char *Chunk::getCodePtr() {
    return code->data;
}

unsigned char Chunk::getInstruction(int offset) {
    uint32_t index = offset < 0 ? code->count + offset: offset;
    return code->get(index);
}

int Chunk::popInstruction() {
    return code->pop();
}

void Chunk::printConstantsArray() {
    printValueArray(constants->values->data, constants->values->data + constants->size());
}

Value Chunk::getConstant(int index) {
    return constants->get(index);
}


// length as uint32
// OP_LOAD_INLINE_CONSTANT
// - 0
//    8 byte double
// - 1
//    length as uint32
//    string characters
// the rest of the code
ArrayList<byte>* Chunk::exportAsBinary(){
    ArrayList<byte>* output = new ArrayList<byte>();
    uint32_t size = getExportSize();
    appendAsBytes(output, size);

    assert(sizeof(double) == 8);
    for (int i=0;i<constants->size();i++){
        output->push(OP_LOAD_INLINE_CONSTANT);
        Value value = constants->get(i);
        switch (value.type) {
            case VAL_NUMBER: {
                output->push(0);
                appendAsBytes(output, AS_NUMBER(value));
                break;
            }
            case VAL_OBJ: {
                ObjType type = AS_OBJ(value)->type;
                output->push(1 + type);
                switch (type) {
                    case OBJ_STRING: {
                        ObjString* str = AS_STRING(value);
                        assert(sizeof(str->length) == 4);
                        appendAsBytes(output, str->length);
                        output->appendMemory(cast(byte*, str->chars), str->length);
                        break;
                    }
                    default:
                        cerr << "Invalid Object Type " << type << endl;
                        output->pop();
                        output->pop();
                        break;
                }
                break;
            }
            default:
                cerr << "Invalid Constant Type " << value.type << endl;
                output->pop();
                break;
        }
    }


    output->appendMemory(code->peek(0), code->count);
    return output;
}

uint32_t Chunk::getExportSize(){
    uint32_t total = 0;
    for (int i=0;i<constants->size();i++){
        Value value = constants->get(i);
        total += 2;
        switch (value.type) {
            case VAL_NUMBER: {
                total += sizeof(double);
                break;
            }
            case VAL_OBJ: {
                ObjType type = AS_OBJ(value)->type;
                switch (type) {
                    case OBJ_STRING: {
                        ObjString* str = AS_STRING(value);
                        total += sizeof(str->length);
                        total += str->length;
                        break;
                    }
                    default:
                        cerr << "Invalid Object Type " << type << endl;
                        total -= 2;
                        break;
                }
                break;
            }
            default:
                cerr << "Invalid Constant Type " << value.type << endl;
                total -= 2;
                break;
        }
    }

    total += code->count;
    return total;
}

void Chunk::exportAsBinary(const char* path) {
    ArrayList<byte>* data = exportAsBinary();
    ofstream stream;
    stream.open(path, ios::out | ios::binary);
    stream.write(cast(char*, data->peek(0)), data->count);
    stream.close();
    delete data;
}


// The chunk owns the array list now.
Chunk::Chunk(ArrayList<byte>* exportedBinaryData){
    code = exportedBinaryData;
    constants = new ValueArray();
    lines = new ArrayList<int>();
}

Chunk* Chunk::importFromBinary(const char *path) {
    ifstream stream;
    stream.open(path, ios::in | ios::binary);
    uint32_t length = 0;
    stream.read((char*) &length, sizeof(uint32_t));
    if (length == 0){
        return nullptr;
    }
    char data[length];
    stream.read(data, length);
    stream.close();

    return new Chunk(new ArrayList<byte>(cast(byte*, data), length));
}

// TODO: do i need to worry about other systems having a different byte order?
void appendAsBytes(ArrayList<byte>* data, uint32_t number){
    assert(sizeof(number) == 4);
    data->appendMemory(cast(byte*, &number), sizeof(number));
}

void appendAsBytes(ArrayList<byte>* data, double number){
    assert(sizeof(number) == 8);
    data->appendMemory(cast(byte*, &number), sizeof(number));
}
