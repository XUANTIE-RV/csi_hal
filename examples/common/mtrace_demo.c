#define DEBUG_MTRACE
#include <stdlib.h>	// for malloc
#include <mcheck.h>	// for mtrace() / muntrace();

#define LOG_LEVEL 2
#include <syslog.h>

int main(int argc, char **argv)
{
	LOG_I("Enter\n");
	int ret;

#ifdef DEBUG_MTRACE
	mtrace();
#endif

	int *tmp = malloc(0x1234);
	LOG_I("malloc 0x1234 bytes(%p) without free.\n", tmp);

#ifdef DEBUG_MTRACE
	muntrace();
#endif
	LOG_I("Exit\n");
}

