export LIBGL_ALWAYS_SOFTWARE=1

PORT=10666
HOST=totaltrash.org
HOST=127.0.0.1

# gdb -ex run --args cbuild/doom2k -nomouse -net $HOST:10666
# valgrind cbuild/doom2k -nomouse -net $HOST:10666
cbuild/doom2k -nomouse -net $HOST:$PORT

