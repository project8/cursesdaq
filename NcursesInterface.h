#include <curses.h>
#include <string>
#include <list>
#include <sys/time.h>
using namespace std;

class TerminalScreen
{
public:
	TerminalScreen();
	~TerminalScreen();

	//things that affect the buffer only
	void attach_screen();
	void clear_screen();
	void print(string s,int row,int col);
	//things that actually affect the screen
	void update_screen();

	char *screen_buffer;
	bool *dirty_buffer;
	int *attrib_buffer;
	int nrows;
	int ncols;
};

/*
class TypedText
{
public:
	TypedText();

	int row;
	int colstart;
	int color;
	string text;
	double typingspeed; //in characters per second
	timeval start_time;

	void update(TerminalScreen &screen,timeval t);
};
*/

class TextItem
{
public:
	virtual void update(TerminalScreen &screen,timeval t)=0;
	virtual bool isDead(timeval t) {return false;};
};

class TextBox : public TextItem
{
public:
	TextBox();
	int row_min;
	int row_max;
	int col_min;
	int col_max;
//	string text;
	int color;

	list<string> lines;
	
	timeval last_entry;
	double glow_timeout;
	
	void addchar(char c,timeval t);
	void addstring(string s,timeval t);
	void rmchar(timeval t);
	void update(TerminalScreen &screen,timeval t);
};


class ScreenControl
{
public:
	~ScreenControl();
	void init();
	void update();

	TerminalScreen screen;

	list<TextItem*> items;
};

class TypedText : public TextItem
{
public:
	TypedText();
	virtual ~TypedText();

	string text;
	double typing_speed; //in characters per second
	timeval start_time;
	timeval death_time;
	int row;
	int colstart;
	int color;

	void update(TerminalScreen &screen,timeval t);
	bool isDead(timeval t);
};
