# TODO

- [X] Pass everything in `statement`, and `db`` classes that uses return code to `sqlite::ErrorCode` class. 
- [X] _Consider_: change the class name `db` to `database`.
- [ ] Finish up the signature `statement::bind(vector<string> col_ids, generic_col_vals vals)`.
- [ ] Implement the signature `statement::bind(col_id_1, col_val_1, col_id_2, col_val_2, ...)`.
- [ ] Add `const` qualifier where possible/needed on `sqlite.hpp` (consider defining const versions).
- [ ] `bind` with `generic_col_vals`.
- [ ] Define the special object `SQLITE3_ONULL``.
- [X] Don't REPEAT yourself. _Decide_ between using internal class-level variable `rc_`, and using a local function-scope variable `rc`.
- [ ] Add support to read `blob` data from row (link)[https://www.sqlite.org/c3ref/blob_open.html]).