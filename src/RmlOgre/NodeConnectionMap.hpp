#ifndef NIMBLE_RMLOGRE_NODECONNECTIONMAP_HPP
#define NIMBLE_RMLOGRE_NODECONNECTIONMAP_HPP

#include <OgreIdString.h>

#include <vector>


namespace Ogre {

class CompositorWorkspaceDef;

}

namespace nimble::RmlOgre {

struct NodeConnection
{
	Ogre::IdString outNode, inNode;
	int outChannel = -1, inChannel = -1;
};

class NodeConnectionMap
{
	Ogre::CompositorWorkspaceDef* workspace;
	Ogre::IdString currentNode;
	std::vector<NodeConnection> map;

public:
	using Iterator = decltype(map)::iterator;
	using ConstIterator = decltype(map)::const_iterator;

	NodeConnectionMap(Ogre::CompositorWorkspaceDef* workspace) :
		workspace{workspace}
	{}

	Iterator begin() { return this->map.begin(); }
	Iterator end() { return this->map.end(); }
	ConstIterator begin() const { return this->map.begin(); }
	ConstIterator end() const { return this->map.end(); }

	void setCurrentNode(Ogre::IdString node)
	{
		this->currentNode = node;
	}
	void setIn(int id, int channel);
	void setOut(int id, int channel);

	void setExternal(int externalChannel, int channel);
};

}

#endif // NIMBLE_RMLOGRE_NODECONNECTIONMAP_HPP
