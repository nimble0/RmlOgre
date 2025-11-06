#ifndef NIMBLE_RMLOGRE_RENDEROBJECT_HPP
#define NIMBLE_RMLOGRE_RENDEROBJECT_HPP

#include <OgreMovableObject.h>
#include <OgreRenderable.h>


namespace nimble::RmlOgre {

struct Material;

class RenderObject : public Ogre::MovableObject, public Ogre::Renderable
{
friend struct Material;

	static Ogre::String TYPE_NAME;

public:
	RenderObject() :
		Ogre::MovableObject(nullptr)
	{}
	RenderObject(
		Ogre::IdType id,
		Ogre::ObjectMemoryManager* objectMemoryManager,
		Ogre::SceneManager* sceneManager,
		Ogre::uint8 renderQueueId);

	inline static const Ogre::uint8 RENDER_QUEUE_ID = 3;

	void setVao(Ogre::VertexArrayObject* vao);

	const Ogre::String& getMovableType() const override;

	bool getPolygonModeOverrideable() const override { return false; }

	const Ogre::LightList& getLights() const override
	{
		static Ogre::LightList ll;
		return ll;
	}

	void getWorldTransforms(Ogre::Matrix4*) const override
	{
		OGRE_EXCEPT(Ogre::Exception::ERR_NOT_IMPLEMENTED,
			"nimble::RmlOgre::RenderObject doesn't implements getWorldTransforms.",
			"nimble::RmlOgre::RenderObject::getWorldTransforms");
	}

	void getRenderOperation(Ogre::v1::RenderOperation& op, bool casterPass) override
	{
		OGRE_EXCEPT(Ogre::Exception::ERR_NOT_IMPLEMENTED,
			"nimble::RmlOgre::RenderObject doesn't support the old v1::RenderOperations.",
			"nimble::RmlOgre::RenderObject::getRenderOperation");
	}

	void setPreparedMaterial(const Material& material);
};

}

#endif // NIMBLE_RMLOGRE_RENDEROBJECT_HPP
