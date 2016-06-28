#include "RvsArchiveAdapter.h"
#include "RedisVirtualServiceLog.h"

#include <boost/archive/archive_exception.hpp>
#include <boost/archive/binary_iarchive.hpp>
#include <boost/archive/binary_oarchive.hpp>
#include <boost/serialization/vector.hpp>
#include <sstream>
#include <algorithm>

using std::stringstream;
using namespace boost::serialization;
using namespace boost::archive;

namespace redis{
	
template<class Archive>
void serialize(Archive & ar, RVSKeyValue & objKeyValue, const unsigned int version)
{
	ar & objKeyValue.dwKey;
	ar & objKeyValue.strValue;
}
}

string CRvsArchiveAdapter::Oarchive(const vector<RVSKeyValue> &vecKeyValue)
{
	RVS_XLOG(XLOG_DEBUG, "CRvsArchiveAdapter::%s\n", __FUNCTION__);

	stringstream sso;
	
	try
	{
		boost::archive::binary_oarchive pboa(sso);
		pboa & vecKeyValue;
	}
	catch (boost::archive::archive_exception & e)
	{
		RVS_XLOG(XLOG_ERROR, "CRvsArchiveAdapter::%s, error[%s]\n", __FUNCTION__, e.what());
	}

	return sso.str();
}

int CRvsArchiveAdapter::Iarchive(const string &ar, vector<RVSKeyValue>& vecKeyValue)
{
	RVS_XLOG(XLOG_DEBUG, "CRvsArchiveAdapter::%s\n", __FUNCTION__);
	stringstream ssi(ar);
	try
	{
		boost::archive::binary_iarchive pbia(ssi);
		pbia & vecKeyValue;
	}
	catch (boost::archive::archive_exception & e)
	{
		RVS_XLOG(XLOG_ERROR, "CRvsArchiveAdapter::%s, error[%s]\n", __FUNCTION__, e.what());
		return -1;
	}

	return 0;
}

