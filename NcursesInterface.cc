#include "NcursesInterface.h"
#include <iostream>
#include <sstream>
using namespace std;

bool operator<(const timeval &t1,const timeval &t2)
{
	if(t1.tv_sec<t2.tv_sec) return true;
	if(t2.tv_sec<t1.tv_sec) return false;
	if(t1.tv_usec<t2.tv_usec) return true;
	return false;
}

double difference(const timeval &t1,const timeval &t2)
{
	return (double)(t2.tv_sec-t1.tv_sec)+1e-6*(t2.tv_usec-t1.tv_usec);
}

int min(int i,int j)
{
	if(i<j) return i;
	return j;
}

int max(int i,int j)
{
	if(j<i) return i;
	return j;
}


TerminalScreen::TerminalScreen()
{
	screen_buffer=NULL;
	dirty_buffer=NULL;
	attrib_buffer=NULL;
	nrows=ncols=0;
}

TerminalScreen::~TerminalScreen()
{
	delete screen_buffer;
	delete dirty_buffer;
	delete attrib_buffer;
}

void TerminalScreen::attach_screen()
{
	delete screen_buffer;
	delete dirty_buffer;
	delete attrib_buffer;
	getmaxyx(stdscr,nrows,ncols);
	screen_buffer=new char[nrows*ncols];
	dirty_buffer=new bool[nrows*ncols];
	attrib_buffer=new int[nrows*ncols];
	clear_screen();
}

void TerminalScreen::clear_screen()
{
	for(int i=0;i<nrows;i++)
	for(int j=0;j<ncols;j++)
	{
		if((screen_buffer[i*ncols+j]!=' '))
			dirty_buffer[i*ncols+j]=true;
		screen_buffer[i*ncols+j]=' ';
		attrib_buffer[i*ncols+j]=A_NORMAL;
	}
}

void TerminalScreen::update_screen()
{
	for(int i=0;i<nrows;i++)
	for(int j=0;j<ncols;j++)
		if(dirty_buffer[i*ncols+j])
		{
			attrset(attrib_buffer[i*ncols+j]);
			mvaddch(i,j,screen_buffer[i*ncols+j]);
			dirty_buffer[i*ncols+j]=false;
		}
	refresh();
}

void TerminalScreen::print(string s,int row,int col)
{
	for(int i=0;i<(int)s.size();i++)
	{
		if(i>ncols) break;
		screen_buffer[row*ncols+col+i]=s[i];
		dirty_buffer[row*ncols+col+i]=true;
	}
}

//--------------------
/*
TypedText::TypedText()
{
	typingspeed=30;
}

void TypedText::update(TerminalScreen &screen,timeval t)
{
	double deltat=(double)(t.tv_sec-start_time.tv_sec)+1e-6*(t.tv_usec-start_time.tv_usec);
	int num_chars=typingspeed*deltat;
	for(int i=0;(i<=num_chars)&&(i<(int)text.size());i++)
	{
		if(row>screen.nrows) break;
		if(colstart+i>screen.ncols) break;
		int index=row*screen.ncols+colstart+i;
		screen.attrib_buffer[index]|=color;
		screen.attrib_buffer[index]|=A_BOLD;
		if(i!=num_chars) 
			screen.attrib_buffer[index]-=A_BOLD;
		screen.screen_buffer[index]=text[i];
		screen.dirty_buffer[index]=true;
	}
}
*/

//----------------------------------

TextBox::TextBox() 
{
	row_min=0;
	row_max=80;
	col_min=0;
	col_max=80;
	lines.push_back(string());
};

void TextBox::addchar(char c,timeval t)
{
	last_entry=t;
	if((c==127))
	{
		rmchar(t);
		return;
	}

	if(c=='\n')
		lines.push_front(string());
	else
		lines.front().append(string(1,c));
}

void TextBox::addstring(string s,timeval t)
{
	for(size_t i=0;i<s.size();i++)
		addchar(s[i],t);
}


void TextBox::rmchar(timeval t)
{
	last_entry=t;
	if(lines.front().size()!=0)
		lines.front()=lines.front().substr(0,lines.front().size()-1);
}

void TextBox::update(TerminalScreen &screen,timeval t)
{
	double deltat=(double)(t.tv_sec-last_entry.tv_sec)+1e-6*(t.tv_usec-last_entry.tv_usec);
	int onrow=row_max;
	int oncol=col_min;
	for(list<string>::iterator online=lines.begin();online!=lines.end();online++)
	{
		if((*online).size()==0) continue;
		int nrows=((*online).size()-1)/(col_max-col_min+1)+1;
		onrow-=nrows;
		int linerow=onrow;
		oncol=col_min;
		if(onrow<row_min) break;
		for(size_t i=0;i<(*online).size();i++)
		{
			if(oncol>col_max) {oncol=col_min; linerow++;}
			int index=linerow*screen.ncols+oncol;
			screen.screen_buffer[index]=(*online)[i];
			screen.attrib_buffer[index]|=color;
			screen.dirty_buffer[index]=true;
			if((online==lines.begin())&&(i==(*online).size()-1)&&((deltat<glow_timeout)))
				screen.attrib_buffer[index]|=A_BOLD;

			oncol++;
		}
	}
	/*
	for(int i=0;i<(int)text.size();i++)
	{
		if(onrow>=row_max) break;
		if(text[i]=='\n')
		{
			onrow++; oncol=col_min;
			continue;
		}
		int index=onrow*screen.ncols+oncol;
		screen.screen_buffer[index]=text[i];
		screen.attrib_buffer[index]|=color;
		screen.attrib_buffer[index]|=A_BOLD;
		if(((i+1)!=(int)text.size())||(deltat>glow_timeout))
			screen.attrib_buffer[index]-=A_BOLD;
		screen.dirty_buffer[index]=true;
		oncol++;
		if(oncol>=col_max)
		{
			onrow++; oncol=col_min;
		}

	}
	*/
} 
	
//
//------------------------------

ScreenControl::~ScreenControl()
{
	for(list<TextItem*>::iterator it=items.begin();it!=items.end();it++)
		delete (*it);
}
	
void ScreenControl::init()
{
	setlocale(LC_CTYPE, "en_US.UTF-8");
	initscr();
	curs_set(0);
	noecho();
	cbreak();
	start_color();
	screen.attach_screen();
	screen.update_screen();

}

void ScreenControl::update()
{
	screen.clear_screen();
	timeval now;
	gettimeofday(&now,NULL);
	for(list<TextItem*>::iterator it=items.begin();it!=items.end();it++)
	{
		if((*it)->isDead(now))
		{
			list<TextItem*>::iterator tokill=it;
			it--;
			delete (*tokill);
			items.erase(tokill);
		} else
			(*it)->update(screen,now);
	}
	screen.update_screen();

}

//-----------------------

TypedText::TypedText()
{
	gettimeofday(&start_time,NULL);
	death_time.tv_sec=0;
	typing_speed=30;
	color=1;
}

TypedText::~TypedText()
{
}

void TypedText::update(TerminalScreen &screen,timeval t)
{
	double deltat=difference(start_time,t);
	int n_typed=typing_speed*deltat;
	int n_erased=-2;
	if(death_time.tv_sec!=0)
	{
		double pastdeath=difference(death_time,t);
		n_erased=pastdeath*typing_speed;
	}

	for(int i=max(n_erased,0);i<min(n_typed,(int)text.size());i++)
	//for(int i=0;i<(int)text.size();i++)
	{
		int oncol=colstart+i;
		if((oncol<0)||(oncol>screen.ncols)) continue;
		if((row<0)||(row>screen.nrows)) continue;
		int index=row*screen.ncols+oncol;
		screen.screen_buffer[index]=text[i];
		screen.attrib_buffer[index]|=color;
		if(i==(n_erased)) screen.attrib_buffer[index]|=A_DIM;
		//else if(i==(n_typed-1)) screen.attrib_buffer[index]|=A_BOLD;
		else if(i==(n_typed-1)) screen.attrib_buffer[index]=COLOR_PAIR(2);
		else screen.attrib_buffer[index]|=A_BOLD;
		//else if(i==(n_typed-1)) screen.attrib_buffer[index]|=A_BOLD;
		screen.dirty_buffer[index]=true;
	}
}

bool TypedText::isDead(timeval t)
{
	if(death_time.tv_sec==0) return false;
	double deltat=(double)(t.tv_sec-death_time.tv_sec)+1e-6*(t.tv_usec-death_time.tv_usec);
	double typing_time=((double)text.size())/typing_speed;
	if(deltat>typing_time) return true;
	return false;
}
