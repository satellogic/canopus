#include <canopus/drivers/simusat/memhooks.h>

extern int app_main( void );

int
main(int argc, char **argv)
{
	memhooks_init(MEMHOOKS_TMS570LS3137);

	return app_main();
}
