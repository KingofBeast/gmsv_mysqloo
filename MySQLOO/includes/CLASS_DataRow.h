#ifndef _CLASS_DATAROW_H_
#define _CLASS_DATAROW_H_

#include <string>
#include <vector>

class DataRow : public std::vector<std::string*>
{
public:
	DataRow(void);
	virtual ~DataRow(void);
};

#endif //_CLASS_DATAROW_H_