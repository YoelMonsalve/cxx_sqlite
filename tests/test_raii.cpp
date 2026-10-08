#include <iostream>
#include <cassert>
#include <string>
#include "sqlite.hpp"

using namespace cxx_sqlite3;

void test_statement_move() {
    std::cout << "Testing statement move semantics..." << std::endl;
    database db(":memory:");
    db.execute("CREATE TABLE users (id INTEGER PRIMARY KEY, name TEXT);");

    // Prepare statement and move it
    statement stmt1 = db.prepare_statement("INSERT INTO users (name) VALUES (:name);");
    assert(stmt1.is_valid());

    statement stmt2 = std::move(stmt1);
    assert(!stmt1.is_valid());
    assert(stmt2.is_valid());

    stmt2.bind(":name", "Alice");
    stmt2.step();
    stmt2.reset();

    stmt2.bind(":name", "Bob");
    stmt2.step();

    // Verify row count
    statement query = db.prepare_statement("SELECT COUNT(*) FROM users;");
    ErrorCode ec;
    row r = query.fetch_row(ec);
    assert(ec == error::row);
    std::cout << "Statement move semantics PASSED." << std::endl;
}

void test_transaction_commit() {
    std::cout << "Testing transaction commit..." << std::endl;
    database db(":memory:");
    db.execute("CREATE TABLE items (id INTEGER PRIMARY KEY, name TEXT);");

    {
        transaction tx(db);
        db.execute("INSERT INTO items (name) VALUES ('item1');");
        db.execute("INSERT INTO items (name) VALUES ('item2');");
        tx.commit();
    }

    statement query = db.prepare_statement("SELECT COUNT(*) FROM items;");
    ErrorCode ec;
    row r = query.fetch_row(ec);
    assert(ec == error::row);
    std::cout << "Transaction commit PASSED." << std::endl;
}

void test_transaction_rollback() {
    std::cout << "Testing transaction rollback on exception / uncommitted scope..." << std::endl;
    database db(":memory:");
    db.execute("CREATE TABLE items (id INTEGER PRIMARY KEY, name TEXT);");
    db.execute("INSERT INTO items (name) VALUES ('existing');");

    // Scope 1: Exit without commit -> should auto rollback
    {
        transaction tx = db.begin_transaction();
        db.execute("INSERT INTO items (name) VALUES ('rollback_me');");
        // tx goes out of scope without commit()
    }

    // Verify 'rollback_me' was not persisted
    statement q1 = db.prepare_statement("SELECT COUNT(*) FROM items WHERE name = 'rollback_me';");
    ErrorCode ec;
    q1.fetch_row(ec);
    assert(ec == error::row);

    // Scope 2: Exception thrown inside transaction block
    try {
        transaction tx(db);
        db.execute("INSERT INTO items (name) VALUES ('fail_item');");
        throw std::runtime_error("simulated failure");
        tx.commit();
    } catch (const std::exception &e) {
        // Expected
    }

    // Verify database is still in autocommit and item was rolled back
    assert(db.autocommit());
    std::cout << "Transaction rollback PASSED." << std::endl;
}

int main() {
    test_statement_move();
    test_transaction_commit();
    test_transaction_rollback();
    std::cout << "ALL RAII TESTS COMPLETED SUCCESSFULLY!" << std::endl;
    return 0;
}
