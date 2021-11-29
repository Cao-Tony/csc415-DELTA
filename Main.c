#include "fsMain.h"

// mod(pi,2)
#define MAGIC_NUMBER_CHAR "1141592653"

int main(int argc, char *argv[])
{
	fsBoot(argc, argv);
	fsShutDown();
	return 0;
}