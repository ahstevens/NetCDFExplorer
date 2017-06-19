#include "netcdf.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

#define ERR_CODE 2
#define ERR(e) { if (e != NC_NOERR) { printf("Error: %s\n", nc_strerror(e)); exit(ERR_CODE); } }

void printUsage(char* argv[]);
void printSummary(int ncid);
void printVarList(int ncid, int dimFilter);
void printDims(int ncid, int varID);
void printAttribs(int ncid, int varID);
void getNCTypeName(nc_type type, char* buffer);
void printAttribValue(int ncid, int varID, char* attribName, nc_type type, size_t len);
void printVarData(int ncid, int varID);

struct NCVar {
	int id;
	char name[NC_MAX_NAME];
	int ndims;
	int* dimids;
	size_t* dimlens;
	size_t len;
	float* data;
	float min;
	float max;
	bool uses_fill;
	float fill; // fill value, i.e., N/A or NULL

	float* get(int dim1index = -1, int dim2index = -1, int dim3index = -1, int dim4index = -1)
	{
		//if (dim1index > -1 && ndims < 1 ||
		//	dim2index > -1 && ndims < 2 ||
		//	dim3index > -1 && ndims < 3 ||
		//	dim4index > -1 && ndims < 4)
		//{
		//	printf("Bad index in %s.get()", name);
		//	return NULL;
		//}
		
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
				printf("Bad index in dimension 1 (length = %d) of %s.get(): %d\n", dimlens[0], name, dim1index);
				return NULL;
			}
			if (dim2index < 0 || dim2index >= (int)dimlens[1])
			{
				printf("Bad index in dimension 2 (length = %d) of %s.get(): %d\n", dimlens[1], name, dim2index);
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
				printf("Bad index in dimension 1 (length = %d) of %s.get(): %d\n", dimlens[0], name, dim1index);
				return NULL;
			}
			if (dim2index < 0 || dim2index >= (int)dimlens[1])
			{
				printf("Bad index in dimension 2 (length = %d) of %s.get(): %d\n", dimlens[1], name, dim2index);
				return NULL;
			}
			if (dim3index < 0 || dim3index >= (int)dimlens[2])
			{
				printf("Bad index in dimension 3 (length = %d) of %s.get(): %d\n", dimlens[2], name, dim3index);
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
				printf("Bad index in dimension 1 (length = %d) of %s.get(): %d\n", dimlens[0], name, dim1index);
				return NULL;
			}
			if (dim2index < 0 || dim2index >= (int)dimlens[1])
			{
				printf("Bad index in dimension 2 (length = %d) of %s.get(): %d\n", dimlens[1], name, dim2index);
				return NULL;
			}
			if (dim3index < 0 || dim3index >= (int)dimlens[2])
			{
				printf("Bad index in dimension 3 (length = %d) of %s.get(): %d\n", dimlens[2], name, dim3index);
				return NULL;
			}
			if (dim4index < 0 || dim4index >= (int)dimlens[3])
			{
				printf("Bad index in dimension 4 (length = %d) of %s.get(): %d\n", dimlens[3], name, dim4index);
				return NULL;
			}
			return &data[dim4index + dim3index * dimlens[3] + dim2index * dimlens[3] * dimlens[2] + dim1index * dimlens[3] * dimlens[2] * dimlens[1]];
			break;
		}

	}
};

NCVar getVar(int ncid, const char* var_name);
void freeVar(NCVar* var);
void buildArakawaCGrid(int ncid);

int main(int argc, char* argv[])
{
	int status = NC_NOERR;
	int ncid;

	if (argc < 2)
	{
		printUsage(argv);
		exit(EXIT_FAILURE);
	}

	const char* fName = argv[1];

	status = nc_open(fName, NC_NOWRITE, &ncid);
	ERR(status);

	printf("Opened netCDF file %s\n", fName);

	bool running = true;
	while (running)
	{
		printf("\nMain Options:\n");
		printf("\t0: Exit\n");
		printf("\t1: File Summary\n");
		printf("\t2: Global Attributes\n");
		printf("\t3: Dimensions\n");
		printf("\t4: All Variables\n");
		printf("\t5: Single-value Variables\n");
		printf("\t6: 1D Variables\n");
		printf("\t7: 2D Variables\n");
		printf("\t8: 3D Variables\n");
		printf("\t9: 4D Variables\n");

		printf("\nEnter choice: ");

		int choice = NC_MIN_INT;
		scanf("%d", &choice);
		while (getchar() != '\n');

		switch (choice)
		{
		case 0:
			running = false;
			break;
		case 1:
			printSummary(ncid);
			break;
		case 2:
			printAttribs(ncid, NC_GLOBAL);
			break;
		case 3:
			printDims(ncid, NC_GLOBAL);
			break;
		case 4:
			printVarList(ncid, -1);
			break;
		case 5:
			printVarList(ncid, 0);
			break;
		case 6:
			printVarList(ncid, 1);
			break;
		case 7:
			printVarList(ncid, 2);
			break;
		case 8:
			printVarList(ncid, 3);
			break;
		case 9:
			printVarList(ncid, 4);
			break;
		case 10:
			buildArakawaCGrid(ncid);
			break;
		default:
			printf("ERROR: Invalid choice\n");
			break;
		}
	}

	status = nc_close(ncid);
	ERR(status);

	return EXIT_SUCCESS;
}

void printUsage(char* argv[])
{
	printf("\nUsage:\n\t%s <NetCDF File>\n", argv[0]);
}

void printSummary(int ncid)
{
	int nDims, nVars, nAttribs;
	int status = nc_inq(ncid, &nDims, &nVars, &nAttribs, NULL);
	ERR(status);

	size_t titleLen;

	status = nc_inq_attlen(ncid, NC_GLOBAL, "title", &titleLen);

	char* title = (char*) malloc(sizeof(char) * (titleLen + 1));
	
	status = nc_get_att_text(ncid, NC_GLOBAL, "title", title);

	title[titleLen] = '\0'; // must manually null-terminate the string

	printf("\nModel Title: \"%s\"\n", title);

	free(title);

	int nUnlimDims, unlimDimIDs[NC_MAX_DIMS];
	status = nc_inq_unlimdims(ncid, &nUnlimDims, unlimDimIDs);

	printf("\t%d dimensions (%d unlimited)\n", nDims, nUnlimDims);
	printf("\t%d variables\n", nVars);
	printf("\t%d global attributes\n", nAttribs);
}

void printDims(int ncid, int varID)
{
	int status = NC_NOERR;
	int nDims;
	int dimIDs[NC_MAX_DIMS];

	if (varID == NC_GLOBAL)
	{
		status = nc_inq_ndims(ncid, &nDims);
		ERR(status);
		printf("\nThe NetCDF file contains %d dimensions:\n", nDims);
		for (int i = 0; i < nDims; ++i)
			dimIDs[i] = i;
	}
	else
	{
		char varName[NC_MAX_NAME + 1];
		status = nc_inq_var(ncid, varID, varName, NULL, &nDims, dimIDs, NULL);
		ERR(status);
		printf("\nVariable \"%s\" (ID %d) contains %d dimensions:\n", varName, varID, nDims);
	}

	if (nDims == 0) return;

	int nUnlimDims, unlimDimIDs[NC_MAX_DIMS];
	status = nc_inq_unlimdims(ncid, &nUnlimDims, unlimDimIDs);
	ERR(status);

	printf("%5s%20s%6s\n", "DimID", "Name", "Size");

	for (int i = 0; i < nDims; ++i)
	{
		char dimName[NC_MAX_NAME + 1];
		size_t dimLen;

		int status = nc_inq_dim(ncid, dimIDs[i], dimName, &dimLen);
		ERR(status);

		printf("%5d%20s%6zd", dimIDs[i], dimName, dimLen);

		for (int j = 0; j < nUnlimDims; ++j)
		{
			if (dimIDs[i] == unlimDimIDs[j])
				printf(" (unlimited dimension)");
		}

		printf("\n");
	}
}

void printVarList(int ncid, int dimFilter)
{
	int nVars;
	int status = nc_inq_nvars(ncid, &nVars);
	ERR(status);

	printf("\nnetCDF file contains %d variables:\n", nVars);

	if (nVars == 0) return;

	printf("%5s%20s%8s%12s%12s%13s\n", "VarID", "Name", "Type", "Dimensions", "Attributes", "Description");

	bool* indexFilter = (bool*)malloc(sizeof(bool) * nVars);

	for (int i = 0; i < nVars; ++i)
	{
		char varName[NC_MAX_NAME + 1];
		nc_type varType;
		int nDims, nAttribs;
		int dims[NC_MAX_VAR_DIMS];

		nc_inq_var(ncid, i, varName, &varType, &nDims, dims, &nAttribs);
		ERR(status);

		if (dimFilter != -1 && nDims != dimFilter)
		{
			indexFilter[i] = false;
			continue;
		}
		else
		{
			indexFilter[i] = true;
		}

		char typeName[NC_MAX_NAME + 1];
		getNCTypeName(varType, typeName);

		size_t descLen;
		char* desc;
		if(nc_inq_attlen(ncid, i, "long_name", &descLen) != NC_ENOTATT)
		{
			desc = (char*)malloc(sizeof(char) * (descLen + 1));
			status = nc_get_att_text(ncid, i, "long_name", desc);
			ERR(status);
			desc[descLen] = '\0';
		}
		else
		{
			desc = (char*)malloc(sizeof(char) * 5);
			strcpy(desc, "none");
		}

		printf("%5d%20s%8s%12d%12d  %s\n", i, varName, typeName, nDims, nAttribs, desc);

		free(desc);
	}

	while (true)
	{
		printf("\nEnter a variable ID for detailed information or -1 to go back: ");
		
		int choice = NC_MIN_INT;
		scanf("%d", &choice);
		while (getchar() != '\n');

		if (choice == -1)
		{
			break;
		}
		else if (choice < -1 || choice > nVars - 1 || !indexFilter[choice])
		{
			printf("ERROR: Invalid selection\n");
			continue;
		}

		printDims(ncid, choice);

		printAttribs(ncid, choice);

		while (true)
		{
			printf("\nEnter 0 to dump variable data or -1 to go back: ");

			int dumpData = NC_MIN_INT;
			scanf("%d", &dumpData);
			while (getchar() != '\n');

			if (dumpData == 0)
			{
				printVarData(ncid, choice);
				break;
			}
			else if (dumpData == -1)
			{
				break;
			}

			printf("ERROR: Invalid selection\n");
		}
	}

	free(indexFilter);
}

void printAttribs(int ncid, int varID)
{
	int nAttribs;

	int status = nc_inq_varnatts(ncid, varID, &nAttribs);
	ERR(status);

	if (varID == NC_GLOBAL)
	{
		printf("\nThe NetCDF file contains %d global attributes:\n", nAttribs);
	}
	else
	{
		char varName[NC_MAX_NAME + 1];
		status = nc_inq_varname(ncid, varID, varName);
		printf("\nVariable \"%s\" (ID %d) contains %d attributes:\n", varName, varID, nAttribs);
	}

	if (nAttribs == 0) return;

	printf("%8s%20s%8s%5s  %-8s\n", "AttribID", "Name", "Type", "Size", "Value");

	for (int i = 0; i < nAttribs; ++i)
	{
		char attrName[NC_MAX_NAME + 1];
		nc_type attrType;
		size_t attrLen;

		nc_inq_attname(ncid, varID, i, attrName);
		ERR(status);

		nc_inq_att(ncid, varID, attrName, &attrType, &attrLen);
		ERR(status);

		char typeName[NC_MAX_NAME + 1];
		getNCTypeName(attrType, typeName);

		printf("%8d%20s%8s%5zd  ", i, attrName, typeName, attrLen);

		printAttribValue(ncid, varID, attrName, attrType, attrLen);

		printf("\n");
	}
}

void getNCTypeName(nc_type type, char* buffer)
{
	switch (type)
	{
	case NC_BYTE:
		strcpy(buffer, "BYTE");
		break;
	case NC_UBYTE:
		strcpy(buffer, "UBYTE");
		break;
	case NC_CHAR:
		strcpy(buffer, "CHAR");
		break;
	case NC_SHORT:
		strcpy(buffer, "SHORT");
		break;
	case NC_USHORT:
		strcpy(buffer, "USHORT");
		break;
	case NC_INT:
		strcpy(buffer, "INT");
		break;
	case NC_UINT:
		strcpy(buffer, "UINT");
		break;
	case NC_INT64:
		strcpy(buffer, "INT64");
		break;
	case NC_UINT64:
		strcpy(buffer, "UINT64");
		break;
	case NC_FLOAT:
		strcpy(buffer, "FLOAT");
		break;
	case NC_DOUBLE:
		strcpy(buffer, "DOUBLE");
		break;
	case NC_STRING:
		strcpy(buffer, "STRING");
		break;
	default:
		strcpy(buffer, "NONE");
		break;
	}
}

void printAttribValue(int ncid, int varID, char* attribName, nc_type type, size_t len)
{
	int status = NC_NOERR;

	switch (type)
	{
	case NC_CHAR:
	{
		char* val = (char*)malloc(len + 1);
		status = nc_get_att(ncid, varID, attribName, val);
		ERR(status);
		val[len] = '\0';
		printf("%s", val);
		free(val);
		break;
	}
	case NC_FLOAT:
	{
		float* val = (float*)malloc(len * sizeof(float));
		status = nc_get_att(ncid, varID, attribName, val);
		ERR(status);
		for (int i = 0; i < len; ++i)
			printf("%f ", val[i]);
		free(val);
		break;
	}
	case NC_DOUBLE:
	{
		double* val = (double*)malloc(len * sizeof(double));
		status = nc_get_att(ncid, varID, attribName, val);
		ERR(status);
		for (int i = 0; i < len; ++i)
			printf("%f ", val[i]);
		free(val);
		break;
	}
	case NC_INT:
	{
		int* val = (int*)malloc(len * sizeof(int));
		status = nc_get_att(ncid, varID, attribName, val);
		ERR(status);
		for (int i = 0; i < len; ++i)
			printf("%d ", val[i]);
		free(val);
		break;
	}
	default:
		break;
	}
}

void printVarData(int ncid, int varID)
{
	char varName[NC_MAX_NAME];
	nc_type varType;
	int nDims, dimIDs[NC_MAX_VAR_DIMS], nAtts;
	int status = nc_inq_var(ncid, varID, varName, &varType, &nDims, dimIDs, &nAtts);
	ERR(status);

	int flattenedDimSize = 1;

	for (int i = 0; i < nDims; ++i)
	{
		size_t dimLen;
		nc_inq_dimlen(ncid, dimIDs[i], &dimLen);
		flattenedDimSize *= (int)dimLen;
	}

	printf("Dumping %d values from %d dimensions:\n\n", flattenedDimSize, nDims);

	switch (varType)
	{
	case NC_FLOAT:
	{
		float* val = (float*)malloc(flattenedDimSize * sizeof(float));
		status = nc_get_var_float(ncid, varID, val);
		ERR(status);
		for (int i = 0; i < flattenedDimSize; ++i)
			printf("%f ", val[i]);
		break;
	}
	case NC_DOUBLE:
	{
		double* val = (double*)malloc(flattenedDimSize * sizeof(double));
		status = nc_get_var_double(ncid, varID, val);
		ERR(status);
		for (int i = 0; i < flattenedDimSize; ++i)
			printf("%f ", val[i]);
		break;
	}
	case NC_INT:
	{
		int* val = (int*)malloc(flattenedDimSize * sizeof(int));
		status = nc_get_var_int(ncid, varID, val);
		ERR(status);
		for (int i = 0; i < flattenedDimSize; ++i)
			printf("%d ", val[i]);
		break;
	}
	default:
		break;
	}
	
	printf("\n\nDone!\n");
}

NCVar getVar(int ncid, const char* var_name)
{
	NCVar var;
	strcpy(var.name, var_name);
	nc_inq_varid(ncid, var.name, &var.id);
	nc_inq_varndims(ncid, var.id, &var.ndims);
	var.dimids = (int*)malloc(var.ndims * sizeof(int));
	var.dimlens = (size_t*)malloc(var.ndims * sizeof(size_t));
	nc_inq_vardimid(ncid, var.id, var.dimids);
	var.len = 1;
	for (int i = 0; i < var.ndims; ++i)
	{
		nc_inq_dimlen(ncid, var.dimids[i], &var.dimlens[i]);
		var.len *= var.dimlens[i];
	}

	// allocate and get the data
	var.data = (float*)malloc(var.len * sizeof(float));
	nc_get_var_float(ncid, var.id, (float*)var.data);
	
	// check if data uses a fill value to designate NULL or N/A values
	int usesFillVal = nc_get_att(ncid, var.id, "_FillValue", &(var.fill));
	if (usesFillVal == NC_NOERR)
	{
		var.uses_fill = true;
		var.min = var.max = var.fill;
	}
	else
	{
		var.uses_fill = false;
		var.min = var.max = var.data[0];
	}

	for (size_t i = 0u; i < var.len; ++i)
	{
		if (var.data[i] < var.min && (!var.uses_fill || (var.uses_fill && var.fill != var.data[i])))
			var.min = var.data[i];

		if ((var.data[i] > var.max && !var.uses_fill) || 
			(var.uses_fill && ((var.data[i] > var.max && var.max != var.fill && var.data[i] != var.fill) || (var.max == var.fill && var.data[i] != var.fill))))
			var.max = var.data[i];
	}

	printf("\n%s\n\tMin: %f\n\tMax: %f\n", var.name, var.min, var.max);

	return var;
}

void freeVar(NCVar* var)
{
	free(var->dimids);
	free(var->dimlens);
	free(var->data);
}

void buildArakawaCGrid(int ncid)
{
	//	  psi(i,j+1)----v(i,j+1)----psi(i+1,j+1)
	//		   |						 |
	//		   |						 |
	//		   |						 |
	//		u(i,j)      rho(i,j)	  u(i+1,j)
	//		   |						 |
	//         |						 |
	//		   |						 |
	//	   psi(i,j)------v(i,j)------psi(i+1,j)
	
	NCVar lon_u = getVar(ncid, "lon_u");
	NCVar lat_u = getVar(ncid, "lat_u");
	NCVar mask_u = getVar(ncid, "mask_u");
	NCVar lat_v = getVar(ncid, "lat_v");
	NCVar lon_v = getVar(ncid, "lon_v");
	NCVar mask_v = getVar(ncid, "mask_v");
	NCVar lat_psi = getVar(ncid, "lat_psi");
	NCVar lon_psi = getVar(ncid, "lon_psi");
	NCVar mask_psi = getVar(ncid, "mask_psi");
	NCVar lat_rho = getVar(ncid, "lat_rho");
	NCVar lon_rho = getVar(ncid, "lon_rho");
	NCVar mask_rho = getVar(ncid, "mask_rho");

	NCVar h_rho = getVar(ncid, "h");
	NCVar s_rho = getVar(ncid, "s_rho");
	NCVar s_w = getVar(ncid, "s_w");
	
	NCVar u = getVar(ncid, "u");
	NCVar v = getVar(ncid, "v");
	NCVar w = getVar(ncid, "w");
}