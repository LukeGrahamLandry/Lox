#include "chunk.h"

Chunk::Chunk(){
    code = new ArrayList<byte>();
    constants = new ArrayList<Value>();
    lines = new ArrayList<int>();  // run-length encoded: count, line, count, line
}

Chunk::~Chunk(){
    delete code;
    delete constants;
    delete lines;
}

Chunk::Chunk(const Chunk& other){
    code = new ArrayList<byte>(*other.code);
    delete constants;
    constants = new ArrayList<Value>(*other.constants);
    lines = new ArrayList<int>(*other.lines);
}


void Chunk::write(byte b, int line){
    code->push(b);

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
                if (!sameAddress){  // if the new Value is a different address but the same data, we delete it and use the old one instead
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
    constants->push(value);
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
    printValueArray(constants->data, constants->data + constants->size());
}

Value Chunk::getConstant(int index) {
    return constants->get(index);
}


void Chunk::setCodeAt(int index, byte value){
    code->set(index, value);
}

// When done compiling the function, the chunk is effectively immutable, so we can remove the extra list space.
void Chunk::setDone(){
    constants->shrink();
    code->shrink();
    lines->shrink();
}

// length as uint32
// OP_LOAD_INLINE_CONSTANT
// - 0
//    8 byte double
// - 1
//    length as uint32
//    string characters
// - 2
//    arity
//    chunk exportAsBinary()
// the rest of the code
ArrayList<byte>* Chunk::exportAsBinary(){
    uint32_t size = getExportSize();
    ArrayList<byte>* output = new ArrayList<byte>(size);
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
                        appendAsBytes(output, str->array.length);
                        output->appendMemory(cast(byte *, str->array.contents), str->array.length);
                        break;
                    }
                    case OBJ_FUNCTION: {
                        ObjFunction* func = AS_FUNCTION(value);
                        assert(sizeof(func->arity) == 1);
                        output->push(func->arity);
                        // keep name? should put all the names in a different uniform place.
                        ArrayList<byte>* inner = func->chunk->exportAsBinary();
                        output->append(inner);
                        delete inner;
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


    output->append(code);
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
                        total += sizeof(str->array.length);
                        total += str->array.length;
                        break;
                    }
                    case OBJ_FUNCTION: {
                        total++; // arity
                        total += AS_FUNCTION(value)->chunk->getExportSize();
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
    constants = new ArrayList<Value>();
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
    data->appendMemory(cast(byte *, &number), sizeof(number));
}

void appendAsBytes(ArrayList<byte>* data, double number){
    assert(sizeof(number) == 8);
    data->appendMemory(cast(byte *, &number), sizeof(number));
}


#define OP(name) [name] = #name,

string Chunk::opcodeNames[256] = {
        OP(OP_GET_CONSTANT)
        OP(OP_NIL)
        OP(OP_TRUE)
        OP(OP_FALSE)
        OP(OP_EQUAL)
        OP(OP_GREATER)
        OP(OP_LESS)
        OP(OP_ADD)
        OP(OP_SUBTRACT)
        OP(OP_MULTIPLY)
        OP(OP_DIVIDE)
        OP(OP_NEGATE)
        OP(OP_NOT)
        OP(OP_RETURN)
        OP(OP_EXPONENT)
        OP(OP_PRINT)
        OP(OP_POP)
        OP(OP_POP_MANY)
        OP(OP_DEFINE_GLOBAL)
        OP(OP_DEBUG_BREAK_POINT)
        OP(OP_EXIT_VM)
        OP(OP_ACCESS_INDEX)
        OP(OP_SLICE_INDEX)
        OP(OP_GET_LENGTH)
        OP(OP_GET_LOCAL)
        OP(OP_SET_LOCAL)
        OP(OP_LOAD_INLINE_CONSTANT)
};

#undef OP