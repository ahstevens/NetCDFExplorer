#include "ArakawaCGrid.h"

#include <LatLong-UTM.h>

#include <assert.h>

ArakawaCGrid::ArakawaCGrid(int ncid)
	: m_iNCID(ncid)
	, m_u(NULL)
	, m_v(NULL)
	, m_w(NULL)
	, m_pKDTree(NULL)
{
	build();
	
	float x, y, z;
	x = m_ffMinCoordinate.first + (m_ffMaxCoordinate.first - m_ffMinCoordinate.first) / 2.0;
	y = m_ffMinCoordinate.second + (m_ffMaxCoordinate.second - m_ffMinCoordinate.second) / 2.0;
	z = m_fMinHeight + (m_fMaxHeight - m_fMinHeight) / 2.f;

	float u, v, w;
	getUVWat(x, y, z, 0, u, v, w);
	
}

ArakawaCGrid::~ArakawaCGrid()
{
	if (m_u)
		delete m_u;

	if (m_v)
		delete m_v;

	if (m_w)
		delete m_w;

	if (m_pKDTree)
	{
		ANNpointArray pts = m_pKDTree->thePoints();
		annDeallocPts(pts);
	}
}

bool ArakawaCGrid::contains(float x, float y, float z)
{
	double x_conv, y_conv;
	int zone;
	LLtoUTM(22, x, y, y_conv, x_conv, zone);

	double nMax, nMin, eMax, eMin;
	LLtoUTM(22, m_ffMinCoordinate.first, m_ffMinCoordinate.second, nMin, eMin, zone);
	LLtoUTM(22, m_ffMaxCoordinate.first, m_ffMaxCoordinate.second, nMax, eMax, zone);

	if (x_conv < eMin)
		return false;
	else if (x_conv > eMax)
		return false;
	else if (y_conv < nMin)
		return false;
	else if (y_conv > nMax)
		return false;
	else if (z < m_fMinHeight)
		return false;
	else if (z > m_fMaxHeight)
		return false;
	else
		return true;

	return false;
}

bool ArakawaCGrid::getUVWat(float x, float y, float z, float t, float & u, float & v, float & w)
{
	if (!contains(x, y, z))
	{
		u = v = w = 0.f;
		return false;
	}
	
	double northing, easting;
	int zone;
	LLtoUTM(22, x, y, northing, easting, zone);

	ANNpoint pt = annAllocPt(3);
	pt[0] = easting;
	pt[1] = northing;
	pt[2] = z;
	ANNidxArray indArr = new ANNidx[1];
	ANNdistArray sqDistArr = new ANNdist[1];

	// search the KD tree to find which Arakwa C-grid cell we're in
	m_pKDTree->annkSearch(pt, 1, indArr, sqDistArr);

	int dimLens[3] = { m_nSigmaLayers, m_vvRhoGrid.size(), m_vvRhoGrid.front().size() };
	int* inds = recoverIndex(indArr[0], 3, dimLens); // 3D index from flat array index

	int sigmaLayer = inds[0]; // 0 is sea surface

	// get flow components in the C-grid cell
	float uWest = *m_u->get(t, sigmaLayer, inds[1], inds[2] - 1);
	if (uWest == m_u->fill) uWest = 0.f;
	float uEast = *m_u->get(t, sigmaLayer, inds[1], inds[2]);
	if (uEast == m_u->fill) uEast = 0.f;
	float vSouth = *m_v->get(t, sigmaLayer, inds[1] - 1, inds[2]);
	if (vSouth == m_v->fill) vSouth = 0.f;
	float vNorth = *m_v->get(t, sigmaLayer, inds[1], inds[2]);
	if (vNorth == m_v->fill) vNorth = 0.f;
	float wBottom = *m_w->get(t, sigmaLayer, inds[1], inds[2]);
	if (wBottom == m_w->fill) wBottom = 0.f;
	float wTop = *m_w->get(t, sigmaLayer + 1, inds[1], inds[2]);
	if (wTop == m_w->fill) wTop = 0.f;

	printf("%f -> %f\n%f\n^\n|\n%f\n%f, %f", uWest, uEast, vSouth, vNorth, wBottom, wTop);

	return true;
}

void ArakawaCGrid::build()
{
	buildHorizontalGrid();
	buildVerticalGrid();
	buildKDTree();
	
	// Load up flow vector components
	printf("Loading u-component flow data... ");
	m_u = new NCVar(m_iNCID, "u", false);
	printf(" done!\n");

	printf("Loading v-component flow data... ");
	m_v = new NCVar(m_iNCID, "v", false);
	printf(" done!\n");

	printf("Loading w-component flow data... ");
	m_w = new NCVar(m_iNCID, "w", false);
	printf(" done!\n");
}

void ArakawaCGrid::buildHorizontalGrid()
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


	// PSI POINTS
	NCVar lat_psi(m_iNCID, "lat_psi", true);
	NCVar lon_psi(m_iNCID, "lon_psi", true);
	NCVar mask_psi(m_iNCID, "mask_psi", false);

	assert(lat_psi.dimlens[0] == lon_psi.dimlens[0]);
	assert(lat_psi.dimlens[1] == lon_psi.dimlens[1]);

	for (int i = 0u; i < (int)lat_psi.dimlens[0]; ++i)
	{
		std::vector<GridPoint> row;
		for (int j = 0u; j < (int)lat_psi.dimlens[1]; ++j)
		{
			row.push_back(GridPoint{ *lat_psi.get(i, j), *lon_psi.get(i, j), *mask_psi.get(i, j) == 0.f });
		}
		m_vvPsiGrid.push_back(row);
	}
	
	m_ffMinCoordinate = std::pair<float, float>(lat_psi.min, lon_psi.min);
	m_ffMaxCoordinate = std::pair<float, float>(lat_psi.max, lon_psi.max);


	// RHO POINTS
	NCVar lat_rho(m_iNCID, "lat_rho", false);
	NCVar lon_rho(m_iNCID, "lon_rho", false);
	NCVar mask_rho(m_iNCID, "mask_rho", false);

	assert(lat_rho.dimlens[0] == lon_rho.dimlens[0]);
	assert(lat_rho.dimlens[1] == lon_rho.dimlens[1]);

	// rho, u, and v points have a border of exterior computational points around psi domain boundaries
	assert(lat_rho.dimlens[0] == lat_psi.dimlens[0] + 1u);
	assert(lat_rho.dimlens[1] == lat_psi.dimlens[1] + 1u);
	assert(lon_rho.dimlens[0] == lon_psi.dimlens[0] + 1u);
	assert(lon_rho.dimlens[1] == lon_psi.dimlens[1] + 1u);

	// populate cell heights at rho points too while we're at it
	NCVar h_rho(m_iNCID, "h", true);
	assert(h_rho.dimlens[0] == lat_rho.dimlens[0]);
	assert(h_rho.dimlens[1] == lat_rho.dimlens[1]);

	for (int i = 0u; i < (int)lat_rho.dimlens[0]; ++i)
	{
		std::vector<RhoPoint> row;
		for (int j = 0u; j < (int)lat_rho.dimlens[1]; ++j)
		{
			RhoPoint temp;
			temp.lat = *lat_rho.get(i, j);
			temp.lon = *lon_rho.get(i, j);
			temp.masked = *mask_rho.get(i, j) == 0.f;
			temp.h = *h_rho.get(i, j);
			row.push_back(temp);
		}
		m_vvRhoGrid.push_back(row);
	}

	// Get min/max heights and depths
	m_fMinDepth = h_rho.min;
	m_fMaxHeight = -m_fMinDepth;

	m_fMaxDepth = h_rho.max;
	m_fMinHeight = -m_fMaxDepth;

	// U POINTS
	NCVar lat_u(m_iNCID, "lat_u", false);
	NCVar lon_u(m_iNCID, "lon_u", false);
	NCVar mask_u(m_iNCID, "mask_u", false);

	assert(lat_u.dimlens[0] == lon_u.dimlens[0]);
	assert(lat_u.dimlens[1] == lon_u.dimlens[1]);

	// rho, u, and v points have a border of exterior computational points around psi domain boundaries
	assert(lat_u.dimlens[0] == lat_psi.dimlens[0] + 1u);
	assert(lat_u.dimlens[1] == lat_psi.dimlens[1]);
	assert(lon_u.dimlens[0] == lon_psi.dimlens[0] + 1u);
	assert(lon_u.dimlens[1] == lon_psi.dimlens[1]);

	for (int i = 0u; i < (int)lat_u.dimlens[0]; ++i)
	{
		std::vector<AdvectionPoint> row;
		for (int j = 0u; j < (int)lat_u.dimlens[1]; ++j)
		{
			AdvectionPoint temp;
			temp.lat = *lat_u.get(i, j);
			temp.lon = *lon_u.get(i, j);
			temp.masked = *mask_u.get(i, j) == 0.f;
			temp.vel = 0.f;
			row.push_back(temp);
		}
		m_vvUGrid.push_back(row);
	}


	// V POINTS
	NCVar lat_v(m_iNCID, "lat_v", false);
	NCVar lon_v(m_iNCID, "lon_v", false);
	NCVar mask_v(m_iNCID, "mask_v", false);

	assert(lat_v.dimlens[0] == lon_v.dimlens[0]);
	assert(lat_v.dimlens[1] == lon_v.dimlens[1]);

	// rho, u, and v points have a border of exterior computational points around psi domain boundaries
	assert(lat_v.dimlens[0] == lat_psi.dimlens[0]);
	assert(lat_v.dimlens[1] == lat_psi.dimlens[1] + 1u);
	assert(lon_v.dimlens[0] == lon_psi.dimlens[0]);
	assert(lon_v.dimlens[1] == lon_psi.dimlens[1] + 1u);

	for (int i = 0u; i < (int)lat_v.dimlens[0]; ++i)
	{
		std::vector<AdvectionPoint> row;
		for (int j = 0u; j < (int)lat_v.dimlens[1]; ++j)
		{
			AdvectionPoint temp;
			temp.lat = *lat_v.get(i, j);
			temp.lon = *lon_v.get(i, j);
			temp.masked = *mask_v.get(i, j) == 0.f;
			temp.vel = 0.f;
			row.push_back(temp);
		}
		m_vvVGrid.push_back(row);
	}
}

void ArakawaCGrid::buildVerticalGrid()
{
	// ----------- w_N       (surface)
	//      *      rho_N
	//   -------   w_(N-1)
	//      *      rho_(N-1)
	//   -------   
	//      *      
	//   -------   
	//      *      
	//   -------   w_2
	//      *      rho_2
	//   -------   w_1
	//      *      rho_1
	// ----------- w_0       (bottom)
	
	NCVar s_rho(m_iNCID, "s_rho", false);
	NCVar s_w(m_iNCID, "s_w", false);

	m_nSigmaLayers = (int)s_rho.dimlens[0];

	for (int i = 0; i < m_nSigmaLayers; ++i)
		m_vfSRhoRatios.push_back(*s_rho.get(i));
	
	for (int i = 0; i < m_nSigmaLayers + 1; ++i)
		m_vfSWRatios.push_back(*s_w.get(i));
}

void ArakawaCGrid::buildKDTree()
{
	int nRhoPts = (int)m_vvRhoGrid.size() * (int)m_vvRhoGrid.front().size() * m_nSigmaLayers;

	ANNpointArray dataPts = annAllocPts(nRhoPts, 3);

	int currPt = 0;
	for (int i = 0; i < (int)m_vvRhoGrid.size(); ++i)
	{
		for (int j = 0; j < (int)m_vvRhoGrid.front().size(); ++j)
		{
			RhoPoint pt = m_vvRhoGrid[i][j];

			for (int k = 0; k < m_nSigmaLayers; ++k)
			{
				float heightHere = m_vfSRhoRatios[k] * pt.h;

				double northing, easting;
				int zone;
				LLtoUTM(22, pt.lat, pt.lon, northing, easting, zone);

				dataPts[currPt][0] = easting;
				dataPts[currPt][1] = northing;
				dataPts[currPt][2] = heightHere;

				currPt++;
			}
		}
	}

	m_pKDTree = new ANNkd_tree(dataPts, nRhoPts, 3);
}
