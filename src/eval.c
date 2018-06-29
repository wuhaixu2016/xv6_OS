#include "types.h"
#include "user.h"
#include "fcntl.h"

#ifndef NULL
#define NULL 0
#endif

#define _CACHE_SIZE 2000
#define _FLOAT_CACHE_SIZE _CACHE_SIZE
#define _CHAR_CACHE_SIZE _CACHE_SIZE
#define _INT_CACHE_SIZE _CACHE_SIZE

float _FLOAT_CACHE[_FLOAT_CACHE_SIZE];

unsigned int _float_cache_pointer = 0;

void ResetFloatCache() {
    _float_cache_pointer = 0;
}

float * StoreFloat(float f) {
    if (_FLOAT_CACHE_SIZE <= _float_cache_pointer) {
        return NULL;
    }
    _FLOAT_CACHE[_float_cache_pointer] = f;
    _float_cache_pointer++;
    return &_FLOAT_CACHE[_float_cache_pointer - 1];
}

float * FloatCacheTop() {
    return &_FLOAT_CACHE[_float_cache_pointer - 1];
}

char _CHAR_CACHE[_CHAR_CACHE_SIZE];

unsigned int _char_cache_pointer = 0;

void ResetCharCache() {
    _char_cache_pointer = 0;
}

char * StoreChar(char c) {
    if (_CHAR_CACHE_SIZE <= _char_cache_pointer) {
        return NULL;
    }
    _CHAR_CACHE[_char_cache_pointer] = c;
    _char_cache_pointer++;
    return &_CHAR_CACHE[_char_cache_pointer - 1];
}

char * CharCacheTop() {
    return &_CHAR_CACHE[_char_cache_pointer - 1];
}

int _INT_CACHE[_INT_CACHE_SIZE];

unsigned int _int_cache_pointer = 0;

void ResetIntCache() {
    _int_cache_pointer = 0;
}

int * StoreInt(int i) {
    if (_INT_CACHE_SIZE <= _int_cache_pointer) {
        return NULL;
    }
    _INT_CACHE[_int_cache_pointer] = i;
    _int_cache_pointer++;
    return &_INT_CACHE[_int_cache_pointer - 1];
}

int * IntCacheTop() {
    return &_INT_CACHE[_int_cache_pointer - 1];
}

void ResetCache() {
    ResetFloatCache();
    ResetCharCache();
    ResetIntCache();
}

typedef void * CVectorElementType;

typedef struct {
    unsigned int size;
    unsigned int capacity;
    CVectorElementType * data;
} CVectorStruct, *CVector;

#define VECTOR_DEFAULT_CAPACITY 8

unsigned int CVectorSize(CVector vector) {
    return vector->size;
}

CVector CVectorCreate() {
    CVector vector = (CVector)malloc(sizeof(CVectorStruct));
    vector->size = 0;
    vector->capacity = VECTOR_DEFAULT_CAPACITY;
    vector->data = (CVectorElementType *)malloc(VECTOR_DEFAULT_CAPACITY * sizeof(CVectorElementType));
    return vector;
}

CVector CVectorCopy(CVector that) {
    CVector new_vector = (CVector)malloc(sizeof(CVectorStruct));
    new_vector->size = that->size;
    new_vector->capacity = that->size;
    new_vector->data = (CVectorElementType *)malloc(that->capacity * sizeof(CVectorElementType));
    memmove((void *)new_vector->data, (void *)that->data, (uint)(that->size * sizeof(CVectorElementType)));
    return new_vector;
}

void CVectorDestroy(CVector vector) {
    if (!vector) {
        return;
    }
    if (vector->data) {
        free(vector->data);
    }
    free(vector);
}

void CVectorExpand(CVector vector) {
    if (vector->size < vector->capacity) {
        return;
    }
    vector->capacity <<= 1;
    CVectorElementType * old_data = vector->data;
    vector->data = (CVectorElementType *)malloc(vector->capacity * sizeof(CVectorElementType));
    memmove(vector->data, old_data, vector->size * sizeof(CVectorElementType));
    free(old_data);
}

CVectorElementType CVectorRemove(CVector vector, unsigned int rank) {
    CVectorElementType element_to_remove = vector->data[rank];
    for (unsigned int i = rank; i < vector->size - 1; i++) {
        vector->data[i] = vector->data[i + 1];
    }
    vector->size--;
    return element_to_remove;
}

void CVectorInsert(CVector vector, unsigned int rank, CVectorElementType element) {
    CVectorExpand(vector);
    vector->size++;
    for (unsigned int i = vector->size; rank < i; i--) {
        vector->data[i] = vector->data[i - 1];
    }
    vector->data[rank] = element;
}

void CVectorPushBack(CVector vector, CVectorElementType element) {
    CVectorExpand(vector);
    vector->data[vector->size++] = element;
}

CVectorElementType CVectorPopBack(CVector vector) {
    return vector->data[--(vector->size)];
}

CVectorElementType CVectorGet(CVector vector, unsigned rank) {
    return vector->data[rank];
}

void CVectorSet(CVector vector, unsigned rank, CVectorElementType element) {
    vector->data[rank] = element;
}

typedef CVectorElementType CStackElementType;

typedef struct {
    CVector data;
} CStackStruct, *CStack;

CStack CStackCreate() {
    CStack stack = (CStack)malloc(sizeof(CStackStruct));
    stack->data = CVectorCreate();
    return stack;
}

void CStackDestroy(CStack stack) {
    if (!stack) {
        return;
    }
    if (stack->data) {
        CVectorDestroy(stack->data);
    }
    free(stack);
}

void CStackPush(CStack stack, CStackElementType element) {
    CVectorPushBack(stack->data, element);
}

CStackElementType CStackPop(CStack stack) {
    return CVectorPopBack(stack->data);
}

unsigned int CStackSize(CStack stack) {
    return CVectorSize(stack->data);
}

CStackElementType CStackTop(CStack stack) {
    return CVectorGet(stack->data, CStackSize(stack) - 1);
}

#define _RPN_MAX_LENGTH 2000

char RPN[_RPN_MAX_LENGTH] = "";

unsigned int _rpn_pointer = 0;

void ResetRpn() {
    _rpn_pointer = 0;
}

char * GetRpn() {
    return RPN;
}

char * RpnPush(char c) {
    if (_RPN_MAX_LENGTH - 1 <= _rpn_pointer) {
        return NULL;
    }
    RPN[_rpn_pointer++] = c;
    RPN[_rpn_pointer] = '\0';
    return &RPN[_rpn_pointer - 1];
}

#define N_OPTR 9 

typedef enum { ADD, SUB, MUL, DIV, POW, FAC, L_P, R_P, EOE } Operator;

const char OPERATION_PRIORITY[N_OPTR][N_OPTR] = {
    { '>',   '>',   '<',   '<',   '<',   '<',   '<',   '>',   '>' },
    { '>',   '>',   '<',   '<',   '<',   '<',   '<',   '>',   '>' },
    { '>',   '>',   '>',   '>',   '<',   '<',   '<',   '>',   '>' },
    { '>',   '>',   '>',   '>',   '<',   '<',   '<',   '>',   '>' },
    { '>',   '>',   '>',   '>',   '>',   '<',   '<',   '>',   '>' },
    { '>',   '>',   '>',   '>',   '>',   '>',   ' ',   '>',   '>' },
    { '<',   '<',   '<',   '<',   '<',   '<',   '<',   '=',   ' ' },
    { ' ',   ' ',   ' ',   ' ',   ' ',   ' ',   ' ',   ' ',   ' ' },
    { '<',   '<',   '<',   '<',   '<',   '<',   '<',   ' ',   '=' }
};

Operator Optr2Rank(char optr) {
    switch (optr) {
    case '+': return ADD; 
    case '-': return SUB; 
    case '*': return MUL; 
    case '/': return DIV; 
    case '^': return POW; 
    case '!': return FAC; 
    case '(': return L_P; 
    case ')': return R_P; 
    case '\0': return EOE; 
    default: exit();
    }
}

char OrderBetween(char operation1, char operation2) {
    return OPERATION_PRIORITY[Optr2Rank(operation1)][Optr2Rank(operation2)];
}

int Factorial(int n) {
    int ret = 1;
    for (int i = n; 0 < i; --i) {
        ret *= i;
    }
    return ret;
}

int Pow(int number, int n) {
    if (n == 0) {
        return 1;
    }
    if (n < 0) {
        return -1;
    }
    int result = number;
    for (int i = 1; i < n; i++) {
        result *= number;
    }
    return result;
}

float Calculate2(float a, char optr, float b) {
    switch (optr) {
    case '+': return a + b;
    case '-': return a - b;
    case '*': return a * b;
    case '/': return a / b;
    case '^': Pow(a, b); // Fix Me !
    default: exit();
    }
}

float Calculate1(char optr, float b) {
    switch (optr) {
    case '!': return (float)Factorial((int)b);
    default: exit();
    }
}

int IsDigit(char c) {
    return '0' <= c && c <= '9' ? 1 : 0;
}

//void ReadNumber(char * & posi, CStack stack) { // CStack -> float
//    CStackPush(stack, StoreFloat((float)(*posi - '0')));
//    while (IsDigit(*(++posi))) {
//        CStackPush(stack, StoreFloat(*((float *)CStackPop(stack)) * 10 + (*posi - '0')));
//    }
//    if ('.' != *posi) {
//        return;
//    }
//    float fraction = 1;
//    while (IsDigit(*(++posi))) {
//        CStackPush(stack, StoreFloat(*((float *)CStackPop(stack)) + (*posi - '0') * (fraction /= 10)));
//    }
//}

void ReadNumber(char ** posi, CStack stack) { // CStack -> float
    CStackPush(stack, StoreFloat((float)(**posi - '0')));
    while (IsDigit(*(++(*posi)))) {
        CStackPush(stack, StoreFloat(*((float *)CStackPop(stack)) * 10 + (**posi - '0')));
    }
    if ('.' != **posi) {
        return;
    }
    float fraction = 1;
    while (IsDigit(*(++(*posi)))) {
        CStackPush(stack, StoreFloat(*((float *)CStackPop(stack)) + (**posi - '0') * (fraction /= 10)));
    }
}

void Reverse(char *buffer) {
    int i, j;
    char tmp;
    for (i = 0, j = strlen(buffer) - 1; i < j; i++, j--) {
        tmp = buffer[i];
        buffer[i] = buffer[j];
        buffer[j] = tmp;
    }
}

void Itoa(int number, char *buffer, int radix) {
    static const char alphabet[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
    int rem, i = 0;
    short sign = number > 0;
    do {
        buffer[i++] = (rem = number % radix) < 10
            ? rem + '0'
            : alphabet[rem - 10];
    } while ((number /= radix) > 0);
    if (!sign) {
        buffer[i++] = '-';
    }
    buffer[i++] = '\0';
    Reverse(buffer);
}

char * FloatToString(char * outstr, float value, int places) {
    int min_width = 0;
    int rightjustify = 0; // false

    // this is used to write a float value to string, outstr.  oustr is also the return value.
    int digit;
    float tens = 0.1;
    int tenscount = 0;
    int i;
    float tempfloat = value;
    int c = 0;
    int charcount = 1;
    int extra = 0;
    // make sure we round properly. this could use pow from <math.h>, but doesn't seem worth the import
    // if this rounding step isn't here, the value  54.321 prints as 54.3209

    // calculate rounding term d:   0.5/pow(10,places)
    float d = 0.5;
    if (value < 0) {
        d *= -1.0;
    }
    // divide by ten for each decimal place
    for (i = 0; i < places; i++) {
        d /= 10.0;
    }
    // this small addition, combined with truncation will round our values properly
    tempfloat += d;

    // first get value tens to be the large power of ten less than value
    if (value < 0) {
        tempfloat *= -1.0;
    }
    while ((tens * 10.0) <= tempfloat) {
        tens *= 10.0;
        tenscount += 1;
    }

    if (tenscount > 0) {
        charcount += tenscount;
    }
    else {
        charcount += 1;
    }

    if (value < 0) {
        charcount += 1;
    }
    charcount += 1 + places;

    min_width += 1; // both count the null final character
    if (min_width > charcount) {
        extra = min_width - charcount;
        charcount = min_width;
    }

    if (extra > 0 && rightjustify) {
        for (int i = 0; i < extra; i++) {
            outstr[c++] = ' ';
        }
    }

    // write out the negative if needed
    if (value < 0) {
        outstr[c++] = '-';
    }
    if (tenscount == 0) {
        outstr[c++] = '0';
    }
    for (i = 0; i < tenscount; i++) {
        digit = (int)(tempfloat / tens);
        Itoa(digit, &outstr[c++], 10);
        tempfloat = tempfloat - ((float)digit * tens);
        tens /= 10.0;
    }

    // if no places after decimal, stop now and return

    // otherwise, write the point and continue on
    if (places > 0) {
        outstr[c++] = '.';
    }

    // now write out each decimal place by shifting digits one by one into the ones place and writing the truncated value
    for (i = 0; i < places; i++) {
        tempfloat *= 10.0;
        digit = (int)tempfloat;
        Itoa(digit, &outstr[c++], 10);
        // once written, subtract off that digit
        tempfloat = tempfloat - (float)digit;
    }
    if (extra > 0 && !rightjustify) {
        for (int i = 0; i < extra; i++) {
            outstr[c++] = ' ';
        }
    }

    outstr[c++] = '\0';
    return outstr;
}

void AppendOperand2Rpn(float operand) {
    char * buffer = (char *)malloc(100 * sizeof(char));
    if (operand != (float)(int)operand) {
        FloatToString(buffer, operand, 2);
    }
    else {
        FloatToString(buffer, operand, 0);
    }
    unsigned int n = strlen(buffer);
    for (unsigned int i = 0; i < n; ++i) {
        RpnPush(buffer[i]);
    }
    free(buffer);
}

void AppendOperator2Rpn(char optr) {
    RpnPush(optr);
}

float Evaluate(char * posi) {
    ResetRpn();
    ResetCache();
    CStack operands = CStackCreate(); // float
    CStack operators = CStackCreate(); // char 
    CStackPush(operators, StoreChar('\0'));
    while (CStackSize(operators)) {
        if (IsDigit(*posi)) {
            ReadNumber(&posi, operands);
            AppendOperand2Rpn(*((float *)CStackTop(operands)));
        }
        else {
            switch (OrderBetween(*((char *)CStackTop(operators)), *posi)) {
            case '<':
                CStackPush(operators, StoreChar(*posi));
                posi++;
                break;
            case '=':
                CStackPop(operators);
                posi++;
                break;
            case '>': {
                char op = *((char *)CStackPop(operators));
                AppendOperator2Rpn(op);
                if ('!' == op) {
                    float pOpnd = *((float *)CStackPop(operands));
                    CStackPush(operands, StoreFloat(Calculate1(op, pOpnd)));
                }
                else {
                    float pOpnd2 = *((float *)CStackPop(operands));
                    float pOpnd1 = *((float *)CStackPop(operands));
                    CStackPush(operands, StoreFloat(Calculate2(pOpnd1, op, pOpnd2)));
                }
                break;
            }
            default: exit();
            }
        }
    }
    float ret = *((float *)CStackPop(operands));
    CStackDestroy(operands);
    CStackDestroy(operators);
    return ret;
}

char * RemoveSpace(char * posi) {
    unsigned int length = strlen(posi);
    unsigned int current = 0;
    for (unsigned int i = 0; i < length; ++i) {
        if (posi[i] != ' ') {
            posi[current] = posi[i];
            current++;
        }
    }
    posi[current] = '\0';
    return posi;
}

char input_cache[1000] = "";
char output_cache[1000] = "";

int main(int argc, char* argv[]) {
    if (argc != 2) {
        printf(2, "syntax-error.\n");
        exit();
    }
    memmove(input_cache, argv[1], strlen(argv[1]) * sizeof(char));
    float result = Evaluate(RemoveSpace(input_cache));
    if (result == (float)(int)result) {
        FloatToString(output_cache, result, 0);
    }
    else {
        FloatToString(output_cache, result, 6);        
    }
    uint length = strlen(output_cache);
    for (int i = 1; i < length; ++i) {
        if (output_cache[i] == '-') {
            output_cache[i] = '0';
        }
    }
    printf(1, "%s = %s\n", input_cache, output_cache);
    exit();
}