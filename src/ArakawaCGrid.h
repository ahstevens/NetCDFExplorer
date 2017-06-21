#pragma once

#include <vector>

#include "NetCDFHelper.h"

class ArakawaCGrid
{
public:
	struct GridPoint {
		float lat;
		float lon;
		bool masked;
	};

	struct RhoPoint : public GridPoint {
		float h;
	};

	struct AdvectionPoint : public GridPoint {
		float vel;
	};

	ArakawaCGrid(int ncid);
	~ArakawaCGrid();

	void build();

private:
	int m_iNCID;

	// HORIZONTAL GRID STRUCTURE COMPONENTS
	std::vector< std::vector<GridPoint> > m_vvPsiGrid;
	std::vector< std::vector<RhoPoint> > m_vvRhoGrid;
	std::vector< std::vector<AdvectionPoint> > m_vvUGrid;
	std::vector< std::vector<AdvectionPoint> > m_vvVGrid;
	std::vector< std::vector<AdvectionPoint> > m_vvWGrid;

	std::pair<float, float> m_ffMinCoordinate, m_ffMaxCoordinate;

	// VERTICAL GRID STRUCTURE COMPONENTS
	unsigned int m_unSigmaLayers;
	std::vector<float> m_vfSWRatios, m_vfSRhoRatios;

	// DATA VARIABLES
	NCVar *m_u;
	NCVar *m_v;
	NCVar *m_w;

	void buildHorizontalGrid();
	void buildVerticalGrid();
};