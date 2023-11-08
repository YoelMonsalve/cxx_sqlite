#include <iostream> 
#include <cstdio>
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
		std::cerr << "Error on CREATE TABLE:\n '" << sqlite3_errmsg(conn) << "'\n"; //messageError << "'\n"; 
		sqlite3_free(messageError); 
	} 
	else
		std::cout << "Table created Successfully" << std::endl; 
	

	sql = "SELECT * FROM PERSON;"; 

	std::cout << "STATE OF TABLE BEFORE INSERT" << std::endl; 

	rcode = sqlite3_exec(conn, sql.c_str(), callback, (void *)"foo", NULL); 

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

	/*
	std::cout << "STATE OF TABLE AFTER INSERT" << std::endl; 

	sqlite3_exec(conn, sql.c_str(), callback, NULL, NULL); 

	sql = "DELETE FROM PERSON WHERE ID = 2;"; 
	rcode = sqlite3_exec(conn, sql.c_str(), NULL, 0, &messageError); 
	if (rcode != SQLITE_OK) { 
		std::cerr << "Error DELETE" << std::endl; 
		sqlite3_free(messageError); 
	} 
	else
		std::cout << "Record deleted Successfully!" << std::endl; 

	std::cout << "STATE OF TABLE AFTER DELETING ELEMENT" << std::endl; 
	rcode = sqlite3_exec(conn, sql.c_str(), callback, NULL, NULL); 
	*/
    // Close DB connection, and exit
    if (sqlite3_close(conn) != SQLITE_OK) 
        std::cerr << "Error closing DB connection.\n";
    else 
        std::cout << "DB connection closed successfully.\n"; 
	return (0); 
} 

