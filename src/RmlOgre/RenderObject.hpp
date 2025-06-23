#ifndef NIMBLE_RMLOGRE_RENDEROBJECT_HPP
#define NIMBLE_RMLOGRE_RENDEROBJECT_HPP

#include <OgreMovableObject.h>
#include <OgreRenderable.h>

#include <RmlUi/Core/RenderInterface.h>


namespace nimble::RmlOgre {

class RenderObject : public Ogre::MovableObject, public Ogre::Renderable
{
	static Ogre::String TYPE_NAME;

	bool enableClipping_ = false;
	Rml::Vector2i clipBottomLeft;
	Rml::Vector2i clipTopRight;
	Rml::Matrix4f transform_ = Rml::Matrix4f::Identity();

public:
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

	void enableClipping(bool b) { this->enableClipping_ = b; }
	bool enableClipping() const { return this->enableClipping_; }
	void clip(
		Rml::Vector2i clipBottomLeft,
		Rml::Vector2i clipTopRight)
	{
		this->clipBottomLeft = clipBottomLeft;
		this->clipTopRight = clipTopRight;
	}
	std::pair<Rml::Vector2i, Rml::Vector2i> clipping() const
	{
		return { this->clipBottomLeft, this->clipTopRight };
	}

	void transform(Rml::Matrix4f m) { this->transform_ = m; }
	const Rml::Matrix4f& transform() const { return this->transform_; }
};

}

#endif // NIMBLE_RMLOGRE_RENDEROBJECT_HPP
