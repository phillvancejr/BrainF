#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <sstream>
#include "tinycc/libtcc.h"
#include <filesystem>
#include <fstream>
// to get path to exe on mac
#include <mach-o/dyld.h>
// get mac os version for lld
// #include <sys/sysctl.h>

#define MAX_CELLS 30000

typedef unsigned char u8;
u8 cells[MAX_CELLS];
size_t dp = 0;


#define MAX_OPS 1024
size_t ops[MAX_OPS];

void dump_cells(size_t cell_count);
void interpret(char* src, size_t src_len);
void c_backend(char* src, size_t src_len, char* file_path);
void yasm_64_mac(char* src, size_t src_len, char* file_path);

// mac specific
std::string path_to_exe() {
    std::string exe_path;
    char buf[PATH_MAX];
    uint32_t buf_size = PATH_MAX;
    if ( _NSGetExecutablePath(buf, &buf_size) == 0 )
        exe_path += buf;
    return exe_path;
}

void help(){
    puts(R"(Usage: bf [options] file_path

Options: 
    -h, --help      Display this message
    -c              Compile to executable via TCC for Mac or Linux
    -mac            Compile to executable via Yasm for x64 Mac
)");
}

bool is_bf_file(char* f) {
    auto len = strlen(f);
    return  f[len-3] == '.' &&
            f[len-2] == 'b' &&
            f[len-1] == 'f';
}

std::string exe_dir;
std::string yasm_path;

int main(int argc, char** argv) {
    exe_dir = std::filesystem::path{ path_to_exe() }.parent_path().string();
    yasm_path = exe_dir + "/backends/yasm";

    char* file_path;
    bool compile = false;
    bool native_mac = false;

    if (argc == 3) {
        auto arg = argv[1];
        if (strcmp(arg, "-h") == 0 || strcmp(arg, "--help") == 0){
            help();
            return 0;
        } 
        else if (strcmp(arg, "-c") == 0){
            compile = true;
        }
        else if (strcmp(arg, "-mac") == 0){
            native_mac = true;
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
    else if(native_mac)
        yasm_64_mac(src, src_len, file_path);
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
    auto backend_folder = exe_dir + "/backends";
    tcc_set_lib_path(s, backend_folder.data());
    tcc_set_output_type(s, TCC_OUTPUT_EXE);
    tcc_set_error_func(s,NULL, tcc_error_handler);
    tcc_compile_string(s,bf_src.str().data());
    // get file name
    auto p = std::filesystem::path{ file_path };

    tcc_output_file(s, p.stem().string().data());
    tcc_delete(s);

}

#include <iostream>
using std::cout;
#define nl "\n"

auto yasm_header = R"(
; assembly bf interpreter for compiling to binary
segment .text
; IMPORTS
extern _printf
extern _getchar
extern _puts
extern _isspace
extern _stdin

; EXPORTS
global _main
; PROGRAM
MAX_OPS equ 1024
MAX_CELLS equ 30000
DUMP_CELL_COUNT equ 100

%define restore_stack add rsp, rdx

%macro debug_char 1
    lea rdi, [rel debug_fmt]
    mov sil, %1
    call _printf
%endmacro

_main:
    push rbp ; align stack
interp:
    ;debug_char '@'
    ; exit if src_index >= bf_src_len
    cmp qword[rel src_index], bf_src_len
    jge exit

    ; char c = bf_src[src_index]
    lea rbx, [rel bf_src]
    add rbx, [rel src_index]
    mov dl, [rbx]
    xor rbx, rbx
    mov bl, dl ; use bl for char because rbx is preserved across c calls

    xor rax, rax
    ; check if char is space
    mov edi, ebx
    call _isspace
    ; if its a space go to switch statement
    cmp eax, 0

    je switch

    ; if space inc rcx and continue loop
    is_space: 
        inc qword[rel src_index]
        jmp interp

    switch:
    plus:
        cmp bl, '+'
        jne .next

        ;debug_char '+'

        ; index cells
        lea rbx, [rel cells]
        add rbx, [rel dp]
        ; increment byte in cell
        inc byte [rbx]

        inc qword[rel src_index]
        jmp break
        .next:
    minus:
        cmp bl, '-'
        jne .next

        ;debug_char '-'

        ; index cells
        lea rbx, [rel cells]
        add rbx, [rel dp]
        ; decrement byte in cell
        dec byte [rbx]

        inc qword[rel src_index]
        jmp break
        .next:
    greater: ; next cell
        cmp bl, '>'
        jne .next

        ;debug_char '>'

        ; zero rax
        xor rax, rax
        ; dp < MAX_CELLS?
        cmp qword [rel dp], MAX_CELLS-1
        ; if less than MAX_CELLS-1 set to 1
        mov rbx, 1
        cmovl rax, rbx

        add [rel dp], rax

        inc qword[rel src_index]
        jmp break
        .next:
    less: ; previous cell
        cmp bl, '<'
        jne .next
        ; zero rax

        ;debug_char '<'

        xor rax, rax
        cmp qword [rel dp], 0
        ; if greater than 0 set to 1
        mov rbx, 1
        cmovg rax, rbx
        ; sub from dp
        sub [rel dp], rax

        inc qword[rel src_index]
        jmp break
        .next:
    dot: ; print cell
        cmp bl, '.'
        jne .next

        ;debug_char '.'

        lea rdi, [rel print_fmt]
        ; load cells
        lea rsi, [rel cells]
        ; index cells
        add rsi, [rel dp]
        ; get byte from cell
        mov sil, [rsi]
        call _printf

        inc qword[rel src_index]
        jmp break
        .next:
    comma: ; get char from stdin
        cmp bl, ','
        jne .next

        ;debug_char ','

        ; cells[dp]=getchar()
        ; get from stdin
        call _getchar
        lea rbx, [rel cells]
        add rbx, [rel dp]
        ; store in cells[dp]
        mov [rbx], al

        inc qword[rel src_index]
        jmp break
        .next:
    left_bracket: ; [
        cmp bl, '['
        jne .next

        ;debug_char '['

        ; load ops[src_index]
        mov rcx, [rel src_index]
        lea rbx, [rel ops]
        mov rbx, [rbx + rcx*8]
        ; src_index +1
        inc rcx
        ; rbx = ops[src_index]
        ; rcx = src_index + 1

        ; rdx will hold new position
        mov rdx, rbx

        ; get current cell
        lea rsi, [rel cells] 
        add rsi, [rel dp]
        mov al, [rsi]
        ; if rax (aka cell) == 0
        ;   src_index = rbx ( aka ops[src_index] )
        ; else
        ;   src_index = rcx ( src_index + 1 )
        cmp al, 0
        cmovne rdx, rcx

        ; src_index = rdx
        mov [rel src_index], rdx

        jmp break
        .next:
    right_bracket: ; ]
        cmp bl, ']'
        jne .next

        ;debug_char ']'

        ; load ops[src_index]
        mov rcx, [rel src_index]
        lea rbx, [rel ops]
        mov rbx, [rbx + rcx*8] ; &ops + src_index * 8, 8 because ops is qword
 
        ; src_index +1
        inc rcx
        ; rbx = ops[src_index]
        ; rcx = src_index + 1

        ; rdx will hold new position
        mov rdx, rcx

        ; get current cell
        lea rsi, [rel cells] 
        add rsi, [rel dp]
        mov al, [rsi]
        ; if rax (aka cell) == 0
        ;   src_index = rcx ( src_index + 1 )
        ; else
        ;   src_index = rbx ( aka ops[src_index] )
        cmp al, 0
 
        cmovne rdx, rbx

        ; src_index = rdx
        mov [rel src_index], rdx

        jmp break
        .next:
    tilde: ; dump cells
        cmp bl, '~'
        jne .next

        ;debug_char '~'


        ; cells header
        lea rdi, [rel dump_cells_header]
        mov rsi, DUMP_CELL_COUNT
        call _printf

        ; use local variable
        sub rsp, 16 ; sub 16 to keep stack aligned
        %define print_index [rsp + 8]
        mov qword print_index, 0

        .print_cells:
            lea rdi, [rel dump_cells_fmt]
            ; char c = cells[print_index]
            lea rsi, [rel cells]
            add rsi, print_index
            mov al, [rsi]
            mov rsi, rax

            call _printf
 
            inc qword print_index
            cmp qword print_index, DUMP_CELL_COUNT + 1
            jl .print_cells

        lea rdi, [rel puts_empty]
        call _puts

        inc qword[ rel src_index]

        add rsp, 16 ; restore stack
        jmp break
        .next:
    break:
    ;loop code
    cmp qword [rel src_index], bf_src_len
    jl interp


exit:
    mov rdi, rcx
    mov rax,0x2000001
    syscall
; dump_cells function to display cells
dump_cells:
    push rcx ; save rcx and align stack
    lea rdi, [rel dump_cells_header]
    mov rsi, DUMP_CELL_COUNT
    call _printf

    xor rcx, rcx
.print_cells:
    lea rdi, [rel dump_cells_fmt]
    ; load cells
    lea rsi, [rel cells]
    add rsi, rcx
    ; get cell value
    mov sil, [rsi]

    push rcx ; align stack and preserve rcx because printf might change it
    push rcx
    call _printf
    pop rcx
    pop rcx

    inc rcx

    cmp rcx, DUMP_CELL_COUNT + 1
    jl .print_cells

    lea rdi, [rel puts_empty]
    call _puts
    pop rcx
    ret

segment .bss
src_index: resq 1 ; index for loop
; print_index: resq 1 ; index for dumping cells
dp: resq 1
cells: resb MAX_CELLS

segment .data
bf_src: db )";

void yasm_64_mac(char* src, size_t src_len, char* file_path) {
    // file name
    auto file_name  = std::filesystem::path{ file_path }.stem().string();
    // cout << "FILE NAME " << file_name << nl;

    // cout << "YASM PATH " << yasm_path << nl;
    std::string cmd = yasm_path + " -f macho64 " + file_name + ".s";

    // these lines use llvm's linker to do the linking, however you need the lldb linker 104 KB + libLLVM.dylib 104.6MB
    // char mac_version[256];
    // auto size = sizeof(mac_version);
    // sysctlbyname("kern.osproductversion", mac_version, &size, 0, 0);
    //cmd += " && /usr/local/Cellar/llvm/13.0.1_1/bin/lld -flavor darwin -lSystem -arch x86_64 -platform_version macos " + std::string{mac_version} + " " + std::string{mac_version} + " " + file_name + ".o -o " + file_name;
    cmd += " && ld -macosx_version_min 10.10 -lSystem " + file_name + ".o -o " + file_name ;


    // cout << "CMD: " << cmd << nl;

    std::ostringstream yasm_code;
    yasm_code << yasm_header;

    // should probably do thes two loops together, but too lazy
    // add in bf program
    // yasm_code << "\"";
    for (auto i = 0; i < src_len; i++){
        yasm_code << (int)src[i];
        yasm_code << (i != src_len - 1 ? "," : "\n");
    }
    // add src_len caculating code
    yasm_code << "bf_src_len equ $-bf_src\n";
    // add in ops indices
    yasm_code << "ops: dq ";
    for (auto i = 0; i < src_len; i++) {
        yasm_code << ops[i];

        yasm_code << (i != src_len - 1 ? "," : "\n");
    }
    // add fmt strings
    yasm_code << R"(; strings
dump_cells_header: db '** %zu CELLS **',10,0
dump_cells_fmt: db "%d ",0
puts_empty: db "",0
print_fmt: db "%c",0
)";


    // write assembly file
    {
        std::ofstream out{ file_name + ".s" };
        out << yasm_code.str();
    }

    // call cmd
    system(cmd.data());
    // remove o and s files
    std::string remove = "rm " + file_name + ".o " + file_name + ".s";
    system(remove.data());
}

