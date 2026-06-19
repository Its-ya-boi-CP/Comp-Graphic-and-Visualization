///////////////////////////////////////////////////////////////////////////////
// scenemanager.cpp
// ============
// manage the preparing and rendering of 3D scenes - textures, materials, lighting
//
//  AUTHOR: Brian Battersby - SNHU Instructor / Computer Science
//	Created for CS-330-Computational Graphics and Visualization, Nov. 1st, 2023
///////////////////////////////////////////////////////////////////////////////

#include "SceneManager.h"

#ifndef STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#endif

#include <glm/gtx/transform.hpp>

// declaration of global variables
namespace
{
	const char* g_ModelName = "model";
	const char* g_ColorValueName = "objectColor";
	const char* g_TextureValueName = "objectTexture";
	const char* g_UseTextureName = "bUseTexture";
	const char* g_UseLightingName = "bUseLighting";
}

/***********************************************************
 *  SceneManager()
 *
 *  The constructor for the class
 ***********************************************************/
SceneManager::SceneManager(ShaderManager* pShaderManager)
{
	m_pShaderManager = pShaderManager;
	m_basicMeshes = new ShapeMeshes();

	// initialize the texture collection
	for (int i = 0; i < 16; i++)
	{
		m_textureIDs[i].tag = "/0";
		m_textureIDs[i].ID = -1;
	}

	m_loadedTextures = 0;
}

/***********************************************************
 *  ~SceneManager()
 *
 *  The destructor for the class
 ***********************************************************/
SceneManager::~SceneManager()
{
	m_pShaderManager = NULL;
	delete m_basicMeshes;
	m_basicMeshes = NULL;
}

/***********************************************************
 *  CreateGLTexture()
 *
 *  This method is used for loading textures from image files,
 *  configuring the texture mapping parameters in OpenGL,
 *  generating the mipmaps, and loading the read texture into
 *  the next available texture slot in memory.
 ***********************************************************/
bool SceneManager::CreateGLTexture(const char* filename, std::string tag)
{
	int width = 0;
	int height = 0;
	int colorChannels = 0;
	GLuint textureID = 0;

	// indicate to always flip images vertically when loaded
	stbi_set_flip_vertically_on_load(true);

	// try to parse the image data from the specified image file
	unsigned char* image = stbi_load(
		filename,
		&width,
		&height,
		&colorChannels,
		0);

	// if the image was successfully read from the image file
	if (image)
	{
		std::cout << "Successfully loaded image:" << filename << ", width:" << width << ", height:" << height << ", channels:" << colorChannels << std::endl;

		glGenTextures(1, &textureID);
		glBindTexture(GL_TEXTURE_2D, textureID);

		// set the texture wrapping parameters
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		// set texture filtering parameters
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		// if the loaded image is in RGB format
		if (colorChannels == 3)
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, image);
		// if the loaded image is in RGBA format - it supports transparency
		else if (colorChannels == 4)
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
		else
		{
			std::cout << "Not implemented to handle image with " << colorChannels << " channels" << std::endl;
			return false;
		}

		// generate the texture mipmaps for mapping textures to lower resolutions
		glGenerateMipmap(GL_TEXTURE_2D);

		// free the image data from local memory
		stbi_image_free(image);
		glBindTexture(GL_TEXTURE_2D, 0); // Unbind the texture

		// register the loaded texture and associate it with the special tag string
		m_textureIDs[m_loadedTextures].ID = textureID;
		m_textureIDs[m_loadedTextures].tag = tag;
		m_loadedTextures++;

		return true;
	}

	std::cout << "Could not load image:" << filename << std::endl;

	// Error loading the image
	return false;
}

/***********************************************************
 *  BindGLTextures()
 *
 *  This method is used for binding the loaded textures to
 *  OpenGL texture memory slots.  There are up to 16 slots.
 ***********************************************************/
void SceneManager::BindGLTextures()
{
	for (int i = 0; i < m_loadedTextures; i++)
	{
		// bind textures on corresponding texture units
		glActiveTexture(GL_TEXTURE0 + i);
		glBindTexture(GL_TEXTURE_2D, m_textureIDs[i].ID);
	}
}

/***********************************************************
 *  DestroyGLTextures()
 *
 *  This method is used for freeing the memory in all the
 *  used texture memory slots.
 ***********************************************************/
void SceneManager::DestroyGLTextures()
{
	for (int i = 0; i < m_loadedTextures; i++)
	{
		glGenTextures(1, &m_textureIDs[i].ID);
	}
}

/***********************************************************
 *  FindTextureID()
 *
 *  This method is used for getting an ID for the previously
 *  loaded texture bitmap associated with the passed in tag.
 ***********************************************************/
int SceneManager::FindTextureID(std::string tag)
{
	int textureID = -1;
	int index = 0;
	bool bFound = false;

	while ((index < m_loadedTextures) && (bFound == false))
	{
		if (m_textureIDs[index].tag.compare(tag) == 0)
		{
			textureID = m_textureIDs[index].ID;
			bFound = true;
		}
		else
			index++;
	}

	return(textureID);
}

/***********************************************************
 *  FindTextureSlot()
 *
 *  This method is used for getting a slot index for the previously
 *  loaded texture bitmap associated with the passed in tag.
 ***********************************************************/
int SceneManager::FindTextureSlot(std::string tag)
{
	int textureSlot = -1;
	int index = 0;
	bool bFound = false;

	while ((index < m_loadedTextures) && (bFound == false))
	{
		if (m_textureIDs[index].tag.compare(tag) == 0)
		{
			textureSlot = index;
			bFound = true;
		}
		else
			index++;
	}

	return(textureSlot);
}

/***********************************************************
 *  FindMaterial()
 *
 *  This method is used for getting a material from the previously
 *  defined materials list that is associated with the passed in tag.
 ***********************************************************/
bool SceneManager::FindMaterial(std::string tag, OBJECT_MATERIAL& material)
{
	if (m_objectMaterials.size() == 0)
	{
		return(false);
	}

	int index = 0;
	bool bFound = false;
	while ((index < m_objectMaterials.size()) && (bFound == false))
	{
		if (m_objectMaterials[index].tag.compare(tag) == 0)
		{
			bFound = true;
			material.diffuseColor = m_objectMaterials[index].diffuseColor;
			material.specularColor = m_objectMaterials[index].specularColor;
			material.shininess = m_objectMaterials[index].shininess;
		}
		else
		{
			index++;
		}
	}

	return(bFound);
}

/***********************************************************
 *  SetTransformations()
 *
 *  This method is used for setting the transform buffer
 *  using the passed in transformation values.
 ***********************************************************/
void SceneManager::SetTransformations(
	glm::vec3 scaleXYZ,
	float XrotationDegrees,
	float YrotationDegrees,
	float ZrotationDegrees,
	glm::vec3 positionXYZ)
{
	// variables for this method
	glm::mat4 modelView;
	glm::mat4 scale;
	glm::mat4 rotationX;
	glm::mat4 rotationY;
	glm::mat4 rotationZ;
	glm::mat4 translation;

	// set the scale value in the transform buffer
	scale = glm::scale(scaleXYZ);
	// set the rotation values in the transform buffer
	rotationX = glm::rotate(glm::radians(XrotationDegrees), glm::vec3(1.0f, 0.0f, 0.0f));
	rotationY = glm::rotate(glm::radians(YrotationDegrees), glm::vec3(0.0f, 1.0f, 0.0f));
	rotationZ = glm::rotate(glm::radians(ZrotationDegrees), glm::vec3(0.0f, 0.0f, 1.0f));
	// set the translation value in the transform buffer
	translation = glm::translate(positionXYZ);

	modelView = translation * rotationZ * rotationY * rotationX * scale;

	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setMat4Value(g_ModelName, modelView);
	}
}

/***********************************************************
 *  SetShaderColor()
 *
 *  This method is used for setting the passed in color
 *  into the shader for the next draw command
 ***********************************************************/
void SceneManager::SetShaderColor(
	float redColorValue,
	float greenColorValue,
	float blueColorValue,
	float alphaValue)
{
	// variables for this method
	glm::vec4 currentColor;

	currentColor.r = redColorValue;
	currentColor.g = greenColorValue;
	currentColor.b = blueColorValue;
	currentColor.a = alphaValue;

	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setIntValue(g_UseTextureName, false);
		m_pShaderManager->setVec4Value(g_ColorValueName, currentColor);
	}
}

/***********************************************************
 *  SetShaderTexture()
 *
 *  This method is used for setting the texture data
 *  associated with the passed in ID into the shader.
 ***********************************************************/
void SceneManager::SetShaderTexture(
	std::string textureTag)
{
	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setIntValue(g_UseTextureName, true);

		int textureID = -1;
		textureID = FindTextureSlot(textureTag);
		m_pShaderManager->setSampler2DValue(g_TextureValueName, textureID);
	}
}

/***********************************************************
 *  SetTextureUVScale()
 *
 *  This method is used for setting the texture UV scale
 *  values into the shader.
 ***********************************************************/
void SceneManager::SetTextureUVScale(float u, float v)
{
	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setVec2Value("UVscale", glm::vec2(u, v));
	}
}

/***********************************************************
 *  SetShaderMaterial()
 *
 *  This method is used for passing the material values
 *  into the shader.
 ***********************************************************/
void SceneManager::SetShaderMaterial(
	std::string materialTag)
{
	if (m_objectMaterials.size() > 0)
	{
		OBJECT_MATERIAL material;
		bool bReturn = false;

		bReturn = FindMaterial(materialTag, material);
		if (bReturn == true)
		{
			m_pShaderManager->setVec3Value("material.diffuseColor", material.diffuseColor);
			m_pShaderManager->setVec3Value("material.specularColor", material.specularColor);
			m_pShaderManager->setFloatValue("material.shininess", material.shininess);
		}
	}
}

/**************************************************************/
/*** STUDENTS CAN MODIFY the code in the methods BELOW for  ***/
/*** preparing and rendering their own 3D replicated scenes.***/
/*** Please refer to the code in the OpenGL sample project  ***/
/*** for assistance.                                        ***/
/**************************************************************/
/***********************************************************
 *  LoadSceneTextures()
 *
 *  This method is used for loading the texture image files
 *  that will be applied to objects in the 3D scene.
 ***********************************************************/
void SceneManager::LoadSceneTextures()
{
	bool bReturn = false;

	bReturn = CreateGLTexture("textures/natural-wood-finish.jpg", "tableTexture");
	bReturn = CreateGLTexture("textures/denim_fabric_03_diff_4k.jpg", "wallTexture");
	bReturn = CreateGLTexture("textures/paper.jpg", "paperTexture");
	bReturn = CreateGLTexture("textures/clay.jpg", "clayTexture");
	bReturn = CreateGLTexture("textures/rusty_metal.jpg", "metalTexture");
	bReturn = CreateGLTexture("textures/soil.jpg", "soil");
	bReturn = CreateGLTexture("textures/plastic-seamless-texture.jpg", "plastic");
	bReturn = CreateGLTexture("textures/clock_face.jpg", "clockFaceTexture");
	bReturn = CreateGLTexture("textures/metal-plate.jpg", "metalplate");
	bReturn = CreateGLTexture("textures/seamless-beige-leather.jpg", "bookleather");
	bReturn = CreateGLTexture("textures/tree-green.png", "leaf");
	bReturn = CreateGLTexture("textures/book_cover.jpg", "title");
	bReturn = CreateGLTexture("textures/back_cover.jpg", "backCover");
	bReturn = CreateGLTexture("textures/planner_cover.jpg", "plantitle");

	BindGLTextures();
}


/***********************************************************
 *  DefineObjectMaterials()
 *
 *  This method defines the material properties used by the
 *  lighting shader. Each object can use a material tag that
 *  controls how dull, shiny, or reflective it appears.
 ***********************************************************/
void SceneManager::DefineObjectMaterials()
{
	// Wood material for the desk surface and pencil bodies.
	OBJECT_MATERIAL woodMaterial;
	woodMaterial.diffuseColor = glm::vec3(0.45f, 0.28f, 0.12f);
	woodMaterial.specularColor = glm::vec3(0.12f, 0.09f, 0.06f);
	woodMaterial.shininess = 12.0f;
	woodMaterial.tag = "wood";
	m_objectMaterials.push_back(woodMaterial);

	// Bright cover material for textured book covers.
	OBJECT_MATERIAL coverMaterial;
	coverMaterial.diffuseColor = glm::vec3(1.0f, 1.0f, 1.0f);
	coverMaterial.specularColor = glm::vec3(0.15f, 0.15f, 0.15f);
	coverMaterial.shininess = 8.0f;
	coverMaterial.tag = "cover";
	m_objectMaterials.push_back(coverMaterial);

	// Paper material for books, clock face, and flat paper-like objects.
	OBJECT_MATERIAL paperMaterial;
	paperMaterial.diffuseColor = glm::vec3(0.80f, 0.75f, 0.65f);
	paperMaterial.specularColor = glm::vec3(0.05f, 0.05f, 0.05f);
	paperMaterial.shininess = 4.0f;
	paperMaterial.tag = "paper";
	m_objectMaterials.push_back(paperMaterial);

	// Plastic material for the clock body and pencil cup rim.
	OBJECT_MATERIAL plasticMaterial;
	plasticMaterial.diffuseColor = glm::vec3(0.35f, 0.35f, 0.35f);
	plasticMaterial.specularColor = glm::vec3(0.35f, 0.35f, 0.35f);
	plasticMaterial.shininess = 24.0f;
	plasticMaterial.tag = "plastic";
	m_objectMaterials.push_back(plasticMaterial);

	// Metal material for clock bells, spiral rings, and metal cup surfaces.
	OBJECT_MATERIAL metalMaterial;
	metalMaterial.diffuseColor = glm::vec3(0.55f, 0.55f, 0.58f);
	metalMaterial.specularColor = glm::vec3(0.85f, 0.85f, 0.90f);
	metalMaterial.shininess = 64.0f;
	metalMaterial.tag = "metal";
	m_objectMaterials.push_back(metalMaterial);

	// Clay material for the plant pot.
	OBJECT_MATERIAL clayMaterial;
	clayMaterial.diffuseColor = glm::vec3(0.55f, 0.28f, 0.16f);
	clayMaterial.specularColor = glm::vec3(0.08f, 0.04f, 0.03f);
	clayMaterial.shininess = 8.0f;
	clayMaterial.tag = "clay";
	m_objectMaterials.push_back(clayMaterial);

	// Soil material for the dirt inside the plant pot.
	OBJECT_MATERIAL soilMaterial;
	soilMaterial.diffuseColor = glm::vec3(0.18f, 0.10f, 0.05f);
	soilMaterial.specularColor = glm::vec3(0.02f, 0.02f, 0.02f);
	soilMaterial.shininess = 2.0f;
	soilMaterial.tag = "soil";
	m_objectMaterials.push_back(soilMaterial);

	// Leaf material for the plant leaves.
	OBJECT_MATERIAL leafMaterial;
	leafMaterial.diffuseColor = glm::vec3(0.15f, 0.55f, 0.12f);
	leafMaterial.specularColor = glm::vec3(0.08f, 0.15f, 0.08f);
	leafMaterial.shininess = 10.0f;
	leafMaterial.tag = "leaf";
	m_objectMaterials.push_back(leafMaterial);

	// Dark material for graphite pencil tips and black spiral rings.
	OBJECT_MATERIAL darkMaterial;
	darkMaterial.diffuseColor = glm::vec3(0.02f, 0.02f, 0.02f);
	darkMaterial.specularColor = glm::vec3(0.10f, 0.10f, 0.10f);
	darkMaterial.shininess = 16.0f;
	darkMaterial.tag = "dark";
	m_objectMaterials.push_back(darkMaterial);

	// Wall material - dull so the wall does not look overly shiny.
	OBJECT_MATERIAL wallMaterial;
	wallMaterial.diffuseColor = glm::vec3(0.70f, 0.68f, 0.64f);
	wallMaterial.specularColor = glm::vec3(0.01f, 0.01f, 0.01f);
	wallMaterial.shininess = 1.0f;
	wallMaterial.tag = "wall";
	m_objectMaterials.push_back(wallMaterial);

	// Glossy leather material - used on the leather book cover planes.
	OBJECT_MATERIAL glossyLeatherMaterial;
	glossyLeatherMaterial.diffuseColor = glm::vec3(0.85f, 0.75f, 0.65f);
	glossyLeatherMaterial.specularColor = glm::vec3(0.75f, 0.70f, 0.65f);
	glossyLeatherMaterial.shininess = 100.0f;
	glossyLeatherMaterial.tag = "glossyLeather";
	m_objectMaterials.push_back(glossyLeatherMaterial);
}

/***********************************************************
 *  SetupSceneLights()
 *
 *  This method creates two point lights. The first light is
 *  the main overhead/front light. The second light is a weak
 *  side fill light that keeps shadows visible without making
 *  the scene too dark.
 ***********************************************************/
void SceneManager::SetupSceneLights()
{
	// Enable lighting in the shader.
	m_pShaderManager->setBoolValue(g_UseLightingName, true);

	// Main overhead/front light. This gives the scene most of its brightness.
	m_pShaderManager->setVec3Value("pointLights[0].position", 0.0f, 8.0f, 5.0f);
	m_pShaderManager->setVec3Value("pointLights[0].ambient", 0.22f, 0.22f, 0.20f);
	m_pShaderManager->setVec3Value("pointLights[0].diffuse", 0.95f, 0.90f, 0.80f);
	m_pShaderManager->setVec3Value("pointLights[0].specular", 0.35f, 0.35f, 0.35f);
	m_pShaderManager->setBoolValue("pointLights[0].bActive", true);

	// Weak side fill light. This softens shadows but does not remove them.
	m_pShaderManager->setVec3Value("pointLights[1].position", -6.0f, 4.0f, -3.0f);
	m_pShaderManager->setVec3Value("pointLights[1].ambient", 0.03f, 0.03f, 0.04f);
	m_pShaderManager->setVec3Value("pointLights[1].diffuse", 0.12f, 0.15f, 0.22f);
	m_pShaderManager->setVec3Value("pointLights[1].specular", 0.04f, 0.04f, 0.06f);
	m_pShaderManager->setBoolValue("pointLights[1].bActive", true);
}

/***********************************************************
 *  PrepareScene()
 *
 *  This method is used for preparing the 3D scene by loading
 *  the shapes, textures in memory to support the 3D scene
 *  rendering
 ***********************************************************/
void SceneManager::PrepareScene()
{
	// only one instance of a particular mesh needs to be
	// loaded in memory no matter how many times it is drawn
	// in the rendered 3D scene
	LoadSceneTextures();
	DefineObjectMaterials();
	SetupSceneLights();

	m_basicMeshes->LoadPlaneMesh();
	m_basicMeshes->LoadBoxMesh();
	m_basicMeshes->LoadCylinderMesh();
	m_basicMeshes->LoadConeMesh();
	m_basicMeshes->LoadSphereMesh();
	m_basicMeshes->LoadTorusMesh();
	m_basicMeshes->LoadTaperedCylinderMesh();
}

/***********************************************************
 *  RenderScene()
 *
 *  This method renders the scene by calling separate object
 *  drawing methods. Breaking the scene into smaller methods
 *  keeps the final project code cleaner and easier to grade.
 ***********************************************************/
void SceneManager::RenderScene()
{
	DrawTable();
	DrawBackWall();
	DrawBooks();
	DrawClock();
	DrawPencilCup();
	DrawPencils();
	DrawPlant();
}

/***********************************************************
 *  DrawTable()
 *
 *  Draws the wooden table or bottom plane.
 ***********************************************************/
void SceneManager::DrawTable()
{
	glm::vec3 scaleXYZ;
	float XrotationDegrees = 0.0f;
	float YrotationDegrees = 0.0f;
	float ZrotationDegrees = 0.0f;
	glm::vec3 positionXYZ;

	/*** Set needed transformations before drawing the basic mesh.  ***/
	/*** This same ordering of code should be used for transforming ***/
	/*** and drawing all the basic 3D shapes.						***/
	/******************************************************************/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(20.0f, 1.0f, 10.0f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.0f, 0.0f, 0.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderTexture("tableTexture");
	SetShaderMaterial("wood");
	SetTextureUVScale(4.0f, 4.0f);

	// draw the mesh with transformation values
	m_basicMeshes->DrawPlaneMesh();
	/****************************************************************/
	/****************************************************************/
}

/***********************************************************
 *  DrawBackWall()
 *
 *  Draws the vertical back wall behind the scene.
 ***********************************************************/
void SceneManager::DrawBackWall()
{
	glm::vec3 scaleXYZ;
	float XrotationDegrees = 0.0f;
	float YrotationDegrees = 0.0f;
	float ZrotationDegrees = 0.0f;
	glm::vec3 positionXYZ;

	// Back wall
	scaleXYZ = glm::vec3(20.0f, 1.0f, 10.0f);

	XrotationDegrees = 90.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	positionXYZ = glm::vec3(0.0f, 5.0f, -5.0f);

	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderTexture("wallTexture");
	SetShaderMaterial("wall");
	SetTextureUVScale(5.0f, 1.0f);

	m_basicMeshes->DrawPlaneMesh();
	/****************************************************************/
	/***************************************************************/
	/****************************************************************/
}

/***********************************************************
 *  DrawBooks()
 *
 *  Draws the stacked books and spiral rings.
 ***********************************************************/
void SceneManager::DrawBooks()
{
	glm::vec3 scaleXYZ;
	float XrotationDegrees = 0.0f;
	float YrotationDegrees = 0.0f;
	float ZrotationDegrees = 0.0f;
	glm::vec3 positionXYZ;

	// Bottom book base/body
	scaleXYZ = glm::vec3(6.5f, 0.35f, 5.0f);

	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	positionXYZ = glm::vec3(-5.0f, 0.35f, 0.0f);

	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderTexture("paperTexture");
	SetShaderMaterial("paper");
	m_basicMeshes->DrawBoxMesh();
	/****************************************************************/


	/****************************************************************/
	/****************************************************************/
	// Bottom book top leather plane
	scaleXYZ = glm::vec3(2.8f, 0.0f, 3.5f);

	XrotationDegrees = 0.0f;
	YrotationDegrees = 90.0f;
	ZrotationDegrees = 0.0f;

	positionXYZ = glm::vec3(-5.0f, 0.54f, 0.0f);

	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderTexture("bookleather");
	SetShaderMaterial("glossyLeather");
	SetTextureUVScale(1.0f, 1.0f);
	m_basicMeshes->DrawPlaneMesh();
	/****************************************************************/
	/****************************************************************/


	/****************************************************************/
	/****************************************************************/
	// Bottom book bottom leather plane
	scaleXYZ = glm::vec3(2.8f, 0.0f, 3.5f);

	XrotationDegrees = 0.0f;
	YrotationDegrees = 90.0f;
	ZrotationDegrees = 0.0f;

	positionXYZ = glm::vec3(-5.0f, 0.16f, 0.0f);

	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderTexture("bookleather");
	SetShaderMaterial("glossyLeather");
	SetTextureUVScale(1.0f, 1.0f);
	m_basicMeshes->DrawPlaneMesh();
	/****************************************************************/
	/****************************************************************/

/****************************************************************/
// Spiral rings on back edge of bottom book

	// Ring 1
	scaleXYZ = glm::vec3(0.5f, 0.5f, 0.5f);

	XrotationDegrees = 90.0f;
	YrotationDegrees = 90.0f;
	ZrotationDegrees = 0.0f;

	positionXYZ = glm::vec3(-7.5f, 0.60f, -2.55f);

	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderColor(0.08f, 0.08f, 0.08f, 1.0f);
	SetShaderMaterial("dark");
	m_basicMeshes->DrawTorusMesh();


	// Ring 2
	positionXYZ = glm::vec3(-6.5f, 0.60f, -2.55f);

	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderColor(0.08f, 0.08f, 0.08f, 1.0f);
	SetShaderMaterial("dark");
	m_basicMeshes->DrawTorusMesh();


	// Ring 3
	positionXYZ = glm::vec3(-5.5f, 0.60f, -2.55f);

	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderColor(0.08f, 0.08f, 0.08f, 1.0f);
	SetShaderMaterial("dark");
	m_basicMeshes->DrawTorusMesh();


	// Ring 4
	positionXYZ = glm::vec3(-4.5f, 0.60f, -2.55f);

	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderColor(0.08f, 0.08f, 0.08f, 1.0f);
	SetShaderMaterial("dark");
	m_basicMeshes->DrawTorusMesh();


	// Ring 5
	positionXYZ = glm::vec3(-3.5f, 0.60f, -2.55f);

	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderColor(0.08f, 0.08f, 0.08f, 1.0f);
	SetShaderMaterial("dark");
	m_basicMeshes->DrawTorusMesh();


	// Ring 6
	positionXYZ = glm::vec3(-2.5f, 0.60f, -2.55f);

	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderColor(0.08f, 0.08f, 0.08f, 1.0f);
	SetShaderMaterial("dark");
	m_basicMeshes->DrawTorusMesh();
	/****************************************************************/


	/****************************************************************/
	/****************************************************************/
	// Middle book top title cover
	scaleXYZ = glm::vec3(2.6f, 1.0f, 1.95f);

	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	positionXYZ = glm::vec3(-5.0f, 0.925f, 0.0f);

	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderTexture("title");
	SetShaderMaterial("cover");
	SetTextureUVScale(1.0f, 1.0f);

	m_basicMeshes->DrawPlaneMesh();

	/****************************************************************/
	/****************************************************************/
	// Middle book pages between covers
	scaleXYZ = glm::vec3(5.0f, 0.24f, 3.85f);


	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	positionXYZ = glm::vec3(-5.0f, 0.75f, 0.0f);

	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderTexture("paperTexture");
	SetShaderMaterial("paper");
	SetTextureUVScale(2.0f, 1.0f);

	m_basicMeshes->DrawBoxMesh();
	/****************************************************************/

	/****************************************************************/
	// Middle book back cover
	scaleXYZ = glm::vec3(2.6f, 1.0f, 1.95f);

	XrotationDegrees = 180.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	positionXYZ = glm::vec3(-5.0f, 0.59f, 0.0f);

	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);

	// This turns the title texture OFF for the bottom cover
	SetShaderTexture("backCover");
	SetShaderMaterial("cover");
	SetTextureUVScale(1.0f, 1.0f);

	m_basicMeshes->DrawPlaneMesh();
	/****************************************************************/

	scaleXYZ = glm::vec3(2.25f, 0.12f, 1.5f);
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	positionXYZ = glm::vec3(-5.0f, 1.23f, 0.0f);
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderTexture("plantitle");
	m_basicMeshes->DrawPlaneMesh();
	/****************************************************************/
	// Top book
	scaleXYZ = glm::vec3(4.5f, 0.25f, 3.0f);

	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	positionXYZ = glm::vec3(-5.0f, 1.10f, 0.0f);

	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderColor(0.80f, 0.65f, 0.55f, 1.0f);
	SetShaderMaterial("paper");
	SetTextureUVScale(1.0f, 1.0f);
	m_basicMeshes->DrawBoxMesh();

	/****************************************************************/

	/****************************************************************/
		/****************************************************************/
}

/***********************************************************
 *  DrawClock()
 *
 *  Draws the alarm clock body, face, and bells.
 ***********************************************************/
void SceneManager::DrawClock()
{
	glm::vec3 scaleXYZ;
	float XrotationDegrees = 0.0f;
	float YrotationDegrees = 0.0f;
	float ZrotationDegrees = 0.0f;
	glm::vec3 positionXYZ;

	// Clock body
	scaleXYZ = glm::vec3(0.9f, 0.35f, 0.9f);

	XrotationDegrees = 90.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	positionXYZ = glm::vec3(-5.0f, 2.0f, 0.0f);

	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);

	SetShaderTexture("plastic");
	SetShaderMaterial("plastic");
	SetTextureUVScale(1.0f, 1.0f);

	m_basicMeshes->DrawCylinderMesh();
	/****************************************************************/


	/****************************************************************/
	// Clock face
	scaleXYZ = glm::vec3(0.65f, 0.05f, 0.65f);

	XrotationDegrees = 90.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 90.0f;

	positionXYZ = glm::vec3(-5.0f, 2.0f, 0.31f);

	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderTexture("clockFaceTexture");
	SetShaderMaterial("paper");
	SetTextureUVScale(1.0f, 1.0f);

	m_basicMeshes->DrawCylinderMesh(true, false, false);
	/****************************************************************/


	/****************************************************************/
	// Left clock bell
	scaleXYZ = glm::vec3(0.35f, 0.25f, 0.35f);

	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	positionXYZ = glm::vec3(-5.55f, 2.75f, 0.23f);

	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderTexture("metalTexture");
	SetShaderMaterial("metal");


	m_basicMeshes->DrawSphereMesh();
	/****************************************************************/


	/****************************************************************/
	// Right clock bell
	scaleXYZ = glm::vec3(0.35f, 0.25f, 0.35f);

	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	positionXYZ = glm::vec3(-4.45f, 2.75f, 0.23f);

	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderTexture("metalTexture");
	SetShaderMaterial("metal");

	m_basicMeshes->DrawSphereMesh();
	/****************************************************************/
	/****************************************************************/
}

/***********************************************************
 *  DrawPencilCup()
 *
 *  Draws the metal pencil cup and its top rim.
 ***********************************************************/
void SceneManager::DrawPencilCup()
{
	glm::vec3 scaleXYZ;
	float XrotationDegrees = 0.0f;
	float YrotationDegrees = 0.0f;
	float ZrotationDegrees = 0.0f;
	glm::vec3 positionXYZ;

	// Pencil cup
	scaleXYZ = glm::vec3(0.9f, 1.8f, 0.9f);

	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	positionXYZ = glm::vec3(0.5f, 0.01f, 0.0f);

	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderTexture("metalplate");
	SetShaderMaterial("metal");

	m_basicMeshes->DrawCylinderMesh(false, true, true);

	// Pencil cup top rim
	scaleXYZ = glm::vec3(.80f, 0.4f, 0.75f);

	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	positionXYZ = glm::vec3(0.5f, 1.80f, 0.0f);

	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.12f, 0.12f, 0.12f, 1.0f);
	SetShaderMaterial("plastic");

	m_basicMeshes->DrawTorusMesh();

	/****************************************************************/


	/****************************************************************/
}

/***********************************************************
 *  DrawPencils()
 *
 *  Draws the pencil bodies, wood tips, and dark graphite tips.
 ***********************************************************/
void SceneManager::DrawPencils()
{
	glm::vec3 scaleXYZ;
	float XrotationDegrees = 0.0f;
	float YrotationDegrees = 0.0f;
	float ZrotationDegrees = 0.0f;
	glm::vec3 positionXYZ;

	// Pencil 1
	scaleXYZ = glm::vec3(0.08f, 3.80f, 0.08f);

	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	positionXYZ = glm::vec3(0.2f, 0.2f, 0.0f);

	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderColor(1.0f, 0.25f, 0.10f, 1.0f);
	SetShaderMaterial("wood");
	m_basicMeshes->DrawCylinderMesh();
	/****************************************************************/
	/****************************************************************/
	/****************************************************************/
	// Small cone exactly on top 
	scaleXYZ = glm::vec3(0.025f, 0.15f, 0.02f);

	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	positionXYZ = glm::vec3(0.2f, 4.15f, 0.00f);

	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderColor(0.35f, 0.08f, 0.03f, 1.0f);
	SetShaderMaterial("dark");
	m_basicMeshes->DrawConeMesh();

	/****************************************************************/
	// Pencil 1 tip
	scaleXYZ = glm::vec3(0.08f, 0.22f, 0.08f);

	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	positionXYZ = glm::vec3(0.2f, 4.0f, 0.0f);
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderColor(0.95f, 0.80f, 0.55f, 1.0f);
	SetShaderMaterial("paper");
	m_basicMeshes->DrawConeMesh();
	/****************************************************************/


	/****************************************************************/
	// Pencil 2
	scaleXYZ = glm::vec3(0.08f, 3.5f, 0.08f);

	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 8.0f;

	positionXYZ = glm::vec3(.75f, 0.22f, 0.15f);
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderColor(1.0f, 0.45f, 0.05f, 1.0f);
	SetShaderMaterial("wood");
	m_basicMeshes->DrawCylinderMesh();
	/****************************************************************/
	/****************************************************************/

	/****************************************************************/
	// Pencil 2 tip
	scaleXYZ = glm::vec3(0.08f, 0.22f, 0.08f);

	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 8.0f;

	positionXYZ = glm::vec3(0.2637f, 3.68f, 0.15f);

	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderColor(0.95f, 0.80f, 0.55f, 1.0f);
	SetShaderMaterial("paper");
	m_basicMeshes->DrawConeMesh();
	/****************************************************************/
	// Small cone exactly on top 
	scaleXYZ = glm::vec3(0.025f, 0.15f, 0.02f);

	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 8.0f;

	positionXYZ = glm::vec3(0.25f, 3.8f, 0.15f);

	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderColor(0.35f, 0.08f, 0.03f, 1.0f);
	SetShaderMaterial("dark");
	m_basicMeshes->DrawConeMesh();
}

/***********************************************************
 *  DrawPlant()
 *
 *  Draws the plant pot, soil, and leaves.
 ***********************************************************/
void SceneManager::DrawPlant()
{
	glm::vec3 scaleXYZ;
	float XrotationDegrees = 0.0f;
	float YrotationDegrees = 0.0f;
	float ZrotationDegrees = 0.0f;
	glm::vec3 positionXYZ;

	// Plant pot - tapered cylinder
	scaleXYZ = glm::vec3(0.9f, 1.0f, 0.9f);

	XrotationDegrees = 180.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	positionXYZ = glm::vec3(5.0f, 1.0f, 0.0f);

	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderTexture("clayTexture");
	SetShaderMaterial("clay");

	m_basicMeshes->DrawTaperedCylinderMesh();
	/****************************************************************/
	/****************************************************************/
	// Dirt inside plant pot
	scaleXYZ = glm::vec3(0.65f, 0.08f, 0.65f);

	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	positionXYZ = glm::vec3(5.0f, 1.0f, 0.0f);

	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderTexture("soil");
	SetShaderMaterial("soil");
	SetTextureUVScale(1.0f, 1.0f);

	m_basicMeshes->DrawCylinderMesh();
	/****************************************************************/
	/****************************************************************/
	// Middle plant leaf
	scaleXYZ = glm::vec3(0.25f, 1.4f, 0.25f);

	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	positionXYZ = glm::vec3(5.0f, 1.0f, 0.0f);

	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderTexture("leaf");
	SetShaderMaterial("leaf");
	SetTextureUVScale(1.0f, 1.0f);

	m_basicMeshes->DrawConeMesh();
	/****************************************************************/


	/****************************************************************/
	// Left plant leaf
	scaleXYZ = glm::vec3(0.25f, 1.2f, 0.25f);

	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 25.0f;

	positionXYZ = glm::vec3(4.65f, 1.0f, 0.0f);

	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderTexture("leaf");
	SetShaderMaterial("leaf");
	SetTextureUVScale(1.0f, 1.0f);

	m_basicMeshes->DrawConeMesh();
	/****************************************************************/


	/****************************************************************/
	// Right plant leaf
	scaleXYZ = glm::vec3(0.25f, 1.2f, 0.25f);

	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = -25.0f;

	positionXYZ = glm::vec3(5.35f, 1.0f, 0.0f);

	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderTexture("leaf");
	SetShaderMaterial("leaf");
	SetTextureUVScale(1.0f, 1.0f);

	m_basicMeshes->DrawConeMesh();
	/****************************************************************/
	// front plant leaf
	scaleXYZ = glm::vec3(0.25f, 1.2f, 0.25f);

	XrotationDegrees = 50.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	positionXYZ = glm::vec3(5.0f, 1.0f, 0.0f);

	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderTexture("leaf");
	SetShaderMaterial("leaf");
	SetTextureUVScale(1.0f, 1.0f);

	m_basicMeshes->DrawConeMesh();
	// back plant leaf
	scaleXYZ = glm::vec3(0.25f, 1.2f, 0.25f);

	XrotationDegrees = 50.0f;
	YrotationDegrees = 180.0f;
	ZrotationDegrees = 0.0f;

	positionXYZ = glm::vec3(5.0f, 1.0f, 0.0f);

	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderTexture("leaf");
	SetShaderMaterial("leaf");
	SetTextureUVScale(1.0f, 1.0f);
	m_basicMeshes->DrawConeMesh();
}


