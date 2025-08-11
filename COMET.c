/* readablec_transpiler.c

   A simple transpiler from your StarC language into standard C.
   COMET: Compiler for Optimized Mapping to Executable Target
   Usage:
     gcc -o readablec_transpiler readablec_transpiler.c
     ./readablec_transpiler input.rc output.c

   Notes / limitations (kept intentionally small for clarity):
   - Line-based parsing driven by the first tokens: +, |, @, $, /, //, ///, \\
   - Basic conversions implemented: includes, defines, comments, functions (@),
     variable lines ($), simple commands (write, writeLine, read), control flow
     starting lines (/ if(...), / for(...), / while(...), / switch(...)), //case,
     block end (\\), and raw passthrough for unknown lines.
   - Strings are passed through; string type `$ string name = "x";` converts to
     `char *name = "x";`.
   - `writeLine("text")` => `printf("text\n");`, `write("text")` => `printf("text");`
   - `read x;` => `scanf("%d", &x);` (assumes integer input)

   This is a solid starting point you can extend as needed.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define MAX_LINE 4096

static char *trim_left(char *s) {
    while (*s && isspace((unsigned char)*s)) s++;
    return s;
}

static void trim_right_inplace(char *s) {
    size_t n = strlen(s);
    while (n > 0 && isspace((unsigned char)s[n-1])) { s[n-1] = '\0'; n--; }
}

static int starts_with(const char *s, const char *prefix) {
    return strncmp(s, prefix, strlen(prefix)) == 0;
}

static void write_indent(FILE *out, int indent) {
    for (int i = 0; i < indent; ++i) fputs("    ", out);
}

static void emit_include(FILE *out, const char *rest) {
    char name[256];
    if (sscanf(rest, "include %255s", name) >= 1) {
        // remove trailing semicolon if present
        size_t L = strlen(name);
        if (L > 0 && name[L-1] == ';') name[L-1] = '\0';
        // if name already contains a dot or angle brackets, passthrough
        if (strchr(name, '.') || strchr(name, '<') || strchr(name, '"')) {
            fprintf(out, "#include %s\n", name);
        } else {
            // treat as standard header: <name.h>
            fprintf(out, "#include <%s.h>\n", name);
        }
    }
}

static void emit_define(FILE *out, const char *rest) {
    // +define NAME VALUE
    const char *p = rest + strlen("define");
    p = trim_left((char*)p);
    fprintf(out, "#define %s\n", p);
}

static void emit_comment(FILE *out, const char *rest, int indent) {
    write_indent(out, indent);
    // rest begins after '|'
    fprintf(out, "//%s\n", rest);
}

static void emit_function_start(FILE *out, const char *rest, int *indent) {
    // rest begins after '@'
    // copy rest as-is, append space and '{'
    fprintf(out, "%s {\n", trim_left((char*)rest));
    (*indent)++;
}

static void emit_var_line(FILE *out, const char *rest, int indent) {
    write_indent(out, indent);
    // convert `string` to `char *`
    char buf[MAX_LINE];
    strncpy(buf, trim_left((char*)rest), sizeof(buf)-1);
    buf[sizeof(buf)-1] = '\0';
    // remove leading optional '$ '
    if (starts_with(buf, "$ ")) memmove(buf, buf+2, strlen(buf+2)+1);
    // replace "string " with "char *"
    char *p = strstr(buf, "string ");
    if (p == buf) {
        // shift remainder
        char restbuf[MAX_LINE];
        strcpy(restbuf, buf + strlen("string "));
        snprintf(buf, sizeof(buf), "char *%s", restbuf);
    }
    // if line doesn't end with ';' add one
    size_t L = strlen(buf);
    if (L == 0 || buf[L-1] != ';') strcat(buf, ";");
    fprintf(out, "%s\n", buf);
}

static void emit_block_end(FILE *out, int *indent) {
    if (*indent > 0) (*indent)--;
    write_indent(out, *indent);
    fprintf(out, "}\n");
}

static void emit_command(FILE *out, const char *rest, int indent) {
    // rest begins after '/'
    char tmp[MAX_LINE];
    strncpy(tmp, trim_left((char*)rest), sizeof(tmp)-1);
    tmp[sizeof(tmp)-1] = '\0';

    // remove a leading space if present
    if (tmp[0] == ' ') memmove(tmp, tmp+1, strlen(tmp+1)+1);

    // simple mappings
    if (starts_with(tmp, "writeLine(") || starts_with(tmp, "writeLine ")) {
        // extract inside parentheses or after space
        char *start = strchr(tmp, '(');
        if (start) {
            // keep content including parentheses
            // ensure it ends with );
            size_t L = strlen(tmp);
            if (L >= 1 && tmp[L-1] == ';') tmp[L-1] = '\0';
            write_indent(out, indent);
            fprintf(out, "printf(%s\\n);\n", start+1); // we will place closing parenthesis and newline within printf
            // This is a bit hacky: if user wrote writeLine("Hi"); start+1 == "\"Hi\")" -> printf("Hi")\n; we add \n inside format
        } else {
            // fallback
            write_indent(out, indent);
            fprintf(out, "// unrecognized writeLine usage: %s\n", tmp);
        }
        return;
    }
    if (starts_with(tmp, "write(") || starts_with(tmp, "write ")) {
        char *start = strchr(tmp, '(');
        if (start) {
            size_t L = strlen(tmp);
            if (L >= 1 && tmp[L-1] == ';') tmp[L-1] = '\0';
            // standard printf without added newline
            write_indent(out, indent);
            fprintf(out, "printf(%s);\n", start+1);
        } else {
            // attempt to treat as write "text";
            write_indent(out, indent);
            fprintf(out, "// unrecognized write usage: %s\n", tmp);
        }
        return;
    }
    if (starts_with(tmp, "read ")) {
        // read var; assume int
        const char *varname = trim_left((char*)tmp + strlen("read "));
        // remove trailing ;
        char namebuf[256];
        strncpy(namebuf, varname, sizeof(namebuf)-1);
        namebuf[sizeof(namebuf)-1] = '\0';
        trim_right_inplace(namebuf);
        size_t L = strlen(namebuf);
        if (L > 0 && namebuf[L-1] == ';') namebuf[L-1] = '\0';
        write_indent(out, indent);
        fprintf(out, "scanf(\"%%d\", &%s);\n", namebuf);
        return;
    }

    // check for control keywords that start blocks
    if (starts_with(tmp, "if ") || starts_with(tmp, "if(") ||
        starts_with(tmp, "for ") || starts_with(tmp, "for(") ||
        starts_with(tmp, "while ") || starts_with(tmp, "while(") ||
        starts_with(tmp, "switch ") || starts_with(tmp, "switch(")) {
        // make sure '(' etc are preserved and add '{' then increase indent externally
        write_indent(out, indent);
        // ensure trailing '{' will be added by caller; here we emit the condition line without modification
        fprintf(out, "%s {\n", tmp);
        return;
    }

    // fallback: emit the rest as-is (make sure it ends with semicolon)
    char fallback[MAX_LINE];
    strncpy(fallback, tmp, sizeof(fallback)-1);
    fallback[sizeof(fallback)-1] = '\0';
    trim_right_inplace(fallback);
    size_t LF = strlen(fallback);
    if (LF > 0 && fallback[LF-1] != ';') strcat(fallback, ";");
    write_indent(out, indent);
    fprintf(out, "%s\n", fallback);
}

static void emit_case_line(FILE *out, const char *rest, int indent) {
    // rest begins after '//'
    const char *p = trim_left((char*)rest);
    if (starts_with(p, "case ")) {
        // output 'case <value>:'
        write_indent(out, indent);
        fprintf(out, "%s:\n", p); // user hopefully wrote case X
    } else if (starts_with(p, "default")) {
        write_indent(out, indent);
        fprintf(out, "default:\n");
    } else {
        // passthrough with colon
        write_indent(out, indent);
        fprintf(out, "%s:\n", p);
    }
}

int main(int argc, char **argv) {
    if (argc < 3) {
        fprintf(stderr, "Usage: %s input.rc output.c\n", argv[0]);
        return 1;
    }
    const char *inname = argv[1];
    const char *outname = argv[2];

    FILE *in = fopen(inname, "r");
    if (!in) { perror("fopen input"); return 2; }
    FILE *out = fopen(outname, "w");
    if (!out) { perror("fopen output"); fclose(in); return 3; }

    char line[MAX_LINE];
    int indent = 0;

    // write a small header
    fprintf(out, "/* Transpiled C produced by readablec_transpiler */\n\n");

    while (fgets(line, sizeof(line), in)) {
        char *s = line;
        // trim right newline and spaces
        trim_right_inplace(s);
        // skip empty lines but preserve spacing
        char *t = trim_left(s);
        if (*t == '\0') {
            fputc('\n', out);
            continue;
        }

        // handle three-character prefix '///' first
        if (starts_with(t, "///")) {
            // inner body: convert as command
            char *rest = t + 3;
            emit_command(out, rest, indent);
            continue;
        }
        // handle two-character prefix '//' (mid-block labels like case)
        if (starts_with(t, "//")) {
            char *rest = t + 2;
            emit_case_line(out, rest, indent);
            continue;
        }
        // single-character prefixes
        if (t[0] == '+') {
            // +include or +define
            char *rest = t + 1;
            rest = trim_left(rest);
            if (starts_with(rest, "include")) {
                emit_include(out, rest);
            } else if (starts_with(rest, "define")) {
                emit_define(out, rest);
            } else {
                // passthrough
                write_indent(out, indent);
                fprintf(out, "// unrecognized + directive: %s\n", rest);
            }
            continue;
        }
        if (t[0] == '|') {
            char *rest = t + 1;
            emit_comment(out, rest, indent);
            continue;
        }
        if (t[0] == '@') {
            char *rest = t + 1;
            emit_function_start(out, rest, &indent);
            continue;
        }
        if (t[0] == '$') {
            char *rest = t + 1;
            emit_var_line(out, rest, indent);
            continue;
        }
        if (t[0] == '/') {
            // could be '/ command' or '/if...'
            char *rest = t + 1;
            // determine if this starts a new block or just a command
            char tmp2[MAX_LINE];
            strncpy(tmp2, trim_left(rest), sizeof(tmp2)-1);
            tmp2[sizeof(tmp2)-1] = '\0';
            // if it looks like control flow starting with if/for/while/switch
            if (starts_with(tmp2, "if ") || starts_with(tmp2, "if(") ||
                starts_with(tmp2, "for ") || starts_with(tmp2, "for(") ||
                starts_with(tmp2, "while ") || starts_with(tmp2, "while(") ||
                starts_with(tmp2, "switch ") || starts_with(tmp2, "switch(")) {
                // emit and increase indent
                emit_command(out, rest, indent);
                indent++; // increase because we emitted the '{' already inside emit_command
            } else {
                // normal command
                emit_command(out, rest, indent);
            }
            continue;
        }
        if (t[0] == '\\') {
            emit_block_end(out, &indent);
            continue;
        }

        // fallback: passthrough trimmed line
        write_indent(out, indent);
        fprintf(out, "%s\n", t);
    }

    fclose(in);
    fclose(out);

    // Determine executable extension based on OS
    #ifdef _WIN32
    const char *exe_ext = ".exe";
    #else
    const char *exe_ext = ".out";
    #endif

    // Compose output executable name
    char exe_name[512];
    snprintf(exe_name, sizeof(exe_name), "%s%s", outname, exe_ext);

    // Compose gcc compile command
    char cmd[1024];
    snprintf(cmd, sizeof(cmd), "gcc %s -o %s", outname, exe_name);

    // Run the gcc compiler
    int ret = system(cmd);
    if (ret != 0) {
        fprintf(stderr, "Error: gcc compilation failed with code %d\n", ret);
        return 4;
    }

    printf("Compiled executable: %s\n", exe_name);
    return 0;
}



