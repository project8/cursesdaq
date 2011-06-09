#include "P8SlowLogger.hh"

int main(int argc,char *argv[])
{
	P8SlowLogger logger;
	logger.load_config_file("sensors.config");
	logger.start_thread();
	int r;
	logger.join(r);
	return r;
	

}
