# Getting started.
## compiling
```bash
gcc -Wall -o server server.c handle_file.c string_functions.c
```
## Then run it with.
```bash
./server
```
## Or with valgrind to make sure it is running correctly.
```bash
valgrind ./server --leak-check=full
```
# This uses nextjs inspired file based routing
<pre>
./pages/index.html      = /
./pages/help.html       = /help 
./pages/test/index.html = /test
./pages/test/thing.html = /test/thing
</pre>

## Also very basic templating so that there is a layout file, and the content of page files is popped in.

# Next steps
- add more templating options
- maybe make a standard way to insert templates into the file maybe like flask's {{}}.
- look into making this follow the mvc pattern.
- refactor more to make it easier to follow.