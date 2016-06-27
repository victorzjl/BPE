#ifndef _QSTL_H_
#define _QSTL_H_
//#include <boost/pool/pool_alloc.hpp>
#include <map>
#include <vector>
#include <list>
#include <string>
using std::map;

#define Qmap std::map 
/*
template<class _Kty, class _Ty>
class Qmap : public std::map< _Kty, _Ty, std::less<_Kty>, 
boost::fast_pool_allocator< std::pair<_Kty,_Ty> > >
{};

template<class _Ty>
class Qvector : public std::vector< _Ty, boost::pool_allocator<_Ty> >
{};

template<class _Ty>
class Qlist : public std::list< _Ty, boost::pool_allocator<_Ty> >

{};

template<class _Ty>
class Qstringstream : 
public std::basic_stringstream< _Ty,std::char_traits<_Ty>, 
boost::pool_allocator<_Ty> >
{};

template<class _Ty>
class Qstring: public std::basic_string<_Ty,std::char_traits<_Ty>,
boost::pool_allocator<_Ty> >
{
typedef std::basic_string<_Ty,std::char_traits<_Ty>,
boost::pool_allocator<_Ty> >
BaseClass;
public:
Qstring(){}
Qstring(const _Ty* first,const _Ty* last)
: BaseClass(first,last)
{}
}; 
*/
#endif
