#include "netcdf.h"

#include <stdlib.h>
#include <stdio.h>

#include <iostream>

#define TEST_FILE_NAME "../test/run03v6_12hrloop.nc"

#define ERR_CODE 2
#define ERR(e) { if (e != NC_NOERR) { printf("Error: %s\n", nc_strerror(e)); exit(ERR_CODE); } }

void printSummary(int ncid);
void printVarList(int ncid);
void printDims(int ncid, int varID);
void printAttribs(int ncid, int varID);
void getNCTypeName(nc_type type, char* buffer);
void printAttribValue(int ncid, int varID, char* attribName, nc_type type, size_t len);
void printVarData(int ncid, int varID);

int main(int argc, char *argv[])
{
	int status = NC_NOERR;
	int ncid;

	char* fName = argc > 1 ? argv[1] : TEST_FILE_NAME;

	status = nc_open(fName, NC_NOWRITE, &ncid);
	ERR(status);

	printf("Opened netCDF file %s\n", fName);

	printSummary(ncid);

	bool running = true;
	while (running)
	{
		printf("\nMain Options:\n");
		printf("\t0: Exit\n");
		printf("\t1: Dimensions\n");
		printf("\t2: Variables\n");
		printf("\t3: Global Attributes\n");

		printf("\nEnter choice: ");

		int choice = NC_MIN_INT;
		scanf_s("%d", &choice);
		while (getchar() != '\n');

		switch (choice)
		{
		case 0:
			running = false;
			break;
		case 1:
			printDims(ncid, NC_GLOBAL);
			break;
		case 2:
			printVarList(ncid);
			break;
		case 3:
			printAttribs(ncid, NC_GLOBAL);
			break;
		default:
			printf("ERROR: Invalid choice\n");
			break;
		}
	}

	return EXIT_SUCCESS;
}

void printSummary(int ncid)
{
	int nDims, nVars, nAttribs, unlimDim;
	int status = nc_inq(ncid, &nDims, &nVars, &nAttribs, &unlimDim);

	if(unlimDim == -1)
		printf("There are %d dimensions (no unlimited dimensions), %d variables, and %d global attributes.\n", nDims, nVars, nAttribs);
	else
		printf("There are %d dimensions (1 unlimited dimension = ID %d), %d variables, and %d global attributes.\n", nDims, unlimDim, nVars, nAttribs);
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

	int unlimDimID;
	status = nc_inq_unlimdim(ncid, &unlimDimID);
	ERR(status);

	printf("%5s%20s%6s\n", "DimID", "Name", "Size");

	for (int i = 0; i < nDims; ++i)
	{
		char dimName[NC_MAX_NAME + 1];
		size_t dimLen;

		int status = nc_inq_dim(ncid, dimIDs[i], dimName, &dimLen);
		ERR(status);

		printf("%5d%20s%6zd", dimIDs[i], dimName, dimLen);

		if (i == unlimDimID)
			printf(" (unlimited dimension)");

		printf("\n");
	}
}

void printVarList(int ncid)
{
	int nVars;
	int status = nc_inq_nvars(ncid, &nVars);
	ERR(status);

	printf("\nnetCDF file contains %d variables:\n", nVars);

	printf("%5s%20s%8s%12s%12s\n", "VarID", "Name", "Type", "Dimensions", "Attributes");

	for (int i = 0; i < nVars; ++i)
	{
		char varName[NC_MAX_NAME + 1];
		nc_type varType;
		int nDims, nAttribs;
		int dims[NC_MAX_VAR_DIMS];

		nc_inq_var(ncid, i, varName, &varType, &nDims, dims, &nAttribs);
		ERR(status);

		char typeName[NC_MAX_NAME + 1];
		getNCTypeName(varType, typeName);

		printf("%5d%20s%8s%12d%12d\n", i, varName, typeName, nDims, nAttribs);
	}

	while (true)
	{
		printf("\nEnter a variable ID for detailed information or -1 to go back: ");
		
		int choice = NC_MIN_INT;
		scanf_s("%d", &choice);
		while (getchar() != '\n');

		if (choice == -1)
		{
			break;
		}
		else if (choice < -1 || choice > nVars - 1)
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
			scanf_s("%d", &dumpData);
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
		strcpy_s(buffer, NC_MAX_NAME + 1, "BYTE");
		break;
	case NC_UBYTE:
		strcpy_s(buffer, NC_MAX_NAME + 1, "UBYTE");
		break;
	case NC_CHAR:
		strcpy_s(buffer, NC_MAX_NAME + 1, "CHAR");
		break;
	case NC_SHORT:
		strcpy_s(buffer, NC_MAX_NAME + 1, "SHORT");
		break;
	case NC_USHORT:
		strcpy_s(buffer, NC_MAX_NAME + 1, "USHORT");
		break;
	case NC_INT:
		strcpy_s(buffer, NC_MAX_NAME + 1, "INT");
		break;
	case NC_UINT:
		strcpy_s(buffer, NC_MAX_NAME + 1, "UINT");
		break;
	case NC_INT64:
		strcpy_s(buffer, NC_MAX_NAME + 1, "INT64");
		break;
	case NC_UINT64:
		strcpy_s(buffer, NC_MAX_NAME + 1, "UINT64");
		break;
	case NC_FLOAT:
		strcpy_s(buffer, NC_MAX_NAME + 1, "FLOAT");
		break;
	case NC_DOUBLE:
		strcpy_s(buffer, NC_MAX_NAME + 1, "DOUBLE");
		break;
	case NC_STRING:
		strcpy_s(buffer, NC_MAX_NAME + 1, "STRING");
		break;
	default:
		strcpy_s(buffer, NC_MAX_NAME + 1, "NONE");
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
		break;
	}
	case NC_DOUBLE:
	{
		double* val = (double*)malloc(len * sizeof(double));
		status = nc_get_att(ncid, varID, attribName, val);
		ERR(status);
		for (int i = 0; i < len; ++i)
			printf("%f ", val[i]);
		break;
	}
	case NC_INT:
	{
		int* val = (int*)malloc(len * sizeof(int));
		status = nc_get_att(ncid, varID, attribName, val);
		ERR(status);
		for (int i = 0; i < len; ++i)
			printf("%d ", val[i]);
		break;
	}
	default:
		break;
	}
}

void printVarData(int ncid, int varID)
{
	nc_type varType;
	int status = nc_inq_vartype(ncid, varID, &varType);
	ERR(status);
	
	printf("Not yet implemented...\n");
}