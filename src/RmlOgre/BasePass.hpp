#ifndef NIMBLE_RMLOGRE_BASEPASS_HPP
#define NIMBLE_RMLOGRE_BASEPASS_HPP

#include "NodeConnectionMap.hpp"


namespace Ogre {

class CompositorNode;

}

namespace nimble::RmlOgre {

class Workspace;

struct BasePass
{
	static void clearNode(Ogre::CompositorNode* node) {}

	virtual void addExtraConnections(NodeConnectionMap& connections) const {}
	virtual int numRenderObjects() const { return 0; }
	virtual void writePass(Workspace& workspace, Ogre::CompositorNode* node) const {}
};

}

#endif // NIMBLE_RMLOGRE_BASEPASS_HPP
