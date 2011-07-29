//JSON.hh
//JSON interface for C++
//Gray Rybka 7/2/2011
//see json_self_test() in JSON.cc for usage example
#include <string>
#include <map>
#include <iostream>
using namespace std;

namespace JSON {

class JSONValue;

class JSONObject : public map<string,JSONValue> {
public:
	string toString(int indent=0) const;
	void fromString(const string &s);
};

class JSONValue;

class JSONArray {
public:
	JSONArray();
	JSONArray(const JSONArray &a);
	~JSONArray();

	JSONValue &operator[](const int &i);
	const JSONValue &operator[](const int &i) const;
	int length() const {return n_elements;};
	bool operator<(const JSONArray &a) const;
	bool operator==(const JSONArray &a) const;
	JSONArray &operator=(const JSONArray &a);
	void resize(int maxelems);
	void append(const JSONValue &val);

	string toString(int indent=0) const;


private:
	JSONValue *elements;
	int n_elements;

};

enum JSONValueType {STRING,INTEGER,DOUBLE,ARRAY,OBJECT,BOOLEAN,JSONNULL};

class JSONValue {
public:
	JSONValue();
	JSONValue(const JSONValue &v);
	JSONValue(const string &s);
	JSONValue(const long long &i);
	JSONValue(const int &i);
	JSONValue(const double &d);
	JSONValue(const JSONArray &arr);
	//if bool is a constructer, it will be the default one. you don't want that

	~JSONValue();
	JSONValue &operator=(const JSONValue &v);
	bool operator==(const JSONValue &v) const;
	bool operator<(const JSONValue &v) const;

	void setStringValue(const string &s);
	void setIntValue(long long i);
	void setDoubleValue(double d);
	void setArrayValue(const JSONArray &r);
	void setObjectValue(const JSONObject &o);
	void setBoolValue(const bool &b);
	void setNull();

	const string & getStringValue() const;
	string & getStringValue() ;
	const long long &getIntValue() const;
	long long &getIntValue() ;
	const double &getDoubleValue() const;
	double &getDoubleValue() ;
	const JSONArray &getArrayValue() const;
	JSONArray &getArrayValue();
	const JSONObject &getObjectValue() const;
	JSONObject &getObjectValue();
	const bool &getBoolValue() const;
	bool &getBoolValue();
	bool isNull() const;
	bool isBoolTrue() const; //false if not a bool, no error
	bool isBoolFalse() const; //false if not a bool, no error

	JSONValueType getType() const {return type;};
	string getTypeAsString() const;
	void fromToken(string tok); //turn a token(string, int, double) into a value
	string toString(int indent=0) const;

private:
	void *data;
	JSONValueType type;

	void delete_data();

};

//stream operators
ostream &operator<<(ostream &out,const JSONValue &val);
ostream &operator<<(ostream &out,const JSONArray &arr);
ostream &operator<<(ostream &out,const JSONObject &obj);
istream &operator>>(istream &in,JSONValue &val);
istream &operator>>(istream &in,JSONArray &val); //assumes first [ is off
istream &operator>>(istream &in,JSONObject &obj); //assumes first { is off

void json_self_test();
}
