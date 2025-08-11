# NovaC

includes the NovaC language and the COMET transcomplier

NovaC language is a language I made for fun. 

It is inspired by the console commands in Minecraft. 

It is basically C but its more of a markdown style language that is easier to write and read.

Insted of the curly bracket language C is, this language uses dashes in body formatting.

# The basics

NovaC is a language very similar to C.
If you know C, this language will be easy to use.

NovaC uses the same (or, at least very similar) :

-Syntax

-Logic

-Constucts


The file extension NovaC uses is .rc (Readable C)

Each line has a prefix;
- / for a basic command (e.g. /return 0; , /printf ("Hello world!); )
- | for a comment (e.g. | fix this later , | This prints hello world )
- @ for an entry point (e.g. @ int Main() , @ static void ...)
- $ for a variable declaration (e.g. $ int money = 1000000; , $ float Pi = 3.14; ) 
- (+) for an include (e.g. +include stdio , +define MAX 100 (elements of headers like the <> signs and the .h are removed))

Formating: 
Instead of the usual curly brackets, dashes will be used.

/ → Start block

// → Case/mid-block label

/// → Inner body

\ → Close block

Therefore, a Hello world program would be:

+include stdio

@int Main()

 /printf ("Hello world!);
 
 /return 0;
 
\

# COMET Compiler

The COMET Compiler is written in C and  transcompiles your NovaC code into C code ( .rc -> .c )
It basically translates your NovaC code into C, that you can compile with your C compiler (GCC integration coming soon)

How to compile it = 

Your .rc file must be in the same folder (directory as COMET)
Console Commands:

------

Linux:

(Open your console)

cd <-directory where COMET is installed->

./COMET program.rc program.c

> Your .c file will spawn in the same directory, ready for compiling

------

Windows:

(Use either CMD or Powershell)

Write:

cd <-directory where COMET is installed->

COMET program.rc -o program.c

> Your .c file will spawn in the same directory, ready for compiling

------

MacOS:

MacOS isn't supported, your fault for getting a Mac.

-----




