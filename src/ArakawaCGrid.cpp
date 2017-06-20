#include "ArakawaCGrid.h"

#include <assert.h>

ArakawaCGrid::ArakawaCGrid(int ncid)
	: m_iNCID(ncid)
	, m_u(NULL)
	, m_v(NULL)
	, m_w(NULL)
{
	build();
}

ArakawaCGrid::~ArakawaCGrid()
{
	if (m_u)
		delete m_u;

	if (m_v)
		delete m_v;

	if (m_w)
		delete m_w;
}

void ArakawaCGrid::build()
{
	buildHorizontalGrid();
	buildVerticalGrid();

	m_u = new NCVar(m_iNCID, "u", false);
	m_v = new NCVar(m_iNCID, "v", false);
	m_w = new NCVar(m_iNCID, "w", false);
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
	NCVar lat_psi(m_iNCID, "lat_psi", false);
	NCVar lon_psi(m_iNCID, "lon_psi", false);
	NCVar mask_psi(m_iNCID, "mask_psi", false);

	assert(lat_psi.dimlens[0] == lon_psi.dimlens[0]);
	assert(lat_psi.dimlens[1] == lon_psi.dimlens[1]);

	for (size_t i = 0u; i < lat_psi.dimlens[0]; ++i)
	{
		std::vector<GridPoint> row;
		for (size_t j = 0u; j < lat_psi.dimlens[1]; ++j)
		{
			row.push_back(GridPoint{ *lat_psi.get(i, j), *lon_psi.get(i, j), *mask_psi.get(i, j) == 0.f });
		}
		m_vvPsiGrid.push_back(row);
	}


	// RHO POINTS
	NCVar lat_rho(m_iNCID, "lat_rho", true);
	NCVar lon_rho(m_iNCID, "lon_rho", true);
	NCVar mask_rho(m_iNCID, "mask_rho", false);

	assert(lat_rho.dimlens[0] == lon_rho.dimlens[0]);
	assert(lat_rho.dimlens[1] == lon_rho.dimlens[1]);

	// rho, u, and v points have a border of exterior computational points around psi domain boundaries
	assert(lat_rho.dimlens[0] == lat_psi.dimlens[0] + 1u);
	assert(lat_rho.dimlens[1] == lat_psi.dimlens[1] + 1u);
	assert(lon_rho.dimlens[0] == lon_psi.dimlens[0] + 1u);
	assert(lon_rho.dimlens[1] == lon_psi.dimlens[1] + 1u);

	// populate cell heights at rho points too while we're at it
	NCVar h_rho(m_iNCID, "h", false);
	assert(h_rho.dimlens[0] == lat_rho.dimlens[0]);
	assert(h_rho.dimlens[1] == lat_rho.dimlens[1]);

	for (size_t i = 0u; i < lat_rho.dimlens[0]; ++i)
	{
		std::vector<RhoPoint> row;
		for (size_t j = 0u; j < lat_rho.dimlens[1]; ++j)
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

	m_ffMinCoordinate = std::pair<float, float>(lat_rho.min, lon_rho.min);
	m_ffMaxCoordinate = std::pair<float, float>(lat_rho.max, lon_rho.max);


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

	for (size_t i = 0u; i < lat_u.dimlens[0]; ++i)
	{
		std::vector<AdvectionPoint> row;
		for (size_t j = 0u; j < lat_u.dimlens[1]; ++j)
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

	for (size_t i = 0u; i < lat_v.dimlens[0]; ++i)
	{
		std::vector<AdvectionPoint> row;
		for (size_t j = 0u; j < lat_v.dimlens[1]; ++j)
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

	m_unSigmaLayers = s_rho.dimlens[0];

	for (size_t i = 0; i < m_unSigmaLayers; ++i)
		m_vfSRhoRatios.push_back(*s_rho.get(i));
	
	for (size_t i = 0; i < m_unSigmaLayers + 1; ++i)
		m_vfSWRatios.push_back(*s_w.get(i));
}
