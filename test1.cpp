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

/** insert record into table, via prepared statement */
typedef tuple<const char*, const char *, int , const char *, double> row_t;
/**            ^   ^            ^             ^     ^             ^.............. salary  
 *             |   |            |             |      `........................... address
 *             |   |            |             `.................................. age
 *             |   |            `................................................ surname
 *             |   `............................................................. name
 *              `................................................................ id
 */
void insert_row(sqlite3_stmt *stmt, row_t row) 
{
	int rc;
	if (!stmt) throw sqlite3_exception("NULL stmt");
	if ( (rc = sqlite3_bind_text(stmt, 1, std::get<0>(row), -1, SQLITE_TRANSIENT)) != SQLITE_OK ) {
		throw sqlite3_exception(sqlite3_errstr(rc));
	} else if ( (rc = sqlite3_bind_text(stmt, 2, std::get<1>(row), -1, SQLITE_TRANSIENT)) != SQLITE_OK ) {
		throw sqlite3_exception(sqlite3_errstr(rc));
	} else if ( (rc = sqlite3_bind_int(stmt, 3, std::get<2>(row))) != SQLITE_OK ) {
		throw sqlite3_exception(sqlite3_errstr(rc));
	} else if ( (rc = sqlite3_bind_text(stmt, 4, std::get<3>(row), -1, SQLITE_TRANSIENT)) != SQLITE_OK ) {
		throw sqlite3_exception(sqlite3_errstr(rc));
	} else if ( (rc = sqlite3_bind_double(stmt, 5, std::get<4>(row))) != SQLITE_OK ) {
		throw sqlite3_exception(sqlite3_errstr(rc));
	}

	if ( (rc = sqlite3_step(stmt)) != SQLITE_DONE ) {
		throw sqlite3_exception(sqlite3_errstr(rc));
	}
}

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
	db.command2(sql);
	//return 0;

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
	if (rc != sqlite::error_code::ok) {
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
	
	#if 0
	db.prepare("INSERT INTO PERSON VALUES(:id,:name,:salary);")
	  .bind({":id", ":name", ":salary"}, 
	        VAL_INT(), SQLITE3_OSTRING{"John"}, salary);

	rc = 0;
	while (rc != sqlite::error_code::done) {
		try {
			rc = db.step();
			cout << "last step returned: " << rc 
				<< "ROW: " << sqlite::error_code::row
				<< "DONE: " << sqlite::error_code::done
				<< endl;
			db.reset();
		} catch (sqlite::exception &e) {
			cout << "exception detected: " << e.what() << endl;
		}
	}
	db.finalize();
	#endif
	int N_times = 5;
	int id = 0;
	db.execute("BEGIN TRANSACTION");
	assert(!db.autocommit());
	while (--N_times >= 0) {
		++id;
		db.execute(
			//"INSERT INTO PERSON VALUES(:id,:name,:salary)"
			//" ON CONFLICT(`id`) DO UPDATE SET `name`='name', `salary`='salary';"
			"INSERT OR REPLACE INTO PERSON VALUES(:id,:name,:salary)"
			,{":id", ":name", ":salary"}
			//,VAL_INT(), SQLITE3_OSTRING{"John"}, salary
			,id, "Larry Wheels", salary
		);
	}
	db.execute("COMMIT TRANSACTION");
	
	std::cout << "STATE OF TABLE AFTER INSERT" << std::endl; 
	sql = "SELECT * FROM PERSON;"; 
	//rc = sqlite3_exec(conn, sql.c_str(), callback, (void *)"", NULL); 
	db.command(sql);

	return 0; 
} 

