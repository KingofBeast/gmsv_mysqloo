#include "CLASS_DataRow.h"

DataRow::DataRow(void)
{
}

DataRow::~DataRow(void)
{
	for( iterator columnIterator = begin();
		columnIterator != end();
		++columnIterator)
	{
		delete *columnIterator;
	}
}
