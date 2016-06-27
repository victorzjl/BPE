#include <boost/test/unit_test.hpp>
#include "SmallObject.h"
using namespace sdo::common;

typedef struct student_st:public CSmallObject
{
    int age;
    char name[16];
}SStudent;
BOOST_AUTO_TEST_SUITE(test_smallobject)

BOOST_AUTO_TEST_CASE(test_new_delete)
{
    SStudent *s1=new SStudent;
    SStudent *s2=new SStudent;
    BOOST_CHECK_EQUAL((char *)s2-(char *)s1,32);
    delete s1;
    
    SStudent *s3=new SStudent;
    BOOST_CHECK_EQUAL((char *)s2-(char *)s3,32);

    delete s2;
    delete s3;
}
BOOST_AUTO_TEST_SUITE_END()


