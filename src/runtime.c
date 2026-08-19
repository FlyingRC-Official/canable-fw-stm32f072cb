#include <errno.h>
#include <stddef.h>

extern char _heap_start;
extern char _heap_end;

void _init(void) { }
void _fini(void) { }

void *_sbrk(ptrdiff_t increment)
{
    static char *current = &_heap_start;
    char *previous = current;

    if (increment < 0 || current + increment > &_heap_end) {
        errno = ENOMEM;
        return (void *)-1;
    }
    current += increment;
    return previous;
}
