///////////////////////////////////////////////////////////////////////////////
// shadermanager.cpp
// ============
// manage the loading and rendering of 3D scenes
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
	// free the allocated objects
	m_pShaderManager = NULL;
	if (NULL != m_basicMeshes)
	{
		delete m_basicMeshes;
		m_basicMeshes = NULL;
	}

	// free the allocated OpenGL textures
	DestroyGLTextures();
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
			material.ambientColor = m_objectMaterials[index].ambientColor;
			material.ambientStrength = m_objectMaterials[index].ambientStrength;
			material.diffuseColor = m_objectMaterials[index].diffuseColor;
			material.specularColor = m_objectMaterials[index].specularColor;
			material.shininess = m_objectMaterials[index].shininess;
		}
		else
		{
			index++;
		}
	}

	return(true);
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

	modelView = translation * rotationX * rotationY * rotationZ * scale;

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
			m_pShaderManager->setVec3Value("material.ambientColor", material.ambientColor);
			m_pShaderManager->setFloatValue("material.ambientStrength", material.ambientStrength);
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
  *  This method is used for preparing the 3D scene by loading
  *  the shapes, textures in memory to support the 3D scene
  *  rendering
  ***********************************************************/
void SceneManager::LoadSceneTextures()
{


	bool bReturn = false;
	// load the textures from the image files and bind them to texture slots

	bReturn = CreateGLTexture(
		"textures/ground.jpg", "ground");

	bReturn = CreateGLTexture(
		"textures/fabric02.jpg", "fabric02");

	bReturn = CreateGLTexture(
		"textures/Onyx1.jpg", "Onyx1");

	bReturn = CreateGLTexture(
		"textures/wood.jpg", "wood");

	bReturn = CreateGLTexture(
		"textures/keyboard.jpg", "keyboard");

	bReturn = CreateGLTexture(
		"textures/laptop.jpg", "laptop");
	
	bReturn = CreateGLTexture(
		"textures/matrix.jpg", "matrix");

	bReturn = CreateGLTexture(
		"textures/mousepad.jpg", "mousepad");

	bReturn = CreateGLTexture(
		"textures/blackmetal.jpg", "black_metal");

	bReturn = CreateGLTexture(
		"textures/stainless_end.jpg", "stainless");

	bReturn = CreateGLTexture(
		"textures/mouse1.jpg", "mouse1");

	bReturn = CreateGLTexture(
		"textures/glass.jpg", "glass");

	bReturn = CreateGLTexture(
		"textures/wax.png", "wax");

	bReturn = CreateGLTexture(
		"textures/flame.jpg", "flame");
	
	bReturn = CreateGLTexture(
		"textures/cap2.jpg", "cap2");

	bReturn = CreateGLTexture(
		"textures/pepper.jpg", "pepper");

	bReturn = CreateGLTexture(
		"textures/salt1.jpg", "salt1");

	BindGLTextures();
}

/***********************************************************
 *  DefineObjectMaterials()
 *
 *  This method is used for configuring the various material
 *  settings for all of the objects within the 3D scene.
 ***********************************************************/
void SceneManager::DefineObjectMaterials()
{
	OBJECT_MATERIAL groundMaterial;
	groundMaterial.ambientColor = glm::vec3(0.05f, 0.03f, 0.02f); // Dark brown ambient
	groundMaterial.ambientStrength = 0.2f; // Moderate ambient contribution
	groundMaterial.diffuseColor = glm::vec3(0.45f, 0.3f, 0.2f); // Warm brown for wood
	groundMaterial.specularColor = glm::vec3(0.2f, 0.2f, 0.2f); // Subtle specular for matte finish
	groundMaterial.shininess = 3.0f; // Low shininess for wood grain
	groundMaterial.tag = "ground1";
	m_objectMaterials.push_back(groundMaterial);

	OBJECT_MATERIAL woodMaterial;
	woodMaterial.ambientColor = glm::vec3(0.05f, 0.03f, 0.02f); // Dark brown ambient
	woodMaterial.ambientStrength = 0.2f;
	woodMaterial.diffuseColor = glm::vec3(0.45f, 0.3f, 0.2f); // Warm brown for wood
	woodMaterial.specularColor = glm::vec3(0.2f, 0.2f, 0.2f); // Subtle specular for matte finish
	woodMaterial.shininess = 3.0f; // Low shininess for wood grain
	woodMaterial.tag = "wood1";
	m_objectMaterials.push_back(woodMaterial);

	OBJECT_MATERIAL glassMaterial;
	glassMaterial.ambientColor = glm::vec3(0.05f, 0.05f, 0.05f); // Low ambient for transparency
	glassMaterial.ambientStrength = 0.1f;
	glassMaterial.diffuseColor = glm::vec3(0.1f, 0.1f, 0.15f); // Slight blue tint for glass
	glassMaterial.specularColor = glm::vec3(0.4f, 0.4f, 0.4f); // Bright highlights for glossy surface
	glassMaterial.shininess = 4.0f; // High shininess for glass
	glassMaterial.tag = "glass1";
	m_objectMaterials.push_back(glassMaterial);

	OBJECT_MATERIAL fabricMaterial;
	fabricMaterial.ambientColor = glm::vec3(0.05f, 0.05f, 0.05f); // Dim ambient
	fabricMaterial.ambientStrength = 0.2f;
	fabricMaterial.diffuseColor = glm::vec3(0.6f, 0.4f, 0.3f); // Warm beige for fabric
	fabricMaterial.specularColor = glm::vec3(0.1f, 0.1f, 0.1f); // Minimal specular for matte texture
	fabricMaterial.shininess = 0.1f; // Very low shininess for fabric
	fabricMaterial.tag = "fabric03";
	m_objectMaterials.push_back(fabricMaterial);

	OBJECT_MATERIAL onyxMaterial;
	onyxMaterial.ambientColor = glm::vec3(0.05f, 0.05f, 0.05f); // Dark ambient for laptop base
	onyxMaterial.ambientStrength = 0.2f;
	onyxMaterial.diffuseColor = glm::vec3(0.2f, 0.2f, 0.2f); // Dark gray for matte plastic
	onyxMaterial.specularColor = glm::vec3(0.3f, 0.3f, 0.3f); // Moderate highlights for semi-glossy finish
	onyxMaterial.shininess = 4.2f; // Moderate shininess for plastic laptop base
	onyxMaterial.tag = "Onyx2";
	m_objectMaterials.push_back(onyxMaterial);

	OBJECT_MATERIAL laptopMaterial;
	laptopMaterial.ambientColor = glm::vec3(0.05f, 0.05f, 0.05f); // Dark ambient
	laptopMaterial.ambientStrength = 0.2f;
	laptopMaterial.diffuseColor = glm::vec3(0.3f, 0.3f, 0.3f); // Neutral gray for laptop body
	laptopMaterial.specularColor = glm::vec3(0.4f, 0.4f, 0.4f); // Moderate specular for plastic/metal
	laptopMaterial.shininess = 4.0f; // Slightly shiny for laptop surface
	laptopMaterial.tag = "laptop1";
	m_objectMaterials.push_back(laptopMaterial);

	OBJECT_MATERIAL blackmetalMaterial;
	blackmetalMaterial.ambientColor = glm::vec3(0.05f, 0.05f, 0.05f); // Dark ambient
	blackmetalMaterial.ambientStrength = 0.2f;
	blackmetalMaterial.diffuseColor = glm::vec3(0.2f, 0.2f, 0.2f); // Dark gray for matte black metal
	blackmetalMaterial.specularColor = glm::vec3(0.5f, 0.5f, 0.5f); // Bright highlights for metallic sheen
	blackmetalMaterial.shininess = 0.9f; // Low shininess for matte black metal
	blackmetalMaterial.tag = "black_metal1";
	m_objectMaterials.push_back(blackmetalMaterial);

	OBJECT_MATERIAL stainlessMaterial;
	stainlessMaterial.ambientColor = glm::vec3(0.1f, 0.1f, 0.1f); // Neutral ambient
	stainlessMaterial.ambientStrength = 0.2f;
	stainlessMaterial.diffuseColor = glm::vec3(0.7f, 0.7f, 0.7f); // Bright silver for stainless steel
	stainlessMaterial.specularColor = glm::vec3(0.9f, 0.9f, 0.9f); // Strong highlights for polished metal
	stainlessMaterial.shininess = 40.0f; // High shininess for stainless steel
	stainlessMaterial.tag = "stainless_end1";
	m_objectMaterials.push_back(stainlessMaterial);

	OBJECT_MATERIAL mouseMaterial;
	mouseMaterial.ambientColor = glm::vec3(0.05f, 0.05f, 0.05f); // Dark ambient
	mouseMaterial.ambientStrength = 0.2f;
	mouseMaterial.diffuseColor = glm::vec3(0.3f, 0.3f, 0.3f); // Neutral gray for mouse
	mouseMaterial.specularColor = glm::vec3(0.4f, 0.4f, 0.4f); // Moderate highlights for plastic
	mouseMaterial.shininess = 6.0f; // Moderate shininess for mouse surface
	mouseMaterial.tag = "mouse2";
	m_objectMaterials.push_back(mouseMaterial);

	OBJECT_MATERIAL waxMaterial;
	waxMaterial.ambientColor = glm::vec3(0.1f, 0.1f, 0.1f); // Neutral ambient
	waxMaterial.ambientStrength = 0.2f;
	waxMaterial.diffuseColor = glm::vec3(0.9f, 0.85f, 0.8f); // Creamy white for candle wax
	waxMaterial.specularColor = glm::vec3(0.3f, 0.3f, 0.3f); // Subtle highlights for waxy surface
	waxMaterial.shininess = 7.0f; // Low shininess for wax
	waxMaterial.tag = "wax1";
	m_objectMaterials.push_back(waxMaterial);

	OBJECT_MATERIAL capMaterial;
	capMaterial.ambientColor = glm::vec3(0.1f, 0.1f, 0.1f); // Neutral ambient
	capMaterial.ambientStrength = 0.2f;
	capMaterial.diffuseColor = glm::vec3(0.8f, 0.8f, 0.8f); // Silver for salt/pepper shaker cap
	capMaterial.specularColor = glm::vec3(0.9f, 0.9f, 0.9f); // Bright highlights for metallic cap
	capMaterial.shininess = 1.0f; // High shininess for polished metal
	capMaterial.tag = "cap3";
	m_objectMaterials.push_back(capMaterial);

	OBJECT_MATERIAL keyboardMaterial;
	keyboardMaterial.ambientColor = glm::vec3(0.05f, 0.05f, 0.05f); // Dark ambient
	keyboardMaterial.ambientStrength = 0.2f;
	keyboardMaterial.diffuseColor = glm::vec3(0.2f, 0.2f, 0.2f); // Dark gray for keyboard
	keyboardMaterial.specularColor = glm::vec3(0.3f, 0.3f, 0.3f); // Subtle highlights for plastic keys
	keyboardMaterial.shininess = 0.2f; // Moderate shininess for keyboard
	keyboardMaterial.tag = "keyboard1";
	m_objectMaterials.push_back(keyboardMaterial);

	OBJECT_MATERIAL matrixMaterial;
	matrixMaterial.ambientColor = glm::vec3(0.01f, 0.05f, 0.01f);     // Subtle green ambient
	matrixMaterial.ambientStrength = 0.1f;                            // Low ambient glow
	matrixMaterial.diffuseColor = glm::vec3(0.0f, 0.2f, 0.0f);         // Dark green when lit
	matrixMaterial.specularColor = glm::vec3(0.01f, 0.02f, 0.01f);     // Faint green specular
	matrixMaterial.shininess = 4.0f;                                  // Low shininess = soft highlights
	matrixMaterial.tag = "Matrix1";
	m_objectMaterials.push_back(matrixMaterial);

	OBJECT_MATERIAL mousepadMaterial;
	mousepadMaterial.ambientColor = glm::vec3(0.05f, 0.05f, 0.05f); // Dim ambient
	mousepadMaterial.ambientStrength = 0.2f;
	mousepadMaterial.diffuseColor = glm::vec3(0.3f, 0.3f, 0.3f); // Neutral gray for mousepad
	mousepadMaterial.specularColor = glm::vec3(0.1f, 0.1f, 0.1f); // Minimal specular for fabric
	mousepadMaterial.shininess = 0.3f; // Very low shininess for mousepad
	mousepadMaterial.tag = "mousepad1";
	m_objectMaterials.push_back(mousepadMaterial);

	OBJECT_MATERIAL flameMaterial;
	flameMaterial.ambientColor = glm::vec3(0.1f, 0.05f, 0.02f); // Warm ambient for flame
	flameMaterial.ambientStrength = 0.3f;
	flameMaterial.diffuseColor = glm::vec3(1.0f, 0.5f, 0.2f); // Orange for flame
	flameMaterial.specularColor = glm::vec3(0.9f, 0.6f, 0.3f); // Bright highlights for emissive effect
	flameMaterial.shininess = 3.0f; // Moderate shininess for flame glow
	flameMaterial.tag = "flame1";
	m_objectMaterials.push_back(flameMaterial);

	OBJECT_MATERIAL saltMaterial;
	saltMaterial.ambientColor = glm::vec3(0.1f, 0.1f, 0.1f); // Neutral ambient
	saltMaterial.ambientStrength = 0.2f;
	saltMaterial.diffuseColor = glm::vec3(0.1f, 0.1f, 0.1f); // Near-white for salt
	saltMaterial.specularColor = glm::vec3(0.3f, 0.3f, 0.3f); // Subtle highlights for granular texture
	saltMaterial.shininess = 0.0f; // Low shininess for salt
	saltMaterial.tag = "salt2";
	m_objectMaterials.push_back(saltMaterial);

	OBJECT_MATERIAL pepperMaterial;
	pepperMaterial.ambientColor = glm::vec3(0.05f, 0.05f, 0.05f); // Dark ambient
	pepperMaterial.ambientStrength = 0.2f;
	pepperMaterial.diffuseColor = glm::vec3(0.2f, 0.15f, 0.1f); // Dark brown for pepper
	pepperMaterial.specularColor = glm::vec3(0.2f, 0.2f, 0.2f); // Subtle highlights for granular texture
	pepperMaterial.shininess = 1.0f; // Low shininess for pepper
	pepperMaterial.tag = "pepper1";
	m_objectMaterials.push_back(pepperMaterial);

}

/***********************************************************
 *  SetupSceneLights()
 *
 *  This method is called to add and configure the light
 *  sources for the 3D scene.  There are up to 4 light sources.
 ***********************************************************/
void SceneManager::SetupSceneLights()
{
	// this line of code is NEEDED for telling the shaders to render 
	// the 3D scene with custom lighting - to use the default rendered 
	// lighting then comment out the following line
	m_pShaderManager->setBoolValue(g_UseLightingName, true);

	// located at the bottom of the scene
	m_pShaderManager->setVec3Value("lightSources[0].position", 0.0f, -6.0f, -12.0f);
	m_pShaderManager->setVec3Value("lightSources[0].ambientColor", 0.1f, 0.1f, 0.1f);
	m_pShaderManager->setVec3Value("lightSources[0].diffuseColor", 0.1f, 0.1f, 0.1f);
	m_pShaderManager->setVec3Value("lightSources[0].specularColor", 0.5f, 0.5f, 0.5f);
	m_pShaderManager->setFloatValue("lightSources[0].focalStrength", 0.0001f);
	m_pShaderManager->setFloatValue("lightSources[0].specularIntensity", 0.4f);
	
	// located above the scene
	m_pShaderManager->setVec3Value("lightSources[1].position", 0.0f, 8.0f, -500.0f);
	m_pShaderManager->setVec3Value("lightSources[1].ambientColor", 0.1f, 0.1f, 0.1f);
	m_pShaderManager->setVec3Value("lightSources[1].diffuseColor", 0.1f, 0.1f, 0.1f);
	m_pShaderManager->setVec3Value("lightSources[1].specularColor", 0.5f, 0.5f, 0.5f);
	m_pShaderManager->setFloatValue("lightSources[1].focalStrength", 0.0001f);
	m_pShaderManager->setFloatValue("lightSources[1].specularIntensity", 0.2f);

	// located to the left of the scene
	m_pShaderManager->setVec3Value("lightSources[2].position", -50000.0f, 10.5f, -45.0f);
	m_pShaderManager->setVec3Value("lightSources[2].ambientColor", 0.1f, 0.1f, 0.1f);
	m_pShaderManager->setVec3Value("lightSources[2].diffuseColor", 0.001f, 0.001f, 0.001f);
	m_pShaderManager->setVec3Value("lightSources[2].specularColor", 0.1f, 0.1f, 0.1f);
	m_pShaderManager->setFloatValue("lightSources[2].focalStrength", 0.0001f);
	m_pShaderManager->setFloatValue("lightSources[2].specularIntensity", 0.01f);

	//located to the right of the scene
	m_pShaderManager->setVec3Value("lightSources[3].position", 900.0f, 8.0f, -2.0f);
	m_pShaderManager->setVec3Value("lightSources[3].ambientColor", 0.1f, 0.1f, 0.1f);
	m_pShaderManager->setVec3Value("lightSources[3].diffuseColor", 0.001f, 0.001f, 0.001f);
	m_pShaderManager->setVec3Value("lightSources[3].specularColor", 0.1f, 0.1f, 0.1f);
	m_pShaderManager->setFloatValue("lightSources[3].focalStrength", 0.0001f);
	m_pShaderManager->setFloatValue("lightSources[3].specularIntensity", 0.1f);

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
	
	// load the textures for the 3D scene
	LoadSceneTextures();
	// define the materials that will be used for the objects
	// in the 3D scene
	DefineObjectMaterials();
	// add and defile the light sources for the 3D scene
	SetupSceneLights();

	// only one instance of a particular mesh needs to be
	// loaded in memory no matter how many times it is drawn
	// in the rendered 3D scene
	m_basicMeshes->LoadPlaneMesh();
	m_basicMeshes->LoadBoxMesh();  // Added: Load box mesh for laptop components
	m_basicMeshes->LoadCylinderMesh();  // Added: Load cylinder mesh for table legs
	m_basicMeshes->LoadConeMesh();
	m_basicMeshes->LoadSphereMesh();
}

/***********************************************************
 *  RenderScene()
 *
 *  This method is used for rendering the 3D scene by
 *  transforming and drawing the basic 3D shapes
 ***********************************************************/
void SceneManager::RenderScene()

{
	// declare the variables for the transformations
	glm::vec3 scaleXYZ;
	float XrotationDegrees = 0.0f;
	float YrotationDegrees = 0.0f;
	float ZrotationDegrees = 0.0f;
	glm::vec3 positionXYZ;

	//******************** Ground Plane *******************/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(45.0f, 1.0f, 45.0f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.0f, -7.1f, 0.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(scaleXYZ,XrotationDegrees,YrotationDegrees,ZrotationDegrees,positionXYZ);

	//SetShaderColor(1, 1, 1, 1);

	SetShaderTexture("ground");
	SetTextureUVScale(5.0f, 5.0f);
	SetShaderMaterial("ground1");

	// draw the mesh with transformation values - this plane is used for the base
	m_basicMeshes->DrawPlaneMesh();

	/****************** Kitchen Table *******************/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(39.8f, 0.9f, 19.8f);
	// set the XYZ rotation for the mesh 
	XrotationDegrees = 0.0f;
	YrotationDegrees = 60.0f;  // Rotateed for angled view
	ZrotationDegrees = 0.0f;
	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.0f, -0.1f, -20.0f);
	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(scaleXYZ,XrotationDegrees,YrotationDegrees,ZrotationDegrees,positionXYZ);
	//SetShaderColor(0.6f, 0.4f, 0.2f, 1.0f);  // Brown table

	SetShaderTexture("wood"); // Using fabric02 texture for table cloth; I took a photo of my table cloth then uploaded it to the textures folder
	SetTextureUVScale(1.0f, 1.0f);
	SetShaderMaterial("wood1");


	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();

	/****************** Kitchen Table Cloth Plane (Top) *******************/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(20.0f, 1.0f, 10.0f);
	// set the XYZ rotation for the mesh 
	XrotationDegrees = 0.0f;
	YrotationDegrees = 60.0f;  // Rotateed for angled view
	ZrotationDegrees = 0.0f;
	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.0f, 1.0f, -20.0f);
	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(scaleXYZ,XrotationDegrees,YrotationDegrees,ZrotationDegrees,positionXYZ);
	//SetShaderColor(0.6f, 0.4f, 0.2f, 1.0f);  // Brown table
	
	SetShaderTexture("fabric02"); // Using fabric02 texture for table cloth; I took a photo of my table cloth then uploaded it to the textures folder
	SetTextureUVScale(1.0f, 1.0f);
	SetShaderMaterial("fabric03");
	
	
	// draw the mesh with transformation values
	m_basicMeshes->DrawPlaneMesh();

	/****************** Kitchen Table Cloth Plane (Front) *******************/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(1.0f, 1.0f, 10.0f);
	// set the XYZ rotation for the mesh 
	XrotationDegrees = 0.0f;
	YrotationDegrees = 60.0f;  // Rotateed for angled view
	ZrotationDegrees = 90.0f;
	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-9.98f, -0.020f, -2.73f);
	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(scaleXYZ,XrotationDegrees,YrotationDegrees,ZrotationDegrees,positionXYZ);
	//SetShaderColor(0.6f, 0.4f, 0.2f, 1.0f);  // Brown table

	SetShaderTexture("fabric02"); // Using fabric02 texture for a side table cloth; I took a photo of my side table cloth then uploaded it to the textures folder but it did not look good
	SetTextureUVScale(1.0f, 1.0f); // This was the best looking scale for the texture
	SetShaderMaterial("fabric03");

	// draw the mesh with transformation values
	m_basicMeshes->DrawPlaneMesh();

	/****************** Kitchen Table Cloth Plane (Back) *******************/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(1.0f, 1.0f, 10.0f);
	// set the XYZ rotation for the mesh 
	XrotationDegrees = 0.0f;
	YrotationDegrees = 60.0f;  // Rotateed for angled view
	ZrotationDegrees = 90.0f;
	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(10.0f, -0.02f, -37.33f);
	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(scaleXYZ,XrotationDegrees,YrotationDegrees,ZrotationDegrees,positionXYZ);
	//SetShaderColor(0.6f, 0.4f, 0.2f, 1.0f);  // Brown; Different color to set position

	SetShaderTexture("fabric02"); //Using fabric02 texture for a side table cloth; I took a photo of my side table cloth then uploaded it to the textures folder but it did not look good
	SetTextureUVScale(1.0f, 1.0f);
	SetShaderMaterial("fabric03");

	// draw the mesh with transformation values
	m_basicMeshes->DrawPlaneMesh();


	/****************** Kitchen Table Cloth Plane (Left Side) *******************/
// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(1.0f, 1.0f, 20.0f);
	// set the XYZ rotation for the mesh 
	XrotationDegrees = 0.0f;
	YrotationDegrees = 330.0f;  // Rotateed for angled view
	ZrotationDegrees = 90.0f;
	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-8.61f, -0.02f, -25.0f);
	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(scaleXYZ,XrotationDegrees,YrotationDegrees,ZrotationDegrees,positionXYZ);
	//SetShaderColor(0.6f, 0.4f, 0.2f, 1.0f);  // Brown table; Different color to set position

	SetShaderTexture("fabric02"); // Using fabric02 texture for a side table cloth; I took a photo of my side table cloth then uploaded it to the textures folder but it did not look good
	SetTextureUVScale(1.0f, 1.0f); // This was the best looking scale for the texture
	SetShaderMaterial("fabric03");

	// draw the mesh with transformation values
	m_basicMeshes->DrawPlaneMesh();


	/****************** Kitchen Table Cloth Plane (Right Side) *******************/
	scaleXYZ = glm::vec3(1.0f, 1.0f, 20.0f);
	XrotationDegrees = 0.0f;
	YrotationDegrees = 329.9f;  // Rotateed for angled view
	ZrotationDegrees = 90.0f;
	positionXYZ = glm::vec3(8.69f, 0.0f, -15.03f);
	//SetShaderColor(0.6f, 0.4f, 0.2f, 1.0f);  // Brown table; Different color to set position
	SetTransformations(scaleXYZ,XrotationDegrees,YrotationDegrees,ZrotationDegrees,positionXYZ);

	SetShaderTexture("fabric02"); // Using fabric02 texture for a side table cloth; I took a photo of my side table cloth then uploaded it to the textures folder but it did not look good
	SetTextureUVScale(1.0f, 1.0f); // This was the best looking scale for the texture
	SetShaderMaterial("fabric03");

	// draw the mesh with transformation values
	m_basicMeshes->DrawPlaneMesh();

	/****************** Table Leg 1 (Front Left) *******************/
	scaleXYZ = glm::vec3(0.5f, 8.0f, 0.5f); 
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;
	positionXYZ = glm::vec3(-16.0f, -7.01f, -8.5f); // front left corner
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	//SetShaderColor(0.6f, 0.4f, 0.2f, 1.0f);  // Same brown as table
	
	SetShaderTexture("wood"); // Using wood texture for table legs
	SetTextureUVScale(1.0f, 1.0f); 
	SetShaderMaterial("wood1");
	
	m_basicMeshes->DrawCylinderMesh();

	/****************** Table Leg 2 (Front Right) *******************/
	scaleXYZ = glm::vec3(0.5f, 8.0f, 0.5f); 
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;
	positionXYZ = glm::vec3(-1.84f, -7.01f, 0.85f); // front right corner
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	//SetShaderColor(0.6f, 0.4f, 0.2f, 1.0f);  // Same brown as table
	SetShaderTexture("wood"); // Using wood texture for table legs
	SetTextureUVScale(1.0f, 1.0f);
	SetShaderMaterial("wood1");
	
	m_basicMeshes->DrawCylinderMesh();

	/****************** Table Leg 3 (Back Right) *******************/
	scaleXYZ = glm::vec3(0.5f, 8.0f, 0.5f); 
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;
	positionXYZ = glm::vec3(16.3f, -7.01f, -31.0f); // back right corner
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	//SetShaderColor(0.6f, 0.4f, 0.2f, 1.0f);  // Same brown as table
	SetShaderTexture("wood"); // Using wood texture for table legs
	SetTextureUVScale(1.0f, 1.0f);
	SetShaderMaterial("wood1");
	
	m_basicMeshes->DrawCylinderMesh();

	/****************** Table Leg 4 (Back Left) ********************/
	scaleXYZ = glm::vec3(0.5f, 8.0f, 0.5f);
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;
	positionXYZ = glm::vec3(1.5f, -7.01f, -40.0f); // back left corner
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	//SetShaderColor(0.6f, 0.4f, 0.2f, 1.0f);  // Same brown as table
	SetShaderTexture("wood"); // Using wood texture for table legs
	SetTextureUVScale(1.0f, 1.0f);
	SetShaderMaterial("wood1");
	
	m_basicMeshes->DrawCylinderMesh();

	/****************** Chair Seat *********************************/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(6.4f, 0.3f, 5.5f);
	// set the XYZ rotation for the mesh 
	XrotationDegrees = 0.0f;
	YrotationDegrees = 30.0f;  // Rotateed for angled view
	ZrotationDegrees = 0.0f;
	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-9.98f, -2.010f, 2.73f);
	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	//SetShaderColor(0.6f, 0.4f, 0.2f, 1.0f);  // Brown table

	SetShaderTexture("wood"); //Same material as table
	SetTextureUVScale(1.0f, 1.0f); 
	SetShaderMaterial("wood1");

	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();

	/****************** Chair Seat Cushion ****************************/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(3.0f, 0.4f, 2.6f);
	// set the XYZ rotation for the mesh 
	XrotationDegrees = 0.0f;
	YrotationDegrees = 30.0f;  // Rotateed for angled view
	ZrotationDegrees = 0.0f;
	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-9.98f, -1.86f, 2.73f);
	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	//SetShaderColor(0.6f, 0.4f, 0.2f, 1.0f);  // Brown table

	SetShaderTexture("fabric02"); //Same material as tablecloth
	SetTextureUVScale(1.0f, 100.0f); // The texture is repeated to give it an artistic look and keeping the number of texture down 
	SetShaderMaterial("fabric03");

	// draw the mesh with transformation values
	m_basicMeshes->DrawHalfSphereMesh();

	/****************** Chair Back *********************************/ 
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(6.5f, 0.3f, 5.5f);
	// set the XYZ rotation for the mesh 
	XrotationDegrees = 0.0f;
	YrotationDegrees = 30.0f;  // Rotateed for angled view
	ZrotationDegrees = 90.0f;
	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-12.7f, 1.092f, 4.3f);
	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	//SetShaderColor(0.6f, 0.4f, 0.2f, 1.0f);  // Brown table

	SetShaderTexture("wood");// Same material as table
	SetTextureUVScale(1.0f, 1.0f); 
	SetShaderMaterial("wood1");

	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();

	//******************** Chair Leg 1 (Back Left) *******************/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.25f, 5.0f, 0.25f);
	// set the XYZ rotation for the mesh 
	XrotationDegrees = 0.0f;
	YrotationDegrees = 30.0f;  // Rotateed for angled view
	ZrotationDegrees = 0.0f;
	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-13.5f, -7.092f, 2.38f);
	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	//SetShaderColor(0.6f, 0.4f, 0.2f, 1.0f);  // Brown table

	SetShaderTexture("wood"); // Same material as table
	SetTextureUVScale(1.0f, 1.0f); 
	SetShaderMaterial("wood1");

	// draw the mesh with transformation values
	m_basicMeshes->DrawCylinderMesh();


	//******************** Chair Leg 2 (Front Right) *******************/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.25f, 5.0f, 0.25f);
	// set the XYZ rotation for the mesh 
	XrotationDegrees = 0.0f;
	YrotationDegrees = 30.0f;  // Rotateed for angled view
	ZrotationDegrees = 0.0f;
	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-6.6f, -7.092f, 3.5f);
	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	//SetShaderColor(0.6f, 0.4f, 0.2f, 1.0f);  // Brown table

	SetShaderTexture("wood"); // Same material as table
	SetTextureUVScale(1.0f, 1.0f); 
	SetShaderMaterial("wood1");

	// draw the mesh with transformation values
	m_basicMeshes->DrawCylinderMesh();


	//******************** Chair Leg 3 (Front Left) *******************/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.25f, 5.0f, 0.25f);
	// set the XYZ rotation for the mesh 
	XrotationDegrees = 0.0f;
	YrotationDegrees = 30.0f;  // Rotateed for angled view
	ZrotationDegrees = 0.0f;
	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-8.7f, -7.092f, -0.47f);
	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	//SetShaderColor(0.6f, 0.4f, 0.2f, 1.0f);  // Brown table

	SetShaderTexture("wood"); // Same material as table
	SetTextureUVScale(1.0f, 1.0f); 
	SetShaderMaterial("wood1");

	// draw the mesh with transformation values
	m_basicMeshes->DrawCylinderMesh();


	/******************** Chair Leg 4 (Back Right) *******************/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.25f, 5.0f, 0.25f);
	// set the XYZ rotation for the mesh 
	XrotationDegrees = 0.0f;
	YrotationDegrees = 30.0f;  // Rotateed for angled view
	ZrotationDegrees = 0.0f;
	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-11.2f, -7.092f, 6.1f);
	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	//SetShaderColor(0.6f, 0.4f, 0.2f, 1.0f);  // Brown table

	SetShaderTexture("wood"); // Same material as table
	SetTextureUVScale(1.0f, 1.0f); 
	SetShaderMaterial("wood1");

	// draw the mesh with transformation values
	m_basicMeshes->DrawCylinderMesh();


	/****************** Laptop Base *********************************/
	scaleXYZ = glm::vec3(8.0f, 0.2f, 4.0f); // flat and wide
	XrotationDegrees = 0.0f; //adjusted to match the photo
	YrotationDegrees = -0.78f; 
	ZrotationDegrees = 0.0f;
	positionXYZ = glm::vec3(-13.05f, 1.13f, -9.0f); // slightly above table
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	//SetShaderColor(0.2f, 0.2f, 0.2f, 1.0f);  // Dark grey for laptop base
	
	SetShaderTexture("Onyx1"); // Using Onyx1 texture for laptop base; It was the closest texture I could find to match the photo
	SetTextureUVScale(5.0f, 5.0f); // I up the scale to 5.0f to make the texture look more like the photo
	SetShaderMaterial("Onyx2");

	m_basicMeshes->DrawBoxMesh();

	/****************** Laptop Base (Keyboard) **********************/
	scaleXYZ = glm::vec3(6.0f, 0.2f, 2.75f); // flat and wide
	XrotationDegrees = 0.0f; //adjusted to match the photo
	YrotationDegrees = -0.78f;
	ZrotationDegrees = 0.0f;
	positionXYZ = glm::vec3(-13.05f, 1.2f, -9.0f); // slightly above table
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	//SetShaderColor(0.2f, 0.2f, 0.2f, 1.0f);  // Dark grey for laptop base

	SetShaderTexture("keyboard"); // Texture for the laptop keyboard
	SetTextureUVScale(1.0f, 1.0f);
	SetShaderMaterial("keyboard1");

	m_basicMeshes->DrawBoxMesh();

	/****************** Laptop Top ********************************/
	scaleXYZ = glm::vec3(8.0f, 0.1f, 4.0f); // thin
	XrotationDegrees = 81.46f; //adjusted to match the photo
	YrotationDegrees = 0.0f; 
	ZrotationDegrees = 0.0; 
	positionXYZ = glm::vec3(-13.05f, 3.0f, -11.3f); // lifted behind the base
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	//SetShaderColor(0.1f, 0.1f, 0.1f, 1.0f);  // Even darker grey
	SetShaderTexture("laptop"); // Using laptop texture for top; It was the closest texture I could find to match the photo
	SetTextureUVScale(1.0f, 1.0f);
	SetShaderMaterial("laptop1");
	
	m_basicMeshes->DrawBoxMesh();

	/****************** Laptop Screen *****************************/
	scaleXYZ = glm::vec3(7.0f, 0.01f, 3.0f); // thin, like a screen
	XrotationDegrees = 81.5f; //adjusted to match the photo
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;
	positionXYZ = glm::vec3(-13.03f, 2.96f, -11.2f); // lifted behind the base and in front of Laptop Top
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	//SetShaderColor(0.1f, 0.1f, 1.0f, 1.0f);  // Blue for screen
	SetShaderTexture("matrix"); // I used the matrix texture for the screen because it is how I feel when I am coding
	SetTextureUVScale(1.0f, 1.0f);
	//SetShaderMaterial("matrix1"); // I did not use a material for the screen because the texture alone looks better.
	
	m_basicMeshes->DrawBoxMesh();

	/****************** Mousepad ***********************************/
	scaleXYZ = glm::vec3(3.8f, 0.03f, 3.8f); // flat and wide
	XrotationDegrees = 0.0f; //adjusted to match the photo
	YrotationDegrees = 320.0f;
	ZrotationDegrees = 0.0f;
	positionXYZ = glm::vec3(-4.05f, 1.1f, -4.75f); // slightly above table
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	//SetShaderColor(0.2f, 0.2f, 0.2f, 1.0f);  // Dark grey for positioning 

	SetShaderTexture("mousepad"); // I tried to use a photo of my mousepad but it did not look good so I used the mousepad texture
	SetTextureUVScale(1.0f, 1.0f);
	SetShaderMaterial("mousepad1");

	m_basicMeshes->DrawBoxMesh();

	/****************** Mouse ************************************/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.35f, 0.17f, 0.55f);
	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 140.0f;
	ZrotationDegrees = 0.0f;
	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-4.05f, 1.18f, -4.55f);
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	//SetShaderColor(0.2f, 0.2f, 0.2f, 1.0f);
	SetShaderTexture("mouse1"); 
	SetTextureUVScale(1.0f, 0.6f);
	SetShaderMaterial("mouse2");

	m_basicMeshes->DrawSphereMesh();

	/****************** Water Bottle Base ************************/
	scaleXYZ = glm::vec3(0.6f, 2.75f, 0.6f);
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;
	positionXYZ = glm::vec3(-1.0f, 1.05f, -9.75f); // front left corner
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	//SetShaderColor(0.6f, 0.4f, 0.2f, 1.0f);  //Different color to set position
	SetShaderTexture("black_metal"); // Using black metal texture for water bottle base
	SetTextureUVScale(1.0f, 1.0f);
	SetShaderMaterial("black_metal1");

	m_basicMeshes->DrawCylinderMesh();

	/****************** Water Bottle Steel Ring ***************************/
	scaleXYZ = glm::vec3(0.46f, 0.05f, 0.46f);
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;
	positionXYZ = glm::vec3(-1.0f, 3.80f, -9.75f); // front left corner
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	//SetShaderColor(0.6f, 0.4f, 0.2f, 1.0f);  // Different color to set position

	SetShaderTexture("stainless"); // Using black metal texture for water bottle cap
	SetTextureUVScale(1.0f, 1.0f);
	SetShaderMaterial("stainless1");

	m_basicMeshes->DrawCylinderMesh();

	/****************** Water Bottle Cap ***************************/
	scaleXYZ = glm::vec3(0.45f, 0.3f, 0.45f);
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;
	positionXYZ = glm::vec3(-1.0f, 3.80f, -9.75f); // front left corner
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	//SetShaderColor(0.6f, 0.4f, 0.2f, 1.0f);  // Different color to set position

	SetShaderTexture("black_metal"); // Using black metal texture for water bottle cap
	SetTextureUVScale(1.0f, 1.0f);
	SetShaderMaterial("black_metal1");

	m_basicMeshes->DrawCylinderMesh();

	/****************** Water Bottle Mouthpiece *******************/
	scaleXYZ = glm::vec3(0.06f, 0.45f, 0.03f);
	XrotationDegrees = 10.0f;
	YrotationDegrees = -40.0f;
	ZrotationDegrees = 15.0f;
	positionXYZ = glm::vec3(-1.3f, 3.93f, -9.7f); // front left corner
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	//SetShaderColor(0.6f, 0.4f, 0.2f, 1.0f);  // Different color to set position

	SetShaderTexture("laptop"); // Using laptop texture for mouthpiece because they are similar
	SetTextureUVScale(1.0f, 1.0f);
	SetShaderMaterial("laptop1");


	m_basicMeshes->DrawCylinderMesh();

	/****************** Wax candle 1 *******************/
	scaleXYZ = glm::vec3(0.1f, 3.5f, 0.1f);
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;
	positionXYZ = glm::vec3(-2.0f, 1.18, -20.0f); // Candle closer to the Laptop
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	//SetShaderColor(0.6f, 0.4f, 0.2f, 1.0f);  //Different color to set position
	SetShaderTexture("wax"); // Using wax texture for candle
	SetTextureUVScale(1.0f, 1.0f);
	SetShaderMaterial("wax1");

	m_basicMeshes->DrawCylinderMesh();

	/****************** Glass Candle Holder Base 1 *******************/
	scaleXYZ = glm::vec3(0.5f, 0.5f, 0.5f);
	XrotationDegrees = 0.0f;
	YrotationDegrees = 95.0f;
	ZrotationDegrees = 0.0f;
	positionXYZ = glm::vec3(-2.0f, 1.05, -20.0f); 
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	//SetShaderColor(1.0f, 1.0f, 1.0f, 1.0f);  //Different color to set position
	SetShaderTexture("glass"); // Using glass texture for candle holder base
	SetTextureUVScale(1.0f, 1.0f);
	SetShaderMaterial("glass1");

	m_basicMeshes->DrawConeMesh();

	/****************** Glass Candle Holder Stem 1 *******************/
	scaleXYZ = glm::vec3(0.13f, 0.4f, 0.13f);
	XrotationDegrees = 0.0f;
	YrotationDegrees = 95.0f;
	ZrotationDegrees = 0.0f;
	positionXYZ = glm::vec3(-2.0f, 1.2, -20.0f); 
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	//SetShaderColor(1.0f, 1.0f, 1.0f, 1.0f);  //Different color to set position
	SetShaderTexture("glass"); // Using glass texture for candle holder base
	SetTextureUVScale(1.0f, 1.0f);
	SetShaderMaterial("glass1");

	m_basicMeshes->DrawCylinderMesh();


	/****************** Wax candle 2 *******************/
	scaleXYZ = glm::vec3(0.1f, 3.5f, 0.1f);
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;
	positionXYZ = glm::vec3(4.4f, 1.18, -30.0f); // Candle 2 positioned at the far side of the table
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	//SetShaderColor(0.6f, 0.4f, 0.2f, 1.0f);  //Different color to set position
	SetShaderTexture("wax"); // Using wax texture for candle
	SetTextureUVScale(1.0f, 1.0f);
	SetShaderMaterial("wax1");

	m_basicMeshes->DrawCylinderMesh();

	/****************** Glass Candle Holder Base 2 *******************/
	scaleXYZ = glm::vec3(0.5f, 0.5f, 0.5f);
	XrotationDegrees = 0.0f;
	YrotationDegrees = 95.0f;
	ZrotationDegrees = 0.0f;
	positionXYZ = glm::vec3(4.4f, 1.05, -30.0f); 
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	//SetShaderColor(1.0f, 1.0f, 1.0f, 1.0f);  //Different color to set position
	SetShaderTexture("glass"); // Using glass texture for candle holder base
	SetTextureUVScale(1.0f, 1.0f);
	SetShaderMaterial("glass1");

	m_basicMeshes->DrawConeMesh();

	/****************** Glass Candle Holder Stem 2 *******************/
	scaleXYZ = glm::vec3(0.13f, 0.4f, 0.13f);
	XrotationDegrees = 0.0f;
	YrotationDegrees = 95.0f;
	ZrotationDegrees = 0.0f;
	positionXYZ = glm::vec3(4.4f, 1.2, -30.0f); 
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	//SetShaderColor(1.0f, 1.0f, 1.0f, 1.0f);  //Different color to set position
	SetShaderTexture("glass"); // Using glass texture for candle holder base
	SetTextureUVScale(1.0f, 1.0f);
	SetShaderMaterial("glass1");

	m_basicMeshes->DrawCylinderMesh();

	/****************** Flame - Wax candle 1 *******************/
	scaleXYZ = glm::vec3(0.1f, 0.37f, 0.1f);
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;
	positionXYZ = glm::vec3(-2.0f, 5.0f, -20.0f); // On top of the first candle
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	//SetShaderColor(0.6f, 0.4f, 0.2f, 1.0f);  //Different color to set position
	SetShaderTexture("flame"); // Using flame texture for candle flame
	SetTextureUVScale(3.0f, 3.0f);
	SetShaderMaterial("flame1");

	m_basicMeshes->DrawSphereMesh();

	/****************** Flame - Wax candle 2 *******************/
	scaleXYZ = glm::vec3(0.1f, 0.37f, 0.1f);
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;
	positionXYZ = glm::vec3(4.4f, 5.0f, -30.0f); // On top of the second candle
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	//SetShaderColor(0.6f, 0.4f, 0.2f, 1.0f);  //Different color to set position
	SetShaderTexture("flame"); // Using flame texture for candle flame
	SetTextureUVScale(3.0f, 3.0f);
	SetShaderMaterial("flame1");

	m_basicMeshes->DrawSphereMesh();

	/****************** Salt base 1 *******************/
	scaleXYZ = glm::vec3(0.4f, 0.2f, 0.4f);
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;
	positionXYZ = glm::vec3(0.0f, 1.03, -23.0f); // Positioned in the middle of the table between the candles
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	//SetShaderColor(1.0f, 0.1f, 0.1f, 1.0f);  //Different color to set position
	SetShaderTexture("salt1"); // Using salt texture 
	SetTextureUVScale(1.0f, 0.5f);
	SetShaderMaterial("salt2");

	m_basicMeshes->DrawCylinderMesh();

	/****************** Salt base 2 *******************/
	scaleXYZ = glm::vec3(0.45f, 0.36f, 0.45f);
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;
	positionXYZ = glm::vec3(0.0f, 1.37, -23.0f); // Positioned in the middle of the table between the candles slightly above the first base
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	//SetShaderColor(1.0f, 1.0f, 1.0f, 1.0f);  //Different color to set position
	SetShaderTexture("salt1"); // Using salt texture
	SetTextureUVScale(1.0f, 3.0f);
	SetShaderMaterial("salt2");

	m_basicMeshes->DrawSphereMesh();

	/****************** Salt base 3 *******************/
	scaleXYZ = glm::vec3(0.4f, 0.2f, 0.4f);
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;
	positionXYZ = glm::vec3(0.0f, 1.46, -23.0f); // Positioned in the middle of the table between the candles sightly above the second base
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	//SetShaderColor(1.0f, 1.0f, 1.0f, 1.0f);  //Different color to set position
	SetShaderTexture("salt1"); // Using salt texture
	SetTextureUVScale(1.0f, 0.5f);
	SetShaderMaterial("salt2");

	m_basicMeshes->DrawCylinderMesh();

	/****************** Salt Cap *******************/
	scaleXYZ = glm::vec3(0.32f, 0.2f, 0.32f);
	XrotationDegrees = 360.0f;
	YrotationDegrees = 90.0f;
	ZrotationDegrees = 0.0f;
	positionXYZ = glm::vec3(0.0f, 1.67, -23.0f); // Positioned in the middle of the table between the candles on top of the third base
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	//SetShaderColor(1.0f, 1.0f, 1.0f, 1.0f);  //Different color to set position
	SetShaderTexture("cap2"); // Using cap2 texture for salt cap (The texture comes from a photo of a salt shaker cover)
	SetTextureUVScale(1.0, 3.5f);
	SetShaderMaterial("cap3");

	m_basicMeshes->DrawHalfSphereMesh();

	/****************** Pepper base 1 *******************/
	scaleXYZ = glm::vec3(0.4f, 0.2f, 0.4f);
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;
	positionXYZ = glm::vec3(2.4f, 1.03, -26.5f); // Positioned closer to candle 2 
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	//SetShaderColor(0.0f, 0.0f, 0.0f, 1.0f);  //Different color to set position
	SetShaderTexture("pepper"); // Using pepper texture for pepper base
	SetTextureUVScale(1.0f, 0.5f);
	SetShaderMaterial("pepper1");

	m_basicMeshes->DrawCylinderMesh();

	/****************** Pepper base 2 *******************/
	scaleXYZ = glm::vec3(0.45f, 0.36f, 0.45f);
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;
	positionXYZ = glm::vec3(2.4f, 1.37, -26.5f); // Positioned closer to candle 2 slightly above the first base
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	//SetShaderColor(0.0f, 0.0f, 0.0f, 1.0f);  //Different color to set position
	SetShaderTexture("pepper"); // Using pepper texture for pepper base
	SetTextureUVScale(1.0f, 3.0f);
	SetShaderMaterial("pepper1");

	m_basicMeshes->DrawSphereMesh();

	/****************** Pepper base 3 *******************/
	scaleXYZ = glm::vec3(0.4f, 0.2f, 0.4f);
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;
	positionXYZ = glm::vec3(2.4f, 1.46, -26.5f); // Positioned closer to candle 2 slightly above the second base
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	//SetShaderColor(0.0f, 0.0f, 0.0f, 1.0f);  //Different color to set position
	SetShaderTexture("pepper"); // Using a pepper texture for pepper base
	SetTextureUVScale(1.0f, 0.5f);
	SetShaderMaterial("pepper1");

	m_basicMeshes->DrawCylinderMesh();

	/****************** Peper Cap *******************/
	scaleXYZ = glm::vec3(0.32f, 0.2f, 0.32f);
	XrotationDegrees = 360.0f;
	YrotationDegrees = 90.0f;
	ZrotationDegrees = 0.0f;
	positionXYZ = glm::vec3(2.4f, 1.67, -26.5f); // Positioned closer to candle 2 on top of the third base
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	//SetShaderColor(1.0f, 1.0f, 1.0f, 1.0f);  //Different color to set position
	SetShaderTexture("cap2"); // Using a cap2 texture for pepper cap (The texture comes from a photo of a salt shaker cover)
	SetTextureUVScale(1.0, 3.5f);
	SetShaderMaterial("cap3");

	m_basicMeshes->DrawHalfSphereMesh();


}