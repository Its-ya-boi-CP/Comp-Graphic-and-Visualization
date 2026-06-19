///////////////////////////////////////////////////////////////////////////////
// scenemanager.h
// ============
// manage the preparing and rendering of 3D scenes - textures, materials, lighting
//
//  AUTHOR: Brian Battersby - SNHU Instructor / Computer Science
//	Created for CS-330-Computational Graphics and Visualization, Nov. 1st, 2023
///////////////////////////////////////////////////////////////////////////////

#pragma once

#include "ShaderManager.h"
#include "ShapeMeshes.h"

#include <string>
#include <vector>

/***********************************************************
 *  SceneManager
 *
 *  This class contains the code for preparing and rendering
 *  3D scenes, including the shader settings.
 ***********************************************************/
class SceneManager
{
public:
	// constructor
	SceneManager(ShaderManager* pShaderManager);
	// destructor
	~SceneManager();

	struct TEXTURE_INFO
	{
		std::string tag;
		uint32_t ID;
	};

	struct OBJECT_MATERIAL
	{
		glm::vec3 diffuseColor;
		glm::vec3 specularColor;
		float shininess;
		std::string tag;
	};

private:
	// pointer to shader manager object
	ShaderManager* m_pShaderManager;
	// pointer to basic shapes object
	ShapeMeshes* m_basicMeshes;
	// total number of loaded textures
	int m_loadedTextures;
	// loaded textures info
	TEXTURE_INFO m_textureIDs[16];
	// defined object materials
	std::vector<OBJECT_MATERIAL> m_objectMaterials;

	// Object drawing helper methods
	void DrawTable();
	void DrawBackWall();
	void DrawBooks();
	void DrawClock();
	void DrawPencilCup();
	void DrawPencils();
	void DrawPlant();

	// load texture images and convert to OpenGL texture data
	bool CreateGLTexture(const char* filename, std::string tag);
	// bind loaded OpenGL textures to slots in memory
	void BindGLTextures();
	// free the loaded OpenGL textures
	void DestroyGLTextures();
	// find a loaded texture by tag
	int FindTextureID(std::string tag);
	int FindTextureSlot(std::string tag);
	// find a defined material by tag
	bool FindMaterial(std::string tag, OBJECT_MATERIAL& material);

	void SetTransformations(
		glm::vec3 scaleXYZ,
		float XrotationDegrees,
		float YrotationDegrees,
		float ZrotationDegrees,
		glm::vec3 positionXYZ);

	void SetShaderColor(
		float redColorValue,
		float greenColorValue,
		float blueColorValue,
		float alphaValue);

	void SetShaderTexture(std::string textureTag);

	void SetTextureUVScale(float u, float v);

	void SetShaderMaterial(std::string materialTag);

public:
	void PrepareScene();
	void RenderScene();
	void LoadSceneTextures();
	void DefineObjectMaterials();
	void SetupSceneLights();
};