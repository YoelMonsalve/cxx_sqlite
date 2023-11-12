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

int main(int argc, char** argv) 
{ 
   	/*throw MyException("This is a custom exception");
	//throw std::runtime_error("this is an exception");
	exit(0);*/

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
	
    sqlite3* conn; 
	int rcode = 0; 
	rcode = sqlite3_open_v2(
        "example.db",         // datafile name
        &conn,                // sqlite3 object 
        SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE         // open flags
            | SQLITE_OPEN_NOMUTEX | SQLITE_OPEN_NOFOLLOW,
        NULL                  // zVfs
    );
	char* messageError; 

	if (rcode != SQLITE_OK) { 
		std::cerr << "Error open DB " << sqlite3_errmsg(conn) << std::endl; 
		return -1; 
	} 
	else
		std::cout << "Open Database Successfully!" << std::endl; 
    
    // Run a query
	std::string sql = "CREATE TABLE PERSON("
					"id       INTEGER PRIMARY KEY NOT NULL,"
					"name     TEXT,"
					"salary   REAL NOT NULL);"; 
	
	rcode = sqlite3_exec(conn, sql.c_str(), NULL, 0, &messageError); 

	if (rcode != SQLITE_OK) { 
		std::cerr << "Error on CREATE TABLE: " << sqlite3_errmsg(conn) << "\n"; //messageError << "'\n"; 
		sqlite3_free(messageError); 
	} 
	else
		std::cout << "Table created Successfully" << std::endl; 
	

	sql = "SELECT * FROM PERSON;"; 

	std::cout << "STATE OF TABLE BEFORE INSERT" << std::endl; 

	rcode = sqlite3_exec(conn, sql.c_str(), callback, (void *)"", NULL); 

	sqlite3_stmt *stmt;
	int rc;
	rc = sqlite3_prepare_v2(
		conn,
		"INSERT INTO PERSON VALUES(:id,:name,:salary);",
		-1,
		&stmt,
		NULL
	);
	if (rc != SQLITE_OK) {
		std::cerr << "sqlite3_prepare_v2: " << sqlite3_errmsg(conn) << std::endl; 
		stmt = nullptr;
	}

    cxx_sqlite3::db db{};

	/** insert record, and flush statement */
   	try {
		int rc;
        if (!stmt) throw sqlite3_exception("Error in preparing stmt");
        
        int iCol;
        int val = 1;
        SQLITE3_OINT VAL_INT = val;
        
		/*if ( (rc = db.bind_one(stmt, pair<string, cxx_sqlite3::basic_object<int>>{":id", VAL_INT()})) != SQLITE_OK) {
			throw sqlite3_exception(sqlite3_errstr(rc));
		}

		if ( (rc = db.bind_one(stmt, ":name", "John")) != SQLITE_OK) {
			throw sqlite3_exception(sqlite3_errstr(rc));
		}
        
        if ( (rc = db.bind_one(stmt, ":salary", VAL_DBL())) != SQLITE_OK) {
			throw sqlite3_exception(sqlite3_errstr(rc));
		}
        */

		double salary = 300.22;
		db.bind(stmt, {":id", ":name", ":salary"}, VAL_INT(), SQLITE3_OSTRING{"John"}, salary);

        if ( (rc = sqlite3_step(stmt)) != SQLITE_DONE ) {
            throw sqlite3_exception(sqlite3_errstr(rc));
        }
	}
	catch (sqlite3_exception &e) {
		std::cerr << "sqlite3: " << e.what() << ": " << sqlite3_errmsg(conn) << std::endl;
	}
	catch (std::exception &e) {
		std::cerr << "exception: " << e.what() << ": " << sqlite3_errmsg(conn) << std::endl;
	}
	if ((rc = sqlite3_reset(stmt)) != SQLITE_OK) {
		//std::cerr << "sqlite3_reset: " << sqlite3_errstr(rc) << std::endl;
	}

	/** clear statement */
	if ((rc = sqlite3_finalize(stmt)) != SQLITE_OK) {
		std::cerr << "sqlite3_finalize: " << sqlite3_errmsg(conn) << std::endl; 
	}

    // Close DB connection, and exit
    if (sqlite3_close(conn) != SQLITE_OK) 
        std::cerr << "Error closing DB connection.\n";
    else 
        std::cout << "DB connection closed successfully.\n"; 
	return (0); 
} 

