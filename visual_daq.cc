#include "NcursesInterface.h"
#include "P8SlowLogger.hh"
#include <vector>
#include <iostream>
#include <stdlib.h>
using namespace std;

TypedText *addReading(SensorReading rdg);
double tvdiff(struct timeval a,struct timeval b);

int refreshrate;
P8SlowLogger logger;

int main(int argc,char *argv[])
{
	if(argc>1)
	{
		logger.log_file_name=string(argv[1]);
	}
	refreshrate=5;
	vector<string> sensor_names;

	logger.load_config_file("sensors.config");
	for(list<P8SlowLoggerSensor>::iterator it=logger.sensors.begin();it!=logger.sensors.end();it++)
		sensor_names.push_back((*it).name);
	logger.start_thread();

	ScreenControl screencontrol;
	screencontrol.init();
    init_pair(1,COLOR_GREEN,COLOR_BLACK);
    init_pair(2,COLOR_WHITE,COLOR_BLACK);
	init_pair(3,COLOR_WHITE,COLOR_RED);
	timeout(0);

		struct timeval now;
		gettimeofday(&now,NULL);

	vector<struct timeval> sensor_times;
	for(size_t i=0;i<sensor_names.size();i++)
	{
		sensor_times.push_back(now);
	}

	TextBox typing_box;
	typing_box.row_min=screencontrol.screen.nrows/2;
	typing_box.row_max=screencontrol.screen.nrows-1;
	typing_box.col_min=1;
	typing_box.col_max=screencontrol.screen.ncols-1;
	typing_box.glow_timeout=1.0;
	typing_box.color=COLOR_PAIR(1);
	screencontrol.items.push_back(&typing_box);


	while(1)
	{
		gettimeofday(&now,NULL);
		for(size_t i=0;i<sensor_names.size();i++)
		{
			double delta=tvdiff(now,sensor_times[i]);
			if(delta>0.5)
			{
				TypedText *tt=addReading(logger.getLastReading(sensor_names[i]));
				tt->colstart=2;
				tt->row=2+i;
				screencontrol.items.push_back(tt);
				sensor_times[i]=tt->death_time;
			}
		}
		char input;
		while((input=getch())!=ERR)
		{
			typing_box.addchar(input,now);
		}
		screencontrol.update();
		usleep(1000);
	}

	endwin();
}


TypedText *addReading(SensorReading rdg)
{
	TypedText *ret=new TypedText();
//	stringstream ss;
//	ss << rdg;
//	ret->text=ss.str();
	ret->text=logger.printSensorReading(rdg);
	gettimeofday(&ret->start_time,NULL);
	ret->death_time=ret->start_time;
	ret->death_time.tv_sec+=refreshrate;
	ret->death_time.tv_usec+=(int)(1000000*((double)rand())/((double)RAND_MAX));
	if(!rdg.has_error)
		ret->color=COLOR_PAIR(1);
	else
		ret->color=COLOR_PAIR(3);
	return ret;
}

double tvdiff(struct timeval a,struct timeval b)
{
	return ((double)a.tv_sec-(double)b.tv_sec)+((double)a.tv_usec-(double)b.tv_usec)/1000000;
}
