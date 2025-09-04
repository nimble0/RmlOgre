#include "NodeConnectionMap.hpp"

#include <Compositor/OgreCompositorNode.h>
#include <Compositor/OgreCompositorWorkspace.h>

#include <stdexcept>


using namespace nimble::RmlOgre;

void NodeConnectionMap::setIn(int id, int channel)
{
	if(id < 0)
		return;

	auto outIter = this->outMap.find(id);
	if(outIter == this->outMap.end())
		throw std::logic_error("No out connection specified for id " + id);

	Ogre::CompositorNode* outNode = outIter->second.first;
	int outChannel = outIter->second.second;
	outNode->connectTo(outChannel, this->currentNode, channel);
}
void NodeConnectionMap::setOut(int id, int channel)
{
	if(id < 0)
		return;

	this->outMap.insert({id, {this->currentNode, channel}});
}

void NodeConnectionMap::setExternal(int externalChannel, int channel)
{
	this->currentNode->connectExternalRT(
		this->workspace->getExternalRenderTargets()[externalChannel],
		channel);
}
