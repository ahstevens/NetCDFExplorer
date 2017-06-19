#include "ArakawaCGrid.h"

ArakawaCGrid::ArakawaCGrid(unsigned int width, unsigned int height, unsigned int vertical_levels)
	: m_unWidth(width)
	, m_unHeight(height)
	, m_unLevels(vertical_levels)
{
}

ArakawaCGrid::~ArakawaCGrid()
{
}

void ArakawaCGrid::buildHorizontalGrid(NCVar * psi_lat, NCVar * psi_lon)
{
}

void ArakawaCGrid::buildRhoPoints(NCVar * rho_lat, NCVar * rho_lon)
{
}

void ArakawaCGrid::buildUVPoints(NCVar * u_lat, NCVar * u_lon, NCVar * v_lat, NCVar * v_lon)
{
}
