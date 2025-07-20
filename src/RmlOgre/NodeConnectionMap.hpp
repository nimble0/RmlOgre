#ifndef NIMBLE_RMLOGRE_NODECONNECTIONMAP_HPP
#define NIMBLE_RMLOGRE_NODECONNECTIONMAP_HPP

#include <OgreIdString.h>

#include <vector>


namespace nimble::RmlOgre {

struct NodeConnection
{
	Ogre::IdString outNode, inNode;
	int outChannel = -1, inChannel = -1;
};

class NodeConnectionMap
{
	Ogre::IdString currentNode;
	std::vector<NodeConnection> map;

public:
	using Iterator = decltype(map)::iterator;
	using ConstIterator = decltype(map)::const_iterator;

	Iterator begin() { return this->map.begin(); }
	Iterator end() { return this->map.end(); }
	ConstIterator begin() const { return this->map.begin(); }
	ConstIterator end() const { return this->map.end(); }

	void setCurrentNode(Ogre::IdString node)
	{
		this->currentNode = node;
	}
	void setIn(int id, int channel)
	{
		if(id < 0)
			return;

		if(static_cast<std::size_t>(id) >= this->map.size())
			this->map.resize(id + 1);
		this->map[id].inNode = this->currentNode;
		this->map[id].inChannel = channel;
	}
	void setOut(int id, int channel)
	{
		if(id < 0)
			return;

		if(static_cast<std::size_t>(id) >= this->map.size())
			this->map.resize(id + 1);
		this->map[id].outNode = this->currentNode;
		this->map[id].outChannel = channel;
	}
};

}

#endif // NIMBLE_RMLOGRE_NODECONNECTIONMAP_HPP
