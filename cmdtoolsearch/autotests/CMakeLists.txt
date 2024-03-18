find_package(Qt6Test ${QT_MIN_VERSION} CONFIG REQUIRED)

include(ECMAddTests)

ecm_add_tests(
    cmdtooltest.cpp
    LINK_LIBRARIES
        Qt::Test
        cmdtool_static
)
