#include <iostream> 
#include <cstdio>
#include <tuple>
#include <exception>
#include <error.h>
#include <sqlite3.h> 

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

/** Custom sqlite exception */
class sqlite3_exception: public std::runtime_error
{
public:
	sqlite3_exception(const char *msg) : runtime_error(msg) {}
};

/** insert record into table, via prepared statement */
typedef tuple<int, const char*, const char *, int , const char *, double> row_t;
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
	if ( (rc = sqlite3_bind_int(stmt, 1, std::get<0>(row))) != SQLITE_OK ) {
		throw sqlite3_exception(sqlite3_errstr(rc));
	} else if ( (rc = sqlite3_bind_text(stmt, 2, std::get<1>(row), -1, SQLITE_TRANSIENT)) != SQLITE_OK ) {
		throw sqlite3_exception(sqlite3_errstr(rc));
	} else if ( (rc = sqlite3_bind_text(stmt, 3, std::get<2>(row), -1, SQLITE_TRANSIENT)) != SQLITE_OK ) {
		throw sqlite3_exception(sqlite3_errstr(rc));
	} else if ( (rc = sqlite3_bind_int(stmt, 4, std::get<3>(row))) != SQLITE_OK ) {
		throw sqlite3_exception(sqlite3_errstr(rc));
	} else if ( (rc = sqlite3_bind_text(stmt, 5, std::get<4>(row), -1, SQLITE_TRANSIENT)) != SQLITE_OK ) {
		throw sqlite3_exception(sqlite3_errstr(rc));
	} else if ( (rc = sqlite3_bind_double(stmt, 6, std::get<5>(row))) != SQLITE_OK ) {
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

int main(int argc, char** argv) 
{ 
	/**
    MyClass::INT a = 1;
    MyClass::CHAR c = 'a';
    printf("a = %d, c = %c\n", a, c);
    printf("FLAGS::one is: %d\n", FLAGS::one);
    printf("FLAGS::two is: %d\n", FLAGS::two);
    printf("MyClass::FLAGS::one is: %d\n", MyClass::FLAGS::one);
    printf("MyClass::FLAGS::two is: %d\n", MyClass::FLAGS::two);
    exit(0);
    */

   	/*throw MyException("This is a custom exception");
	//throw std::runtime_error("this is an exception");
	exit(0);
	*/

    sqlite3* conn; 
	int rcode = 0; 
	rcode = sqlite3_open_v2(
        "example.db",         // datafile name
        &conn,                  // sqlite3 object 
        SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE         // open flags
            | SQLITE_OPEN_NOMUTEX | SQLITE_OPEN_NOFOLLOW,
        NULL                  // zVfs
    );
	char* messageError; 

	if (rcode) { 
		std::cerr << "Error open DB " << sqlite3_errmsg(conn) << std::endl; 
		return (-1); 
	} 
	else
		std::cout << "Open Database Successfully!" << std::endl; 
    
    // Run a query
	std::string sql = "CREATE TABLE PERSON("
					"ID       INT  PRIMARY KEY NOT NULL, "
					"NAME     TEXT NOT NULL, "
					"SURNAME  TEXT NOT NULL, "
					"AGE	  INT  NOT NULL, "
					"ADDRESS  CHAR(50), "
					"SALARY	  REAL);"; 
	
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

	/*
	sql = "INSERT INTO PERSON VALUES(1, 'STEVE', 'GATES', 30, 'PALO ALTO', 1000.0);"
			"INSERT INTO PERSON VALUES(2, 'BILL', 'ALLEN', 20, 'SEATTLE', 300.22);"
			"INSERT INTO PERSON VALUES(3, 'PAUL', 'JOBS', 24, 'SEATTLE', 9900.0);"; 

	rcode = sqlite3_exec(conn, sql.c_str(), NULL, 0, &messageError); 
	if (rcode != SQLITE_OK) { 
		std::cerr << "Error Insert: '" << messageError << "'" << std::endl; 
		sqlite3_free(messageError); 
	} 
	else
		std::cout << "Records created Successfully!" << std::endl; 
	*/
	sqlite3_stmt *stmt;
	int rc;
	rc = sqlite3_prepare_v2(
		conn,
		"INSERT INTO PERSON VALUES(?,?,?,?,?,?);",
		-1,
		&stmt,
		NULL
	);
	if (rc != SQLITE_OK) {
		std::cerr << "sqlite3_prepare_v2: " << sqlite3_errmsg(conn) << std::endl; 
		stmt = nullptr;
	}

	/** insert record, and flush statement */
   	try {
		insert_row(stmt, row_t{1, "STEVE", "GATES", 30, "PALO ALTO", 1000.0});
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

	/** insert record, and flush statement */
	try {
		insert_row(stmt, row_t{1, "STEVE", "GATES", 30, "PALO ALTO", 1000.0});
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

	/** insert record, and flush statement */
	try {
		insert_row(stmt, row_t{2, "BILL", "ALLEN", 20, "SEATTLE", 300.22});
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

	/** insert record, and flush statement */
	try {
		insert_row(stmt, row_t{3, "PAUL", "JOBS", 24, "SEATTLE", 9900.0});
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

