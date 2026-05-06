# UNIT_TEST_TASKS

Test suite: **KROIA UnitTest** (`https://github.com/KROIA/UnitTest.git`), fetched by `unittests/UnitTest.cmake`. Build with `-DCOMPILE_UNITTESTS=ON`.

Test layout convention (mirrors KROIA template's `unitTests/ExampleTest/`):
```
unittests/
└── <SuiteName>/
    ├── main.cpp
    ├── tests.h
    └── TST_<thing>.h
```

## Pending
_None yet. Run the **unit-test** agent to scan coverage and seed tasks._

## Task entry template
```
### UT-<id> — <title>
- **Target:** <file/class under test, e.g. core/src/Utilities.cpp::readUserSections>
- **Suite:** unittests/<SuiteName>/
- **Cases:** <bullet list of cases / edge conditions>
- **Status:** pending | in-progress | done
```
