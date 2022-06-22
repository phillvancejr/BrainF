#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <sstream>
#include "tinycc/libtcc.h"
#include <filesystem>
#define MAX_CELLS 30000

typedef unsigned char u8;
u8 cells[MAX_CELLS];
size_t dp = 0;


#define MAX_OPS 1024
size_t ops[MAX_OPS];

void dump_cells(size_t cell_count);
void interpret(char* src, size_t src_len);
void c_backend(char* src, size_t src_len, char* file_path);
void help(){
    puts(R"(Usage: bf [options] file_path

Options: 
    -h, --help      Display this message
    -c              Compile to executable
)");
}

bool is_bf_file(char* f) {
    auto len = strlen(f);
    return  f[len-3] == '.' &&
            f[len-2] == 'b' &&
            f[len-1] == 'f';
}

int main(int argc, char** argv) {
    char* file_path;
    bool compile = false;
    if (argc == 3) {
        auto arg = argv[1];
        if (strcmp(arg, "-h") == 0 || strcmp(arg, "--help") == 0){
            help();
            return 0;
        } else if (strcmp(arg, "-c") == 0){
            compile = true;
        }

        auto f = argv[2];
        auto len = strlen(f);
        if (is_bf_file(f)){
            file_path = f;
        } else {
            fprintf(stderr, "\x1b[91mexpected .bf file but found %s\n\x1b[0m", f);
            return EXIT_FAILURE;
        }
    } else if (argc == 2 ){
        auto f = argv[1];
        auto len = strlen(f);
        if (is_bf_file(f)){
            file_path = f;
        } else {
            fprintf(stderr, "\x1b[91mexpected .bf file but found %s\n\x1b[0m", f);
            return EXIT_FAILURE;
        }

    } else {
        help();
        return 0;
    }
    memset(cells,0,MAX_CELLS);
    memset(ops,0,MAX_OPS);
    char* src;
    size_t src_len;
    FILE* src_file = fopen(file_path, "r");
    fseek(src_file, 0, SEEK_END);
    src_len = ftell(src_file);
    rewind(src_file);
    src = (char*)malloc(src_len+1);
    fread(src,1,src_len,src_file);
    src[src_len]='\0';
    fclose(src_file);
	
    size_t brackets[MAX_OPS];
    memset(brackets,0,MAX_OPS);
    size_t bracket_top = -1;
    // match ending brackets	
    for (int i = 0; i < src_len; i++) {
        char c = src[i];
    if (c == '[') {
        brackets[++bracket_top]=i;
    } else if (c == ']') {
        size_t open = brackets[bracket_top--];
        ops[open]=i;
        ops[i]=open;
    } else {
        ops[i]=i;
        }
    }

    if (compile)
        c_backend(src, src_len, file_path);
    else
        interpret(src, src_len);
}

void dump_cells(size_t cell_count) {
    printf("** %zu CELLS **\n", cell_count);
    for (int i = 0; i < cell_count + 1; i++){
        printf("%d ", cells[i]);
    }
    puts("");
}

void interpret(char* src, size_t src_len) {
    size_t i = 0;
    for (; i < src_len;) {
        char c = src[i];
        if (isspace(c)){
            i++;
            continue;
        }

        switch(c){
        case '+':
            cells[dp]++;
            i++;
            break;
        case '-':
            cells[dp]--;
            i++;
            break;
        case '>':
            dp += dp < MAX_CELLS ? 1 : 0;
            i++;
            break;
        case '<':
            dp -= dp > 0 ? 1 : 0;
            i++;
            break;
        case '.': {
            u8 v = cells[dp]; 
            if (v <= 127)
            printf("%c",v);
            i++;
            // TODO? special syscalls over 128?
            break;
        }
        case ',':
            if(dp < MAX_CELLS && dp >= 0)
            cells[dp]=getc(stdin);
            i++;
            break;
        case '[':
            if (cells[dp] == 0)
            i = ops[i];
            else 
            i++;
            break;
        case ']':
            if (cells[dp] == 0)
            i++;
            else  
            i = ops[i];
            break;
            // additions
        case '~':
            i++;
            dump_cells(100);
            break;
        }   

    }
}

void tcc_error_handler(void* _, const char* msg){
    printf("TCC ERROR: %s\n", msg);
}

const char* bf_header = R"(
typedef unsigned long size_t;
const size_t MAX_OPS = 1024;
const size_t MAX_CELLS = 30000;
typedef unsigned char u8;
u8 cells[30000];
size_t dp=0;
size_t ops[1024] = {)";

const char* bf_interpreter = R"(
int printf(const char*, ...);
int getchar(void);
int puts(const char*);
int isspace(int ch) {
    return  ch == ' '   ||
            ch == '\f'  ||
            ch == '\n'  ||
            ch == '\r'  ||
            ch == '\t'  ||
            ch == '\v';
}

void dump_cells(size_t cell_count) {
    printf("** %zu CELLS **\n", cell_count);
    for (int i = 0; i < cell_count + 1; i++){
        printf("%d ", cells[i]);
    }
    puts("");
}
int main() {
    for (int i = 0; i < MAX_CELLS; i ++)
        cells[i]=0;
    size_t i = 0;
	for (; i < bf_src_len;) {
		char c = bf_src[i];
        if (isspace(c)){
            i++;
            continue;
        }
            
        switch(c){
            case '+':
                cells[dp]++;
                i++;
                break;
            case '-':
                cells[dp]--;
                i++;
                break;
            case '>':
                dp += dp < MAX_CELLS ? 1 : 0;
                i++;
                break;
            case '<':
                dp -= dp > 0 ? 1 : 0;
                i++;
                break;
            case '.': {
                u8 v = cells[dp]; 
                if (v <= 127)
                    printf("%c",v);
                i++;
                // TODO? special syscalls over 128?
                break;
                }
            case ',':
                if(dp < MAX_CELLS && dp >= 0)
                    cells[dp]=getchar();
                i++;
                break;
            case '[':
                if (cells[dp] == 0)
                    i = ops[i];
                else 
                    i++;
                break;
            case ']':
                if (cells[dp] == 0)
                    i++;
            else 
                i = ops[i];
            break;
            // additions
            case '~':
                i++;
                dump_cells(100);
                break;
        }
    }
    return 0;
}
)";

void c_backend(char* src, size_t src_len, char* file_path) {
    // construct source with stringstream
    std::ostringstream bf_src;

    bf_src << bf_header;
    // create ops code
    for (auto i = 0; i < src_len; i++) {
        bf_src << ops[i] << ", ";
    }
    bf_src << "};\n";
    bf_src << "char* bf_src=\"";
    bf_src << src << "\";\n";
    bf_src << "size_t bf_src_len = " << src_len << ";\n";
    bf_src << "\n" << bf_interpreter;

    // printf("%s", bf_src.str().data());
    // compile with tcc
    TCCState* s = tcc_new();
    tcc_set_lib_path(s, "tcc");
    tcc_set_output_type(s, TCC_OUTPUT_EXE);
    tcc_set_error_func(s,NULL, tcc_error_handler);
    tcc_compile_string(s,bf_src.str().data());
    // get file name
    auto p = std::filesystem::path{ file_path };

    tcc_output_file(s, p.stem().string().data());
    tcc_delete(s);

}
