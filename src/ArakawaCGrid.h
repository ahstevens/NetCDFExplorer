#pragma once

#include <vector>

class ArakawaCGrid
{
public:

	class Cell
	{
	public:
		Cell();
		~Cell();

	private:

	};

	ArakawaCGrid();
	~ArakawaCGrid();

private:
	std::vector<float> m_S_w; // S-coordinate relative vertical w locations (at cell vertical boundaries)
	std::vector<float> m_S_rho; // S-coordinate relative vertical rho locations (in middle of cells)
};