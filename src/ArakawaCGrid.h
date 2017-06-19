#pragma once

#include <vector>

extern struct NCVar;

class ArakawaCGrid
{
public:
	ArakawaCGrid(unsigned int width, unsigned int height, unsigned int vertical_levels);
	~ArakawaCGrid();

	void buildHorizontalGrid(NCVar *psi_lat, NCVar *psi_lon);
	void buildRhoPoints(NCVar *rho_lat, NCVar *rho_lon);
	void buildUVPoints(NCVar *u_lat, NCVar *u_lon, NCVar *v_lat, NCVar *v_lon);

private:
	unsigned int m_unWidth, m_unHeight, m_unLevels;
	std::vector<float> m_S_w; // S-coordinate relative vertical w locations (at cell vertical boundaries)
	std::vector<float> m_S_rho; // S-coordinate relative vertical rho locations (in middle of cells)
};