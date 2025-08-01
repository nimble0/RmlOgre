#include "RenderObject.hpp"

#include <OgreRoot.h>
#include <Vao/OgreVaoManager.h>


using namespace nimble::RmlOgre;

Ogre::String RenderObject::TYPE_NAME = "RmlOgre::RenderObject";

RenderObject::RenderObject(
	Ogre::IdType id,
	Ogre::ObjectMemoryManager* objectMemoryManager,
	Ogre::SceneManager* sceneManager,
	Ogre::uint8 renderQueueId
) :
	Ogre::MovableObject(id, objectMemoryManager, sceneManager, renderQueueId)
{
	this->mObjectData.mLocalAabb->setFromAabb(Ogre::Aabb::BOX_INFINITE, mObjectData.mIndex);
	this->mObjectData.mWorldAabb->setFromAabb(Ogre::Aabb::BOX_INFINITE, mObjectData.mIndex);
	this->mObjectData.mLocalRadius[mObjectData.mIndex] = Ogre::Aabb::BOX_INFINITE.getRadius();
	this->mObjectData.mWorldRadius[mObjectData.mIndex] = Ogre::Aabb::BOX_INFINITE.getRadius();
	this->mObjectData.mQueryFlags[mObjectData.mIndex] = Ogre::SceneManager::QUERY_ENTITY_DEFAULT_MASK;

	this->setUseIdentityView(true);

	this->mRenderables.push_back(this);
}

void RenderObject::setVao(Ogre::VertexArrayObject* vao)
{
	this->mVaoPerLod[Ogre::VertexPass::VpNormal].push_back(vao);
	this->mVaoPerLod[Ogre::VertexPass::VpShadow].push_back(vao);
}

const Ogre::String& RenderObject::getMovableType() const
{
	return RenderObject::TYPE_NAME;
}
