# TODO

- [ ] Pass everything in `statement`, and `db`` classes that uses return code to `sqlite::ErrorCode` class. 
- [ ] _Consider_: change the class name `db` to `database`.
- [ ] Implement the signature `db::command(std::string sql, arg1, val1, arg2, val2, ...)`.
- [ ] Add `const` qualifier where possible/needed on `sqlite.hpp` (consider defining const versions).
- [ ] `bind` with `generic_col_vals`.
- [ ] Define the special object `SQLITE3_ONULL``.
- [ ] Don't REPEAT yourself. _Decide_ between using internal class-level variable `rc_`, and using a local function-scope variable `rc`.
- [ ] 