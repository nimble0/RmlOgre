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

#include <OgreWindowEventUtilities.h>

#if OGRE_PLATFORM == OGRE_PLATFORM_APPLE
	#include "OSX/macUtils.h"
#endif


static void registerHlms(Ogre::ConfigFile& cf, const Ogre::String& resourcePath)
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

void addResourceLocation(
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

void setupResources(Ogre::ConfigFile& cf)
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
				addResourceLocation(archName, typeName, secName);
			}
		}
	}
}

class MyWindowEventListener final : public Ogre::WindowEventListener
{
	bool mQuit = false;

public:
	void windowClosed( Ogre::Window *rw ) override { mQuit = true; }

	bool getQuit() const { return mQuit; }
};

#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
INT WINAPI WinMain( HINSTANCE hInst, HINSTANCE hPrevInstance, LPSTR strCmdLine, INT nCmdShow )
#else
int main( int argc, const char *argv[] )
#endif
{
	using namespace Ogre;

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

	registerHlms(cf, resourcePath);
	setupResources(cf);
	Ogre::ResourceGroupManager::getSingleton().initialiseAllResourceGroups(true);

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


	CompositorManager2 *compositorManager = root->getCompositorManager2();


	Ogre::TextureGpu* uiOutput = Ogre::Root::getSingleton()
		.getRenderSystem()
		->getTextureGpuManager()
		->createOrRetrieveTexture(
			"UiOutput",
			Ogre::GpuPageOutStrategy::Discard,
			Ogre::TextureFlags::RenderToTexture,
			Ogre::TextureTypes::Type2D);
	uiOutput->setResolution(window->getWidth(), window->getHeight());
	uiOutput->setPixelFormat(Ogre::PFG_RGBA16_FLOAT);
	if(uiOutput->getResidencyStatus() == Ogre::GpuResidency::OnStorage)
		uiOutput->scheduleTransitionTo( Ogre::GpuResidency::Resident, nullptr );
	auto* uiWorkspace = compositorManager->addWorkspace(
		sceneManager,
		uiOutput,
		camera,
		"UiWorkspace",
		true);

	auto* mainWorkspace = compositorManager->addWorkspace(
		sceneManager,
		{ window->getTexture(), uiOutput },
		camera,
		"ExampleWorkspace",
		true);


	MyWindowEventListener myWindowEventListener;
	WindowEventUtilities::addWindowEventListener(window, &myWindowEventListener);

	bool bQuit = false;

	while(!bQuit)
	{
		WindowEventUtilities::messagePump();
		bQuit |= myWindowEventListener.getQuit();
		if(!bQuit)
			bQuit |= !root->renderOneFrame();
	}

	WindowEventUtilities::removeWindowEventListener(window, &myWindowEventListener);

	return 0;
}
