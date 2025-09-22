include(OGRE)

set(OGRE-Next_DIR CACHE STRING "Path to OGRE-Next's SDK folder")

set(OGRE_SDK ${OGRE-Next_DIR})
setupOgre(OGRE_SDK, FALSE, FALSE)
