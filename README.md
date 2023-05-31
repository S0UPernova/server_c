# Getting started.
## compiling
```bash
gcc -Wall server.c -o server
```
## Then run it with.
```bash
./server
```
# This uses nextjs inspired file based routing
<pre>
./pages/index.html      = /
./pages/help.html       = /help 
./pages/test/index.html = /test
./pages/test/thing.html = /test/thing
</pre>

# Also very basic templating so that there is a layout file, and the content of page files is popped in.