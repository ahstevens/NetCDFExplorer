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

		printVarData(ncid, choice);
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
	case NC_SHORT:
	{
		short* val = (short*)malloc(len * sizeof(short));
		status = nc_get_att(ncid, varID, attribName, val);
		ERR(status);
		for (int i = 0; i < len; ++i)
			printf("%hi ", val[i]);
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
	int nDims, dimIDs[NC_MAX_VAR_DIMS], *dimLens, nAtts;
	int status = nc_inq_var(ncid, varID, varName, &varType, &nDims, dimIDs, &nAtts);
	ERR(status);

	dimLens = (int*)malloc(nDims * sizeof(int));

	unsigned valuecount = 1;

	for (int i = 0; i < nDims; ++i)
	{
		nc_inq_dimlen(ncid, dimIDs[i], &dimLens[i]);
		valuecount *= (unsigned)dimLens[i];
	}

	float val_offset = 0.f;
	float val_scale = 1.f;

	for (int i = 0; i < nAtts; ++i)
	{
		char attrName[NC_MAX_NAME + 1];
		nc_type attrType;
		size_t attrLen;

		nc_inq_attname(ncid, varID, i, attrName);
		ERR(status);

		if (strcmp(attrName, "add_offset") == 0)
		{
			status = nc_get_att(ncid, varID, attrName, &val_offset);
			ERR(status);
		}

		if (strcmp(attrName, "scale_factor") == 0)
		{
			status = nc_get_att(ncid, varID, attrName, &val_scale);
			ERR(status);
		}
	}

	printf("\nSUMMARY:\n\n   Size: %d (%d", nDims, dimLens[0]);
	for (int i = 1; i < nDims; ++i)
		printf("x%d", dimLens[i]);
	printf(") dimensions\n  Count: %d values\n\n", valuecount);

	switch (varType)
	{
	case NC_FLOAT:
	{
		float* vals = (float*)malloc(valuecount * sizeof(float));

		float fillval;
		bool isfillval = nc_get_att(ncid, varID, "_FillValue", &fillval) == NC_NOERR;

		status = nc_get_var_float(ncid, varID, vals);
		ERR(status);

		float minVal = NC_MAX_FLOAT;
		float maxVal = NC_MIN_FLOAT;

		long double avgVal = 0; // this could overflow

		for (int i = 0; i < valuecount; ++i)
		{
			if (isfillval && vals[i] == fillval) continue;
			if (vals[i] < minVal) minVal = vals[i];
			if (vals[i] > maxVal) maxVal = vals[i];
			avgVal += (long double)vals[i];
			//printf("%lf ", val[i]);
		}

		printf("Raw Min: %f\nRaw Max: %f\n  Range: %f\nAverage: %f\n", minVal, maxVal, maxVal - minVal, avgVal / (float)valuecount);

		break;
	}
	case NC_DOUBLE:
	{
		double* vals = (double*)malloc(valuecount * sizeof(double));
		
		double fillval;
		bool isfillval = nc_get_att(ncid, varID, "_FillValue", &fillval) == NC_NOERR;

		status = nc_get_var_double(ncid, varID, vals);
		ERR(status);

		double minVal = NC_MAX_DOUBLE;
		double maxVal = NC_MIN_DOUBLE;

		long double avgVal = 0; // this could overflow

		for (int i = 0; i < valuecount; ++i)
		{
			if (isfillval && vals[i] == fillval) continue;
			if (vals[i] < minVal) minVal = vals[i];
			if (vals[i] > maxVal) maxVal = vals[i];
			avgVal += (long double)vals[i];
			//printf("%lf ", val[i]);
		}

		printf("Raw Min: %f\nRaw Max: %f\n  Range: %f\nAverage: %f\n", minVal, maxVal, maxVal - minVal, avgVal / (double)valuecount);

		break;
	}
	case NC_INT:
	{
		int* vals = (int*)malloc(valuecount * sizeof(int));

		int fillval;
		bool isfillval = nc_get_att(ncid, varID, "_FillValue", &fillval) == NC_NOERR;

		status = nc_get_var_int(ncid, varID, vals);
		ERR(status);

		int minVal = NC_MAX_INT;
		int maxVal = NC_MIN_INT;

		long long avgVal = 0;

		for (int i = 0; i < valuecount; ++i)
		{
			if (isfillval && vals[i] == fillval) continue;
			if (vals[i] < minVal) minVal = vals[i];
			if (vals[i] > maxVal) maxVal = vals[i];
			avgVal += (long long)vals[i];
			//printf("%d ", vals[i]);
		}

		printf("Raw Min: %d\nRaw Max: %d\n  Range: %d\nAverage: %f\n", minVal, maxVal, maxVal - minVal, (float)avgVal / (float)valuecount);

		break;
	}
	case NC_SHORT:
	{
		short* vals = (short*)malloc(valuecount * sizeof(short));

		short fillval;
		bool isfillval = nc_get_att(ncid, varID, "_FillValue", &fillval) == NC_NOERR;

		status = nc_get_var_short(ncid, varID, vals);
		ERR(status);

		short minVal = NC_MAX_SHORT;
		short maxVal = NC_MIN_SHORT;

		long long avgVal = 0;

		for (int i = 0; i < valuecount; ++i)
		{
			if (isfillval && vals[i] == fillval) continue;
			if (vals[i] < minVal) minVal = vals[i];
			if (vals[i] > maxVal) maxVal = vals[i];
			avgVal += (long long)vals[i];
			//printf("%hi ", val[i]);
		}

		printf("Raw Min: %hi\nRaw Max: %hi\n  Range: %hi\nAverage: %f\n", minVal, maxVal, maxVal - minVal, (float)avgVal / (float)valuecount);

		break;
	}
	default:
		break;
	}
	
	printf("\n");
}
