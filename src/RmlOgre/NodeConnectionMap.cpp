#include "NodeConnectionMap.hpp"

#include <Compositor/OgreCompositorWorkspaceDef.h>


using namespace nimble::RmlOgre;

void NodeConnectionMap::setIn(int id, int channel)
{
	if(id < 0)
		return;

	if(static_cast<std::size_t>(id) >= this->map.size())
		this->map.resize(id + 1);
	this->map[id].inNode = this->currentNode;
	this->map[id].inChannel = channel;
}
void NodeConnectionMap::setOut(int id, int channel)
{
	if(id < 0)
		return;

	if(static_cast<std::size_t>(id) >= this->map.size())
		this->map.resize(id + 1);
	this->map[id].outNode = this->currentNode;
	this->map[id].outChannel = channel;
}

void NodeConnectionMap::setExternal(int externalChannel, int channel)
{
	this->workspace->connectExternal(externalChannel, this->currentNode, channel);
}
