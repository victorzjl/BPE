#include "CvsArchiveAdapter.h"

#include <boost/archive/archive_exception.hpp>
#include <boost/archive/binary_iarchive.hpp>
#include <boost/archive/binary_oarchive.hpp>
#include <boost/serialization/vector.hpp>
#include <sstream>
#include <algorithm>
#include "AsyncVirtualServiceLog.h"

using std::stringstream;
using namespace boost::serialization;
using namespace boost::archive;

template<class Archive>
void serialize(Archive & ar, CVSKeyValue & objKeyValue, const unsigned int version)
{
	ar & objKeyValue.dwKey;
	ar & objKeyValue.strValue;
}


string CCvsArchiveAdapter::Oarchive(const vector<CVSKeyValue> &vecKeyValue, bool isClear)
{
	CVS_XLOG(XLOG_DEBUG, "CCvsArchiveAdapter::%s\n", __FUNCTION__);
	if (isClear)
	{
		if (vecKeyValue.size()>0)
		{
			return vecKeyValue[0].strValue;
		}
		return "";
	}
	stringstream sso;
	
	try
	{
		boost::archive::binary_oarchive pboa(sso);
		pboa & vecKeyValue;
	}
	catch (boost::archive::archive_exception & e)
	{
		CVS_XLOG(XLOG_ERROR, "CCvsArchiveAdapter::%s, error[%s]\n", __FUNCTION__, e.what());
	}

	return sso.str();
}

int CCvsArchiveAdapter::Iarchive(const string &ar, vector<CVSKeyValue>& vecKeyValue, bool isClear)
{
	CVS_XLOG(XLOG_DEBUG, "CCvsArchiveAdapter::%s\n", __FUNCTION__);
	if (isClear)
	{
		CVSKeyValue vs;
		vs.dwKey = 10001;
		vs.strValue = ar;
		vecKeyValue.push_back(vs);
		return 0;
	}
	if (ar.size()==0)
		return 0;
	stringstream ssi(ar);
	try
	{
		boost::archive::binary_iarchive pbia(ssi);
		pbia & vecKeyValue;
	}
	catch (boost::archive::archive_exception & e)
	{
		CVS_XLOG(XLOG_ERROR, "CCvsArchiveAdapter::%s, error[%s]\n", __FUNCTION__, e.what());
		return -1;
	}

	return 0;
}

