#include "NetCDFHelper.h"
#include "netcdf.h"

#include <cstdlib>

NCVar::NCVar(int ncid, const char* var_name, bool calcMinMax)
{
	name = (char*)malloc((strlen(var_name) + 1) * sizeof(char));
	strcpy(this->name, var_name);
	nc_inq_varid(ncid, name, &id);
	nc_inq_varndims(ncid, id, &ndims);
	dimids = (int*)malloc(ndims * sizeof(int));
	dimlens = (size_t*)malloc(ndims * sizeof(size_t));
	nc_inq_vardimid(ncid, id, dimids);
	len = 1;
	for (int i = 0; i < ndims; ++i)
	{
		nc_inq_dimlen(ncid, dimids[i], &dimlens[i]);
		len *= dimlens[i];
	}

	// allocate and get the data
	data = (float*)malloc(len * sizeof(float));
	nc_get_var_float(ncid, id, (float*)data);

	// check if data uses a fill value to designate NULL or N/A values
	int usesFillVal = nc_get_att(ncid, id, "_FillValue", &(fill));
	if (usesFillVal == NC_NOERR)
	{
		uses_fill = true;
		min = max = fill;
	}
	else
	{
		uses_fill = false;
		min = max = data[0];
	}

	if (!calcMinMax)
		return;

	bool fillValueReplaced = !uses_fill || data[0] != fill;
	for (size_t i = 0u; i < len; ++i)
	{
		if (data[i] == fill)
			continue;

		if (!fillValueReplaced)
		{
			min = max = data[i];
			fillValueReplaced = true;
			continue;
		}

		if (data[i] < min)
			min = data[i];

		if (data[i] > max)
			max = data[i];
	}

	printf("\n%s\n\tMin: %f\n\tMax: %f\n", name, min, max);
}

NCVar::~NCVar()
{
	free(name);
	free(dimids);
	free(dimlens);
	free(data);
}

// Last dimension varies fastest, e.g., time, depth, lat, lon
float* NCVar::get(int dim1index, int dim2index, int dim3index, int dim4index)
{
	switch (ndims)
	{
	case 0:
		return data;
		break;
	case 1:
		if (dim1index < 0 || dim1index >= (int)dimlens[0])
		{
			printf("Bad index in dimension 1 of %s.get(): %d\n", name, dim1index);
			return NULL;
		}
		if (dim2index != -1 || dim3index != -1 || dim4index != -1)
		{
			printf("There are only %d dimensions in %s!\n", ndims, name);
			return NULL;
		}
		return &data[dim1index];
		break;
	case 2:
		if (dim1index < 0 || dim1index >= (int)dimlens[0])
		{
			printf("Bad index in dimension 1 (length = %zu) of %s.get(): %d\n", dimlens[0], name, dim1index);
			return NULL;
		}
		if (dim2index < 0 || dim2index >= (int)dimlens[1])
		{
			printf("Bad index in dimension 2 (length = %zu) of %s.get(): %d\n", dimlens[1], name, dim2index);
			return NULL;
		}
		if (dim3index != -1 || dim4index != -1)
		{
			printf("There are only %d dimensions in %s!\n", ndims, name);
			return NULL;
		}
		return &data[dim2index + dim1index * dimlens[1]];
		break;
	case 3:
		if (dim1index < 0 || dim1index >= (int)dimlens[0])
		{
			printf("Bad index in dimension 1 (length = %zu) of %s.get(): %d\n", dimlens[0], name, dim1index);
			return NULL;
		}
		if (dim2index < 0 || dim2index >= (int)dimlens[1])
		{
			printf("Bad index in dimension 2 (length = %zu) of %s.get(): %d\n", dimlens[1], name, dim2index);
			return NULL;
		}
		if (dim3index < 0 || dim3index >= (int)dimlens[2])
		{
			printf("Bad index in dimension 3 (length = %zu) of %s.get(): %d\n", dimlens[2], name, dim3index);
			return NULL;
		}
		if (dim4index != -1)
		{
			printf("There are only %d dimensions in %s!\n", ndims, name);
			return NULL;
		}
		return &data[dim3index + dim2index * dimlens[2] + dim1index * dimlens[2] * dimlens[1]];
		break;
	case 4:
		if (dim1index < 0 || dim1index >= (int)dimlens[0])
		{
			printf("Bad index in dimension 1 (length = %zu) of %s.get(): %d\n", dimlens[0], name, dim1index);
			return NULL;
		}
		if (dim2index < 0 || dim2index >= (int)dimlens[1])
		{
			printf("Bad index in dimension 2 (length = %zu) of %s.get(): %d\n", dimlens[1], name, dim2index);
			return NULL;
		}
		if (dim3index < 0 || dim3index >= (int)dimlens[2])
		{
			printf("Bad index in dimension 3 (length = %zu) of %s.get(): %d\n", dimlens[2], name, dim3index);
			return NULL;
		}
		if (dim4index < 0 || dim4index >= (int)dimlens[3])
		{
			printf("Bad index in dimension 4 (length = %zu) of %s.get(): %d\n", dimlens[3], name, dim4index);
			return NULL;
		}
		return &data[dim4index + dim3index * dimlens[3] + dim2index * dimlens[3] * dimlens[2] + dim1index * dimlens[3] * dimlens[2] * dimlens[1]];
		break;
	default:
		return NULL;
		break;
	}

}