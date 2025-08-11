# NovaC

includes the NovaC language and the COMET transcomplier

NovaC language was a language I made for fun.
It is basically C but easier to read.
Each line has a prefix;
- / for a basic command (e.g. /return 0; , /printf ("Hello world!); )
- | for a comment (e.g. | fix this later , | This prints hello world )
- @ for an entry point (e.g. @ int Main() , @ static void ...)
- $ for a variable declaration (e.g. $ int money = 1000000; , $ float Pi = 3.14; ) 
- + for an include (e.g. +include stdio , +define MAX 100 (elements of headers like the <> signs and the .h are removed))

Formating: 
Instead of the usual curly brackets, dashes will be used.

/ → Start block

// → Case/mid-block label

/// → Inner body

\ → Close block

Therefore, a Hello world! program would be:

+include stdio

@int Main()
 /printf ("Hello world!);
 /return 0;
\

# COMET Compiler

The NovaC programming language uses the .rc file extension (stands for readable C)
The COMET Compiler is written in C and will transcompile your NovaC code
It basically translates your NovaC code into C
Your .rc file must be in the same folder (directory as COMET)
In order to compile it, write:

------

Linux:

(Open your console)

cd <directory where COMET is installed>
./COMET program.rc program.c

Windows:

(Use either CMD or Powershell)

cd <directory where COMET is installed>
COMET program.rc -o program.c

MacOS:

MacOS isn't supported, your fault for getting a Mac.

-----




