#ifndef NIMBLE_RMLOGRE_NODECONNECTIONMAP_HPP
#define NIMBLE_RMLOGRE_NODECONNECTIONMAP_HPP

#include <unordered_map>
#include <vector>


namespace Ogre {

class CompositorNode;
class CompositorWorkspace;

}

namespace nimble::RmlOgre {

class NodeConnectionMap
{
	Ogre::CompositorWorkspace* workspace;
	Ogre::CompositorNode* currentNode;

	std::unordered_map<int, std::pair<Ogre::CompositorNode*, int>> outMap;

public:
	NodeConnectionMap(Ogre::CompositorWorkspace* workspace) :
		workspace{workspace}
	{}

	void setCurrentNode(Ogre::CompositorNode* node)
	{
		this->currentNode = node;
	}
	void setIn(int id, int channel);
	void setOut(int id, int channel);

	void setExternal(int externalChannel, int channel);
};

}

#endif // NIMBLE_RMLOGRE_NODECONNECTIONMAP_HPP
