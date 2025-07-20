#include <OgreAbiUtils.h>
#include <OgreArchiveManager.h>
#include <OgreCamera.h>
#include <OgreConfigFile.h>
#include <OgreItem.h>
#include <OgreRoot.h>
#include <OgreTextureGpuManager.h>
#include <OgreWindow.h>

#include <OgreHlmsManager.h>
#include <OgreHlmsUnlit.h>

#include <Compositor/OgreCompositorManager2.h>
#include <Compositor/Pass/OgreCompositorPassProvider.h>

#include <OgreWindowEventUtilities.h>

#include <RmlUi/Core.h>
#include <RmlOgre/Compositor/CompositorPassGeometry.hpp>
#include <RmlOgre/Compositor/CompositorPassGeometryDef.hpp>
#include <RmlOgre/Compositor/CompositorPassRenderQuad.hpp>
#include <RmlOgre/Compositor/CompositorPassRenderQuadDef.hpp>
#include <RmlOgre/RenderInterface.hpp>

#if OGRE_PLATFORM == OGRE_PLATFORM_APPLE
	#include "OSX/macUtils.h"
#endif

#include <cstring>


#define Q(x) #x
#define QUOTE(x) Q(x)

static void register_hlms(Ogre::ConfigFile& cf, const Ogre::String& resourcePath)
{
	using namespace Ogre;

#if OGRE_PLATFORM == OGRE_PLATFORM_APPLE || OGRE_PLATFORM == OGRE_PLATFORM_APPLE_IOS
	String rootHlmsFolder = macBundlePath() + '/' + cf.getSetting("DoNotUseAsResource", "Hlms", "");
#else
	String rootHlmsFolder = resourcePath + cf.getSetting("DoNotUseAsResource", "Hlms", "");
#endif

	if(rootHlmsFolder.empty())
		rootHlmsFolder = "./";
	else if(*(rootHlmsFolder.end() - 1) != '/')
		rootHlmsFolder += "/";

	// At this point rootHlmsFolder should be a valid path to the Hlms data folder

	HlmsUnlit* hlmsUnlit = nullptr;

	ArchiveManager& archiveManager = ArchiveManager::getSingleton();
	auto* hlmsManager = Root::getSingleton().getHlmsManager();

	{
		// Create & Register HlmsUnlit
		// Get the path to all the subdirectories used by HlmsUnlit
		String mainPath;
		StringVector libraryPaths;
		HlmsUnlit::getDefaultPaths(mainPath, libraryPaths);
		Archive* archive = archiveManager.load(rootHlmsFolder + mainPath, "FileSystem", true);
		ArchiveVec archiveLibraryFolders;
		for(auto& libraryPath : libraryPaths)
		{
			archiveLibraryFolders.push_back(archiveManager.load(
				rootHlmsFolder + libraryPath,
				"FileSystem",
				true));
		}

		// Create and register the unlit Hlms
		hlmsUnlit = OGRE_NEW HlmsUnlit(archive, &archiveLibraryFolders);
		hlmsManager->registerHlms(hlmsUnlit);
	}

	RenderSystem* renderSystem = Root::getSingletonPtr()->getRenderSystem();
	if(renderSystem->getName() == "Direct3D11 Rendering Subsystem")
	{
		// Set lower limits 512kb instead of the default 4MB per Hlms in D3D 11.0
		// and below to avoid saturating AMD's discard limit (8MB) or
		// saturate the PCIE bus in some low end machines.
		bool supportsNoOverwriteOnTextureBuffers;
		renderSystem->getCustomAttribute(
			"MapNoOverwriteOnDynamicBufferSRV",
			&supportsNoOverwriteOnTextureBuffers);

		if(!supportsNoOverwriteOnTextureBuffers)
		{
			hlmsUnlit->setTextureBufferDefaultSize( 512 * 1024 );
		}
	}

	Root::getSingleton().getHlmsManager()->useDefaultDatablockFrom(HlmsTypes::HLMS_UNLIT);
}

void add_resource_location(
	const Ogre::String &archName,
	const Ogre::String &typeName,
	const Ogre::String &secName)
{
#if( OGRE_PLATFORM == OGRE_PLATFORM_APPLE ) || ( OGRE_PLATFORM == OGRE_PLATFORM_APPLE_IOS )
	// OS X does not set the working directory relative to the app,
	// In order to make things portable on OS X we need to provide
	// the loading with it's own bundle path location
	Ogre::ResourceGroupManager::getSingleton().addResourceLocation(
		Ogre::String( Ogre::macBundlePath() + "/" + archName ), typeName, secName );
#else
	Ogre::ResourceGroupManager::getSingleton().addResourceLocation(archName, typeName, secName);
#endif
}

void setup_resources(Ogre::ConfigFile& cf)
{
	// Go through all sections & settings in the file
	Ogre::ConfigFile::SectionIterator seci = cf.getSectionIterator();

	Ogre::String secName, typeName, archName;
	while(seci.hasMoreElements())
	{
		secName = seci.peekNextKey();
		Ogre::ConfigFile::SettingsMultiMap *settings = seci.getNext();

		if(secName != "Hlms")
		{
			for(auto i = settings->begin(); i != settings->end(); ++i)
			{
				typeName = i->first;
				archName = i->second;
				add_resource_location(archName, typeName, secName);
			}
		}
	}
}

struct MyCompositorPassProvider : public Ogre::CompositorPassProvider
{
	Ogre::CompositorPassDef* addPassDef(
		Ogre::CompositorPassType passType,
		Ogre::IdString customId,
		Ogre::CompositorTargetDef* parentTargetDef,
		Ogre::CompositorNodeDef* parentNodeDef
	) override
	{
		if(customId == "rml/geometry")
			return OGRE_NEW nimble::RmlOgre::CompositorPassGeometryDef(parentTargetDef);
		else if(customId == "rml/render_quad")
			return OGRE_NEW nimble::RmlOgre::CompositorPassRenderQuadDef(parentNodeDef, parentTargetDef);
		else
			return nullptr;
	}

	Ogre::CompositorPass* addPass(
		const Ogre::CompositorPassDef* definition,
		Ogre::Camera* defaultCamera,
		Ogre::CompositorNode* parentNode,
		const Ogre::RenderTargetViewDef* rtvDef,
		Ogre::SceneManager* sceneManager
	) override
	{
		if(auto* d = dynamic_cast<const nimble::RmlOgre::CompositorPassGeometryDef*>(definition))
			return OGRE_NEW nimble::RmlOgre::CompositorPassGeometry(
				d,
				defaultCamera,
				rtvDef,
				parentNode);
		else if(auto* d = dynamic_cast<const nimble::RmlOgre::CompositorPassRenderQuadDef*>(definition))
			return OGRE_NEW nimble::RmlOgre::CompositorPassRenderQuad(
				d,
				defaultCamera,
				parentNode,
				rtvDef);

		OGRE_EXCEPT(Ogre::Exception::ERR_NOT_IMPLEMENTED, "", "");
	}
};

class MyWindowEventListener final : public Ogre::WindowEventListener
{
	bool mQuit = false;
	Rml::Context* rmlContext = nullptr;
	Ogre::TextureGpu* sceneTexture;

public:
	MyWindowEventListener(
		Rml::Context* rmlContext,
		Ogre::TextureGpu* sceneTexture
	) :
		rmlContext{rmlContext},
		sceneTexture{sceneTexture}
	{}
	void windowClosed( Ogre::Window *rw ) override { mQuit = true; }
	void windowResized( Ogre::Window *rw ) override
	{
		auto size = Rml::Vector2i(rw->getWidth(), rw->getHeight());
		this->rmlContext->SetDimensions(size);
		if(this->sceneTexture->getResidencyStatus() == Ogre::GpuResidency::Resident)
		{
			this->sceneTexture->_transitionTo(Ogre::GpuResidency::OnStorage, nullptr);
			this->sceneTexture->_setNextResidencyStatus(Ogre::GpuResidency::OnStorage);
		}
		this->sceneTexture->setResolution(size.x, size.y);
		if(this->sceneTexture->getResidencyStatus() == Ogre::GpuResidency::OnStorage)
		{
			this->sceneTexture->_transitionTo(Ogre::GpuResidency::Resident, nullptr);
			this->sceneTexture->_setNextResidencyStatus(Ogre::GpuResidency::Resident);
		}
	}

	bool getQuit() const { return mQuit; }
};

#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
INT WINAPI WinMain( HINSTANCE hInst, HINSTANCE hPrevInstance, LPSTR strCmdLine, INT nCmdShow )
#else
int main( int argc, const char *argv[] )
#endif
{
	using namespace Ogre;

	const char* documentPath = QUOTE(RMLOGRE_MEDIA_DIR) "/rml/demo.rml";
#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
	if(std::strlen(strCmdLine))
		documentPath = strCmdLine;
#else
	if(argc >= 2)
		documentPath = argv[1];
#endif

#if OGRE_PLATFORM == OGRE_PLATFORM_APPLE
	const String pluginsFolder = macResourcesPath();
	const String writeAccessFolder = macLogPath();
#else
	const String pluginsFolder = "./";
	const String writeAccessFolder = pluginsFolder;
#endif

#ifndef OGRE_STATIC_LIB
#	if OGRE_DEBUG_MODE && \
		!( ( OGRE_PLATFORM == OGRE_PLATFORM_APPLE ) || ( OGRE_PLATFORM == OGRE_PLATFORM_APPLE_IOS ) )
	const char* pluginsFile = "plugins_d.cfg";
#	else
	const char* pluginsFile = "plugins.cfg";
#	endif
#else
	const char* pluginsFile = nullptr;  // TODO
#endif

	const Ogre::AbiCookie abiCookie = Ogre::generateAbiCookie();
	std::unique_ptr<Root> root(OGRE_NEW Root(
		&abiCookie,
		pluginsFolder + pluginsFile,
		writeAccessFolder + "ogre.cfg",
		writeAccessFolder + "Ogre.log"));

	if(!root->showConfigDialog())
		return -1;

	// Initialize Root
	root->getRenderSystem()->setConfigOption("sRGB Gamma Conversion", "Yes");
	Window *window = root->initialise(true, "RmlOgre example");

	Ogre::CompositorManager2 *compositorManager = root->getCompositorManager2();
	compositorManager->setCompositorPassProvider(
		OGRE_NEW MyCompositorPassProvider());

#if OGRE_PLATFORM == OGRE_PLATFORM_APPLE
	// Note:  macBundlePath works for iOS too. It's misnamed.
	const String resourcePath = Ogre::macResourcesPath();
#elif OGRE_PLATFORM == OGRE_PLATFORM_APPLE_IOS
	const String resourcePath = Ogre::macBundlePath() + "/";
#else
	String resourcePath = "";
#endif

	ConfigFile cf;
	cf.load(resourcePath + "resources.cfg");

	register_hlms(cf, resourcePath);
	setup_resources(cf);
	Ogre::ResourceGroupManager::getSingleton().initialiseAllResourceGroups(true);

	Rml::Initialise();
	Rml::LoadFontFace(QUOTE(RMLOGRE_MEDIA_DIR) "/fonts/LatoLatin-Bold.ttf");
	Rml::LoadFontFace(QUOTE(RMLOGRE_MEDIA_DIR) "/fonts/LatoLatin-BoldItalic.ttf");
	Rml::LoadFontFace(QUOTE(RMLOGRE_MEDIA_DIR) "/fonts/LatoLatin-Italic.ttf");
	Rml::LoadFontFace(QUOTE(RMLOGRE_MEDIA_DIR) "/fonts/LatoLatin-Regular.ttf");

	// Create SceneManager
	const size_t numThreads = 1u;
	SceneManager *sceneManager = root->createSceneManager(ST_GENERIC, numThreads, "ExampleSMInstance");

	// Create & setup camera
	Camera *camera = sceneManager->createCamera("Main Camera");

	// Position it at 500 in Z direction
	camera->setPosition(Vector3(0, 5, 15));
	// Look back along -Z
	camera->lookAt(Vector3(0, 0, 0));
	camera->setNearClipDistance(0.2f);
	camera->setFarClipDistance(1000.0f);
	camera->setAutoAspectRatio(true);

	// Add cube
	Ogre::Item* item = sceneManager->createItem(
		"Cube_d.mesh",
		Ogre::ResourceGroupManager::AUTODETECT_RESOURCE_GROUP_NAME,
		Ogre::SCENE_DYNAMIC);
	auto* itemNode = sceneManager->getRootSceneNode(Ogre::SCENE_DYNAMIC)
		->createChildSceneNode(Ogre::SCENE_DYNAMIC);
	itemNode->attachObject(item);

	Ogre::TextureGpu* sceneTexture = Ogre::Root::getSingleton()
		.getRenderSystem()
		->getTextureGpuManager()
		->createOrRetrieveTexture(
			"SceneOutput",
			Ogre::GpuPageOutStrategy::Discard,
			Ogre::TextureFlags::RenderToTexture,
			Ogre::TextureTypes::Type2D);
	sceneTexture->setPixelFormat(Ogre::PixelFormatGpu::PFG_RGBA16_FLOAT);
	sceneTexture->setResolution(100, 100);
	if(sceneTexture->getResidencyStatus() == Ogre::GpuResidency::OnStorage)
	{
		sceneTexture->_transitionTo(Ogre::GpuResidency::Resident, nullptr);
		sceneTexture->_setNextResidencyStatus(Ogre::GpuResidency::Resident);
	}
	compositorManager->addWorkspace(
		sceneManager,
		sceneTexture,
		camera,
		"ExampleWorkspace",
		true);


	auto resolution = Rml::Vector2i(window->getWidth(), window->getHeight());
	nimble::RmlOgre::RenderInterface renderInterface("Ui", sceneManager, window->getTexture(), sceneTexture);
	Rml::Context* context = Rml::CreateContext(
		"Main",
		resolution,
		&renderInterface
	);
	Rml::ElementDocument* document = context->LoadDocument(documentPath);
	if(document)
		document->Show();


	MyWindowEventListener myWindowEventListener(context, sceneTexture);
	WindowEventUtilities::addWindowEventListener(window, &myWindowEventListener);

	bool bQuit = false;

	while(!bQuit)
	{
		WindowEventUtilities::messagePump();
		bQuit |= myWindowEventListener.getQuit();
		if(!bQuit)
		{
			context->Update();
			renderInterface.BeginFrame();
			context->Render();
			renderInterface.EndFrame();
			bQuit |= !root->renderOneFrame();
		}
	}

	WindowEventUtilities::removeWindowEventListener(window, &myWindowEventListener);

	Rml::Shutdown();

	return 0;
}
