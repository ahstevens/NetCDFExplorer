#pragma once

#include <vector>

class NCVar {
	friend class ArakawaCGrid;
public:
	NCVar(int ncid, const char* var_name, bool calcMinMax);
	~NCVar();

	float* get(int dim1index = -1, int dim2index = -1, int dim3index = -1, int dim4index = -1);	

private:
	int id;
	char* name;
	int ndims;
	int* dimids;
	size_t* dimlens;
	size_t len;
	float* data;
	float min;
	float max;
	bool uses_fill;
	float fill; // fill value, i.e., N/A or NULL
};