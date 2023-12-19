#include <iostream> 
#include <cstdio>
#include <string>
#include <vector>
#include <map>
#include <unordered_map>
#include <tuple>
#include <exception>
#include <error.h>
#include <sqlite3.h> 
#include <cassert>

#include "sqlite.hpp"

class MyClass
{
public:
    typedef int INT;
    typedef char CHAR;
    enum class FLAGS {one=10, two=20, three=30};
    MyClass() {}
    ~MyClass() {}
};

enum class FLAGS {one=1, two, three};

static int callback(void* data, int Cols, char** ColValues, char** ColNames) 
{ 
	int i; 
	fprintf(stderr, "%s: ", (const char*)data); 

	for (i = 0; i < Cols; i++) { 
		printf("%s = %s\n", ColNames[i], ColValues[i] ? ColValues[i] : "NULL"); 
	} 

	printf("\n"); 
	return 0; 
} 

using std::tuple;

class MyException: public std::exception
{
	const char *msg;
public:
	MyException(const char *msg__) : msg(msg__) {}
	~MyException() {}
	virtual const char* what() const noexcept
	{ return msg; }
};

/**
SQLITE_TRANSIENT tells SQLite to copy your string. Use this when your string('s buffer) is going to go away before the query is executed.

SQLITE_STATIC tells SQLite that you promise that the pointer you pass to the string will be valid until after the query is executed. 
Use this when your buffer is static, or at least has dynamic scope that exceeds that of the binding.

--Doug Currie
*/

using std::cout;
using std::endl;
using std::vector;
using std::string;
using std::pair;
using std::map;
using std::unordered_map;
using std::tuple;

// convenience macros and typedef's
#define sqlite cxx_sqlite3
using error_code_t = sqlite::error_code_t;
using sqlite::error;

int main(int argc, char** argv) 
{ 
   	/*throw MyException("This is a custom exception");
	//throw std::runtime_error("this is an exception");
	exit(0);*/

	/*
	vector<SQLITE3_OINT> vals1{1, 2, 3};
	//vector<SQLITE3_OGEN> vals2{1, 2, 3};
	SQLITE3_OINT O1{1};
	SQLITE3_OGEN O2 = O1;
	cout << "O1= " << O1 << endl;
	cout << "O1 + 1= " << int(O1 + 1) << endl;
	++O1;
	O1++;
	cout << "O2= " << (SQLITE3_OINT)O2 << endl;
	O1 += 10;
	cout << "O2= " << SQLITE3_OINT(O2) << endl;
	O1 -= 20;
	cout << "O2= " << SQLITE3_OINT{O2} << endl;
	exit(0);
	*/

	char* messageError; 
	std::string sql;
    sqlite::return_code_t rc;
	sqlite::ErrorCode ec;
	sqlite::db db("example.db");
	sqlite::connection conn = db.getConnection();

	#if 0
    // Run a query
	sql = "CREATE TABLE IF NOT EXISTS PERSON("
					"id       INTEGER PRIMARY KEY NOT NULL,"
					"name     TEXT,"
					"salary   REAL NOT NULL);"; 
	db.prepare(sql.c_str());
	rc = db.step();
	if (rc != SQLITE_DONE) { 
		std::cerr << "Error on CREATE TABLE: " << sqlite3_errmsg(conn) << "\n"; //messageError << "'\n"; 
	} 
	else
		std::cout << "Table created Successfully" << std::endl; 

	db.reset();
	db.finalize();
	return 0;
	#endif

	db.execute("CREATE TABLE IF NOT EXISTS PERSON("
				"id       INTEGER PRIMARY KEY NOT NULL,"
				"name     TEXT,"
				"salary   REAL NOT NULL);"
			   );

	std::cout << "STATE OF TABLE BEFORE INSERT" << std::endl; 
	sql = "SELECT * FROM PERSON;"; 
	//rc = sqlite3_exec(conn, sql.c_str(), callback, (void *)"", NULL); 
	ec = db.command2(sql);
	cout << "command2 returned: " << rc << endl;

	/*db.prompt(
		"-header -csv",
		"'SELECT * FROM PERSON'"
		" > data.txt");
	return 0;*/

	/*****************************************************
	 *
	NOTE:  All this boilerplate code:
	
	```
	rc = sqlite3_prepare_v2(
		conn,
		"INSERT INTO PERSON VALUES(:id,:name,:salary);",
		-1,
		&stmt,
		NULL
	);
	if (rc != sqlite::return_code::ok) {
		std::cerr << "sqlite3_prepare_v2: " << sqlite3_errmsg(conn) << std::endl; 
		throw sqlite::exception_prepare(sqlite3_errmsg(conn));
	}
	```

	is elegantly replaced by a couple of lines:
	
	```
	sqlite::statement stmt(conn);
	rc = stmt2.prepare("INSERT INTO PERSON VALUES(:id,:name,:salary);");
	```

	or even more concise:

	```
	rc = sqlite::statement(conn).prepare(
		"INSERT INTO PERSON VALUES(:id,:name,:salary);"
	);
	```

	Indeed, if the prepare fails, an exception will be thrown (which you can
	capture for right treatment), otherwise the usual C-like return code will be
	retrieved.
	*/

	/** insert record, and flush statement */
	int iCol;
	int val = 1;
	SQLITE3_OINT VAL_INT = val;
	double salary = 300.22;
	
	#if 1
	db.prepare("INSERT INTO PERSON(`id`,`name`,`salary`) VALUES(:id,:name,:salary);")
	  .bind({":id", ":name", ":salary"}, 
	        VAL_INT(), SQLITE3_OSTRING{"John"}, salary);

	rc = 0;
	try {
		//rc = db.step();
		db.step(ec);
		if (ec != error::done)  {
			cout << ec
				<< ": " << db.errmsg()
				<< endl;
			exit(1);
		}
		cout << "last step returned: " << ec.value() 
			<< " [ROW: " << sqlite::return_code::row
			<< ", DONE: " << sqlite::return_code::done
			<< "]"
			<< endl;
		db.reset();
	} catch (sqlite::exception &e) {
		cout << "exception detected: " << e.what() << endl;
		exit(1);
	}
	db.finalize();
	#endif
	return 0;
	int N_times = 5;
	int id = 0;
	db.execute("BEGIN TRANSACTION");
	assert(!db.autocommit());
	while (--N_times >= 0) {
		++id;
		db.execute(
			"INSERT INTO PERSON VALUES(:id,:name,:salary)"
			" ON CONFLICT(`id`) DO UPDATE SET `name`=:name, `salary`=:salary;"
			//"INSERT OR REPLACE INTO PERSON VALUES(:id,:name,:salary)"
			,{":id", ":name", ":salary"}
			//,VAL_INT(), SQLITE3_OSTRING{"John"}, salary
			,id, "Larry Salinas", salary
		);
	}
	db.execute("COMMIT TRANSACTION");
	
	std::cout << "STATE OF TABLE AFTER INSERT" << std::endl; 
	sql = "SELECT * FROM PERSON;"; 
	//rc = sqlite3_exec(conn, sql.c_str(), callback, (void *)"", NULL); 
	db.command2(sql);

	return 0; 
} 

