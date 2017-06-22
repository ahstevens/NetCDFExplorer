#pragma once

#include <vector>
#include <ANN/ANN.h>

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

	bool contains(float x, float y, float z);
	bool getUVWat(float x, float y, float z, float t, float &u, float &v, float &w);

private:
	int m_iNCID;

	// HORIZONTAL GRID STRUCTURE COMPONENTS
	std::vector< std::vector<GridPoint> > m_vvPsiGrid;
	std::vector< std::vector<RhoPoint> > m_vvRhoGrid;
	std::vector< std::vector<AdvectionPoint> > m_vvUGrid;
	std::vector< std::vector<AdvectionPoint> > m_vvVGrid;
	std::vector< std::vector<AdvectionPoint> > m_vvWGrid;

	std::pair<float, float> m_ffMinCoordinate, m_ffMaxCoordinate;
	float m_fMinHeight, m_fMaxHeight;

	// VERTICAL GRID STRUCTURE COMPONENTS
	int m_nSigmaLayers;
	std::vector<float> m_vfSWRatios, m_vfSRhoRatios;

	// DATA VARIABLES
	NCVar *m_u;
	NCVar *m_v;
	NCVar *m_w;

	void build();
	void buildHorizontalGrid();
	void buildVerticalGrid();

	// KD-TREE FOR SPATIAL SEARCHING
	ANNkd_tree *m_pKDTree;
};