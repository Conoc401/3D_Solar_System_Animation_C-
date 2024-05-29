//------------Include the Application Header File----------------------------
#include "LJMULevelDemo.h"

//------------DX TK AND STD/STL Includes-------------------------------------
#include <sstream>

//------------Include Hieroglyph Engine Files--------------------------------

//Include the Logging System
#include "Log.h"

//Include the Event System
#include "EventManager.h"
#include "EvtFrameStart.h"
#include "EvtChar.h"
#include "EvtKeyUp.h"
#include "EvtKeyDown.h"
#include "ScriptManager.h"

//Include the DirectX Rendering Components
#include "PipelineManagerDX11.h"
#include "BlendStateConfigDX11.h"
#include "BufferConfigDX11.h"
#include "DepthStencilStateConfigDX11.h"
#include "RasterizerStateConfigDX11.h"
#include "SwapChainConfigDX11.h"
#include "Texture2dConfigDX11.h"
#include "MaterialGeneratorDX11.h"

#include "FirstPersonCamera.h"

//Add a Using Directive to avoid typing Glyph3 for basic constructs
using namespace Glyph3;
//Include our own application Namespace
using namespace LJMUDX;

#include <FileSystem.h>
#include <SamplerStateConfigDX11.h>

#include "LJMUMeshOBJ.h"
#include "FastNoise.h"

LJMULevelDemo AppInstance;

//---------CONSTRUCTORS-------------------------------------------------------

///////////////////////////////////////
//
///////////////////////////////////////
LJMULevelDemo::LJMULevelDemo() :
	m_pRender_text(nullptr),
	m_pRenderView(nullptr),
	m_pCamera(nullptr),
	m_pRenderer11(nullptr),
	m_pWindow(nullptr),
	m_iSwapChain(0),
	m_DepthTarget(nullptr),
	m_RenderTarget(nullptr)
{

}

//---------METHODS------------------------------------------------------------

void LJMULevelDemo::setupCamera()
{

	m_pCamera = new FirstPersonCamera();
	m_pCamera->SetEventManager(&EvtManager);

	Vector3f tcamerapos(0.0f, 500.0f, -2560.0f);
	m_pCamera->Spatial().SetTranslation(tcamerapos);
	m_pCamera->Spatial().RotateXBy(30 * DEG_TO_RAD);

	m_pRenderView = new ViewPerspective(*m_pRenderer11,
		m_RenderTarget,
		m_DepthTarget);
	m_pRenderView->SetBackColor(Vector4f(0.0f, 0.0f, 0.0f, 1.0f));
	m_pCamera->SetCameraView(m_pRenderView);

	m_pRender_text = new LJMUTextOverlay(*m_pRenderer11,
		m_RenderTarget,
		std::wstring(L"Cambria"),
		25);
	m_pCamera->SetOverlayView(m_pRender_text);

	m_pCamera->SetProjectionParams(0.1f,
		100000.0f,
		m_iscreenWidth / m_iscreenHeight,
		static_cast<float>(GLYPH_PI) / 2.0f);

	m_pScene->AddCamera(m_pCamera);
}

MaterialPtr LJMULevelDemo::createWireframePlaneMaterial()
{
	MaterialPtr material = MaterialPtr(new MaterialDX11());
	RenderEffectDX11* pEffect = new RenderEffectDX11();

	// -- Setup shader here
	pEffect->SetVertexShader(m_pRenderer11->LoadShader(VERTEX_SHADER,
		std::wstring(L"Basic.hlsl"),
		std::wstring(L"VSMain"),
		std::wstring(L"vs_4_0"),
		true));

	pEffect->SetPixelShader(m_pRenderer11->LoadShader(PIXEL_SHADER,
		std::wstring(L"Basic.hlsl"),
		std::wstring(L"PSMain"),
		std::wstring(L"ps_4_0"),
		true));

	RasterizerStateConfigDX11 rsConfig;
	rsConfig.CullMode = D3D11_CULL_NONE;
	rsConfig.FillMode = D3D11_FILL_WIREFRAME;

	int iRasterizerState = m_pRenderer11->CreateRasterizerState(&rsConfig);
	if (iRasterizerState == -1) {
		Log::Get().Write(L"Failed to create light rasterizer state");
		assert(false);
	}

	pEffect->m_iRasterizerState = iRasterizerState;

	material->Params[VT_PERSPECTIVE].bRender = true;
	material->Params[VT_PERSPECTIVE].pEffect = pEffect;

	return material;
}

MaterialPtr LJMULevelDemo::CreateBasicTexturedMaterial()
{
	MaterialPtr material = MaterialPtr(new MaterialDX11());

	// Create and fill the effect that will be used for this view type
	RenderEffectDX11* pEffect = new RenderEffectDX11();

	pEffect->SetVertexShader(m_pRenderer11->LoadShader(VERTEX_SHADER,
		std::wstring(L"BasicTexture.hlsl"),
		std::wstring(L"VSMain"),
		std::wstring(L"vs_4_0")));

	pEffect->SetPixelShader(m_pRenderer11->LoadShader(PIXEL_SHADER,
		std::wstring(L"BasicTexture.hlsl"),
		std::wstring(L"PSMain"),
		std::wstring(L"ps_4_0")));

	RasterizerStateConfigDX11 rsConfig;
	rsConfig.CullMode = D3D11_CULL_NONE;
	rsConfig.FillMode = D3D11_FILL_SOLID;

	int iRasterizerState = m_pRenderer11->CreateRasterizerState(&rsConfig);
	if (iRasterizerState == -1) {
		Log::Get().Write(L"Failed to create light rasterizer state");
		assert(false);
	}

	pEffect->m_iRasterizerState = iRasterizerState;

	SamplerStateConfigDX11 SamplerConfig;
	SamplerConfig.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
	SamplerConfig.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
	SamplerConfig.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
	SamplerConfig.MaxAnisotropy = 1;

	int TextureSampler = RendererDX11::Get()->CreateSamplerState(&SamplerConfig);
	material->Parameters.SetSamplerParameter(L"TextureSampler", TextureSampler);

	// Enable the material to render the given view type, and set its effect.
	material->Params[VT_PERSPECTIVE].bRender = true;
	material->Params[VT_PERSPECTIVE].pEffect = pEffect;

	return material;
}

void LJMULevelDemo::SetTextureToBasicMaterial(MaterialPtr material, ResourcePtr texture)
{
	material->Parameters.SetShaderResourceParameter(L"DiffuseTexture", texture);
}

void LJMULevelDemo::SetTexturesToBumpLitMaterial(MaterialPtr material,
	ResourcePtr diffusetexture,
	ResourcePtr bumptexture)
{
	material->Parameters.SetShaderResourceParameter(L"DiffuseTexture", diffusetexture);
	material->Parameters.SetShaderResourceParameter(L"BumpTexture", bumptexture);
}

void LJMULevelDemo::SetTexturesToBumpLitMaterial(MaterialPtr material,
	ResourcePtr diffusetexture,
	ResourcePtr bumptexture,
	ResourcePtr lighttexture)
{
	material->Parameters.SetShaderResourceParameter(L"DiffuseTexture", diffusetexture);
	material->Parameters.SetShaderResourceParameter(L"BumpTexture", bumptexture);
	material->Parameters.SetShaderResourceParameter(L"LightTexture", lighttexture);
}

CustomMeshPtr	LJMULevelDemo::CreatePlaneCustomMesh(const Vector3f& center,
	const Vector3f& xdir,
	const Vector3f& zdir,
	const Vector2f& numVertices,
	const Vector2f& spacing,
	const Vector4f& colour,
	const Vector2f& gridPos,
	const Vector2f& texScale)
{
	std::vector<Vector3f> vertices;
	std::vector<Vector4f> vertexColors;
	std::vector<Vector2f> texCoord;

	std::vector<Vector2f> texWeights;

	Vector3f x_unit = xdir;
	Vector3f z_unit = zdir;

	x_unit.Normalize();
	z_unit.Normalize();

	int xVertices = (int)numVertices.x;
	int zVertices = (int)numVertices.y;

	float xSpacing = spacing.x;
	float zSpacing = spacing.y;

	Vector3f x = x_unit * ((float)xVertices - 1) / 2 * xSpacing;
	Vector3f z = z_unit * ((float)xVertices - 1) / 2 * zSpacing;

	// Get the location of the top-left vertex
	Vector3f startPos = center - x - z;

	// Calculate the unit up vector (y_unit).
	// Direct3D uses left-hand rule to determine the direction (since z axis direction is reversed)
	// Hence z cross x gives the y up vector
	Vector3f y_unit = z_unit.Cross(x_unit);
	y_unit.Normalize();

	float heightScale = 10.0f;

	float majorheightfrequency = 0.02f;
	float majorheight = 1.0f;

	float minorheightfrequency = 0.2;
	float minorheight = 0.25f;

	for (int i = 0; i < xVertices; i++)
	{
		for (int j = 0; j < zVertices; j++)
		{
			//float height = cos((float)i * 0.1f)* sin((float)j * 0.1f)* heightScale;

			//Vector3f pos = startPos + x_unit * xSpacing * i + z_unit * zSpacing * j;

			float shifted_i = (float)i + gridPos.x * (xVertices - 1);
			float shifted_j = (float)j + gridPos.y * (zVertices - 1);

			//Vector3f pos = startPos + x_unit * xSpacing * shifted_i + z_unit * zSpacing * shifted_j;

			float majorperiodicheight_x = sin(shifted_i * majorheightfrequency * GLYPH_PI) * majorheight;
			float majorperiodicheight_z = cos(shifted_j * majorheightfrequency * GLYPH_PI) * majorheight;
			float majorperiodicheight = majorperiodicheight_x * majorperiodicheight_z;

			float minorperiodicheight_x = sin(shifted_i * minorheightfrequency * GLYPH_PI) * minorheight;
			float minorperiodicheight_z = cos(shifted_j * minorheightfrequency * GLYPH_PI) * minorheight;
			float minorperiodicheight = minorperiodicheight_x * minorperiodicheight_z;

			float height = (majorperiodicheight + minorperiodicheight) * heightScale;

			Vector3f pos = startPos +
				x_unit * xSpacing * shifted_i +
				z_unit * zSpacing * shifted_j +
				y_unit * height * heightScale;



			vertices.push_back(pos);

			//vertexColors.push_back(colour);

			float shade = (height / heightScale + 1) / 2.0f;

			if (shade < 0)
				shade = 0;
			else if (shade > 1)
				shade = 1;

			vertexColors.push_back(Vector4f(shade, 1 - shade, shade / 2, 1));

			Vector2f normalisedTexScale = Vector2f(texScale.x / numVertices.x,
				texScale.y / numVertices.y);

			float u = shifted_i;
			float v = shifted_j;
			texCoord.push_back(Vector2f(u, v) * normalisedTexScale);

			float nheight = majorperiodicheight + minorperiodicheight;
			float abruptness = 5.0f;
			float sigmoid = 1 / (1 + exp(-nheight * abruptness));

			texWeights.push_back(Vector2f(nheight, 0.0f));
		}
	}

	int numberofVertices = vertices.size();

	std::vector<int> indices;

	GeneratePlaneIndexArray(xVertices, zVertices, true, indices);

	int numberofIndices = indices.size();
	int numberofTriangles = numberofIndices / 3;

	//auto terrainMesh = std::make_shared<DrawExecutorDX11<CustomVertexDX11::Vertex>>();
	//terrainMesh->SetLayoutElements(CustomVertexDX11::GetElementCount(),
	//	CustomVertexDX11::Elements);

	auto terrainMesh = std::make_shared<DrawExecutorDX11<CustomVertexDX11::Vertex>>();
	terrainMesh->SetLayoutElements(CustomVertexDX11::GetElementCount(),
		CustomVertexDX11::Elements);

	terrainMesh->SetPrimitiveType(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	terrainMesh->SetMaxVertexCount(numberofIndices);

	CustomVertexDX11::Vertex tv;

	for (int i = 0; i < numberofIndices; i++)
	{
		tv.position = vertices[indices[i]];
		tv.color = vertexColors[indices[i]];
		tv.texcoords = texCoord[indices[i]];

		tv.texweights = texWeights[indices[i]];

		terrainMesh->AddVertex(tv);
	}

	return terrainMesh;
}

MeshPtr LJMULevelDemo::CreateCylinderMesh(int h_res,
	int v_res,
	Vector4f colour)
{
	std::vector<Vector3f> vertices;
	std::vector<Vector4f> vertexColors;
	std::vector<Vector2f> texCoord;

	// Cartesian coordinates (x,y,z) variables
	float x;
	float y;
	float z;

	// Nested loops that calculate the vertex position and projector function

	// v_res is the number of vertical segments
	for (int j = 0; j <= v_res; ++j)
	{
		// height goes from -0.5 to +0.5
		float height = float(j) / float(v_res) - 0.5f;

		// h_res is the number of angle increments
		for (int i = 0; i <= h_res; ++i)
		{
			// azimuthAngle goes from 0 to 2 Pi
			float azimuthAngle = 2.0 * GLYPH_PI * float(i) / float(h_res);
			float sa = std::sin(azimuthAngle);
			float ca = std::cos(azimuthAngle);

			x = ca;
			y = height;
			z = sa;

			// u is the horizontal texture coordinate (range from 0 to 1)
			float u = float(i) / float(h_res);
			// v is the vertical texture coordinate (range from 0 to 1)
			float v = float(j) / float(v_res);

			vertices.push_back(Vector3f(x, y, z));
			vertexColors.push_back(colour);
			texCoord.push_back(Vector2f(u, v));
		}
	}

	// Setup index array
	std::vector<int> indices;

	GeneratePlaneIndexArray(h_res + 1, v_res + 1, true, indices);

	int numberofIndices = indices.size();
	int numberofTriangles = numberofIndices / 3;

	auto mesh = std::make_shared<DrawExecutorDX11<BasicVertexDX11::Vertex>>();
	mesh->SetLayoutElements(BasicVertexDX11::GetElementCount(), BasicVertexDX11::Elements);
	mesh->SetPrimitiveType(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	mesh->SetMaxVertexCount(numberofIndices);

	BasicVertexDX11::Vertex tv;

	for (int i = 0; i < numberofIndices; i++)
	{
		tv.position = vertices[indices[i]];
		tv.color = vertexColors[indices[i]];
		tv.texcoords = texCoord[indices[i]];

		mesh->AddVertex(tv);
	}

	return mesh;
}


//////////////////////////////////////
// Get the Window Name of this Application
//////////////////////////////////////
std::wstring LJMULevelDemo::GetName()
{
	return(std::wstring(L"6107COMP: Connor Connolly 801641 "));
}

/////////////////////////////////////
// Assemble our Input Layouts for this
// Stage of the Pipeline.
/////////////////////////////////////
void LJMULevelDemo::inputAssemblyStage()
{
	setupPlane();
	SetupSkySphere();
	SetupSphere();
	SetupMars();
	SetupSun();
	SetupMoon();
	SetupHeightMap();

	//SetupCylinder();
}

void LJMULevelDemo::SetupCylinder()
{
	Vector3f vScale = Vector3f(400.0f, 400.0f, 400.0f);
	Matrix3f mRotation;
	mRotation.MakeIdentity();
	Vector3f vTranslation = Vector3f(-2560.0f, 0.0f, -1700.0f);

	unsigned int h_res = 50;
	unsigned int v_res = 50;

	auto cylinderMesh = CreateCylinderMesh(h_res, v_res, Vector4f(1, 0, 0, 1));

	m_cylinderMaterial = CreateBasicTexturedMaterial();
	SetTextureToBasicMaterial(m_cylinderMaterial, m_cylinderTexture);

	m_pCylinderActor = new Actor();
	m_pCylinderActor->GetBody()->SetGeometry(cylinderMesh);
	m_pCylinderActor->GetBody()->SetMaterial(m_cylinderMaterial);
	m_pCylinderActor->GetNode()->Scale() = vScale;
	m_pCylinderActor->GetNode()->Rotation() = mRotation;
	m_pCylinderActor->GetNode()->Position() = vTranslation;

	m_pScene->AddActor(m_pCylinderActor);
}

void LJMULevelDemo::SetupSphere()
{
	Vector3f vScale = Vector3f(1000.0f, 1000.0f, 1000.0f);
	Matrix3f mRotation;
	mRotation.RotationX(-GLYPH_PI);
	Vector3f vTranslation = Vector3f(0.0f, 1500.0f, 5000.0f);

	unsigned int h_res = 50;
	unsigned int v_res = 50;

	auto sphereMesh = CreateCustomStandardSphere(h_res, v_res, Vector4f(1, 0, 0, 1));

	m_sphereMaterial = CreateGSAnimMaterial();
	m_sphereMaterial = createLitTexturedMaterial();
	SetTextureToBasicMaterial(m_sphereMaterial, m_earthTexture);

	setPlanetLightsParameters();

	// Apply light parameters to the shader
	setLights2Material(m_sphereMaterial);

	// Set Material's surface properties
	setMaterialSurfaceProperties(m_sphereMaterial,
		Vector4f(0.01f, 25.0f, 1.0f, 20.0f),
		Vector4f(0.0f, 0.0f, 0.0f, 1.0f));

	m_pSphereActor = new Actor();
	m_pSphereActor->GetBody()->SetGeometry(sphereMesh);
	m_pSphereActor->GetBody()->SetMaterial(m_sphereMaterial);
	m_pSphereActor->GetNode()->Scale() = vScale;
	m_pSphereActor->GetNode()->Rotation() = mRotation;
	m_pSphereActor->GetNode()->Position() = vTranslation;

	m_pScene->AddActor(m_pSphereActor);

	auto cloudMesh = CreateStandardSphere(h_res, v_res, Vector4f(1, 0, 0, 1));
	m_cloudMaterial = createTransparentLitTexturedMaterial();
	SetTextureToBasicMaterial(m_cloudMaterial, m_cloudTexture);

	// Apply light parameters to the shader
	setLights2Material(m_cloudMaterial);

	// Set Material's surface properties
	setMaterialSurfaceProperties(m_cloudMaterial,
		Vector4f(0.01f, 2.0f, 2.0f, 30.0f),
		Vector4f(0.0f, 0.0f, 0.0f, 1.0f));

	m_pCloudActor = new Actor();
	m_pCloudActor->GetBody()->SetGeometry(cloudMesh);
	m_pCloudActor->GetBody()->SetMaterial(m_cloudMaterial);
	m_pCloudActor->GetNode()->Scale() = vScale * 1.02f;;
	m_pCloudActor->GetNode()->Rotation() = mRotation;
	m_pCloudActor->GetNode()->Position() = vTranslation;

	m_pScene->AddActor(m_pCloudActor);
}

CustomMeshPtr LJMULevelDemo::CreateCustomStandardSphere(int h_res,
	int v_res,
	Vector4f colour)
{
	std::vector<Vector3f> vertices;
	std::vector<Vector4f> vertexColors;
	std::vector<Vector2f> texCoord;

	std::vector<Vector3f> normals;

	std::vector<Vector3f> tangent;
	std::vector<Vector3f> binormal;

	// Cartesian coordinates (x,y,z) variables
	float x;
	float y;
	float z;

	for (int j = 0; j <= v_res; ++j)
	{
		float elevationAngle = GLYPH_PI * float(j) / float(v_res);
		float sp = (float)std::sin(elevationAngle);
		float cp = (float)std::cos(elevationAngle);
		for (int i = 0; i <= h_res; ++i)
		{
			float azimuthAngle = 2.0 * GLYPH_PI * float(i) / float(h_res);
			float sa = std::sin(azimuthAngle);
			float ca = std::cos(azimuthAngle);

			x = sp * ca;
			y = cp;
			z = sp * sa;

			float u = float(i) / float(h_res);
			float v = float(j) / float(v_res);

			vertices.push_back(Vector3f(x, y, z));
			vertexColors.push_back(colour);
			texCoord.push_back(Vector2f(u, v));

			normals.push_back(Vector3f(x, y, z));

			tangent.push_back(Vector3f(0, 1, 0));
			binormal.push_back(Vector3f(0, 0, 1));

		}
	}

	// Setup index array
	std::vector<int> indices;

	GeneratePlaneIndexArray(h_res + 1, v_res + 1, true, indices);

	int numberofIndices = indices.size();
	int numberofTriangles = numberofIndices / 3;

	auto mesh = std::make_shared<DrawExecutorDX11<CustomVertexDX11::Vertex>>();
	mesh->SetLayoutElements(CustomVertexDX11::GetElementCount(), CustomVertexDX11::Elements);
	mesh->SetPrimitiveType(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	mesh->SetMaxVertexCount(numberofIndices);

	CustomVertexDX11::Vertex tv;

	for (int i = 0; i < numberofIndices; i++)
	{
		tv.position = vertices[indices[i]];
		tv.color = vertexColors[indices[i]];
		tv.texcoords = texCoord[indices[i]];

		tv.normal = normals[indices[i]];

		tv.tangent = tangent[indices[i]];
		tv.binormal = binormal[indices[i]];

		mesh->AddVertex(tv);
	}

	return mesh;
}

void LJMUDX::LJMULevelDemo::SetupMars()
{
	Vector3f vScale = Vector3f(1000.0f, 1000.0f, 1000.0f);
	Matrix3f mRotation;
	mRotation.MakeIdentity();
	Vector3f vTranslation = Vector3f(0.0f, 1500.0f, -5000.0f);

	unsigned int h_res = 50;
	unsigned int v_res = 50;

	auto sphereMesh = CreateCustomStandardSphere(h_res, v_res, Vector4f(1, 0, 0, 1));

	m_marsMaterial = createLitTexturedMaterial();
	SetTextureToBasicMaterial(m_marsMaterial, m_marsTexture);

	//setPlanetLightsParameters();

	// Apply light parameters to the shader
	setLights2Material(m_marsMaterial);

	// Set Material's surface properties
	setMaterialSurfaceProperties(m_marsMaterial,
		Vector4f(0.0f, 1.0f, 1.0f, 20.0f),			// Amended change the x element
		Vector4f(0.0f, 0.0f, 0.0f, 1.0f));

	m_pMarsActor = new Actor();
	m_pMarsActor->GetBody()->SetGeometry(sphereMesh);
	m_pMarsActor->GetBody()->SetMaterial(m_marsMaterial);
	m_pMarsActor->GetNode()->Scale() = vScale;
	m_pMarsActor->GetNode()->Rotation() = mRotation;
	m_pMarsActor->GetNode()->Position() = vTranslation;

	m_pScene->AddActor(m_pMarsActor);
}

void LJMUDX::LJMULevelDemo::updateMars()
{
	float rotationSpeed = -0.5f;

	Matrix3f rotMatrix;
	rotMatrix.RotationY(m_tpf * rotationSpeed);

	Matrix3f currentRotMatrix = m_pMarsActor->GetBody()->Rotation();
	m_pMarsActor->GetBody()->Rotation() = currentRotMatrix * rotMatrix;
}

void LJMUDX::LJMULevelDemo::SetupSun()
{
	Vector3f vScale = Vector3f(40.0f, 40.0f, 40.0f);
	Matrix3f mRotation;
	mRotation.RotationX(-GLYPH_PI);
	Vector3f vTranslation = Vector3f(5000.0f, 1500.0f, -5000.0f);

	unsigned int h_res = 50;
	unsigned int v_res = 50;

	auto sphereMesh = generateOBJMesh(L"geosphere.obj", Vector4f(1, 1, 1, 1));

	m_sunMaterial = CreateGSAnimv2Material();
	SetTextureToBasicMaterial(m_sunMaterial, m_sunTexture);

	m_pSunActor = new Actor();
	m_pSunActor->GetBody()->SetGeometry(sphereMesh);
	m_pSunActor->GetBody()->SetMaterial(m_sunMaterial);
	m_pSunActor->GetNode()->Scale() = vScale;
	m_pSunActor->GetNode()->Rotation() = mRotation;
	m_pSunActor->GetNode()->Position() = vTranslation;

	m_pScene->AddActor(m_pSunActor);
}

void LJMUDX::LJMULevelDemo::updateSun()
{
	float rotationSpeed = 0.5f;

	Matrix3f rotMatrix;
	rotMatrix.RotationY(m_tpf * rotationSpeed);

	Matrix3f currentRotMatrix = m_pSunActor->GetBody()->Rotation();
	m_pSunActor->GetBody()->Rotation() = currentRotMatrix * rotMatrix;

	Vector4f time = Vector4f(m_tpf, m_totalTime, 0.0f, 0.0f);
	m_pSunActor->GetBody()->GetMaterial()->Parameters.SetVectorParameter(L"time", time);
}

void LJMUDX::LJMULevelDemo::SetupMoon()
{
	Vector3f vScale = Vector3f(750.0f, 750.0f, 750.0f);
	Matrix3f mRotation;
	mRotation.MakeIdentity();
	Vector3f vTranslation = Vector3f(-5000.0f, 1500.0f, 5000.0f);

	unsigned int h_res = 50;
	unsigned int v_res = 50;

	auto sphereMesh = CreateCustomStandardSphere(h_res, v_res, Vector4f(1, 0, 0, 1));

	m_moonMaterial = createLitTexturedMaterial();
	SetTextureToBasicMaterial(m_moonMaterial, m_moonTexture);

	setPlanetLightsParameters();

	// Apply light parameters to the shader
	setLights2Material(m_moonMaterial);

	// Set Material's surface properties
	setMaterialSurfaceProperties(m_moonMaterial,
		Vector4f(0.0f, 1.0f, 1.0f, 20.0f),			// Amended change the x element
		Vector4f(0.0f, 0.0f, 0.0f, 1.0f));

	m_pMoonActor = new Actor();
	m_pMoonActor->GetBody()->SetGeometry(sphereMesh);
	m_pMoonActor->GetBody()->SetMaterial(m_marsMaterial);
	m_pMoonActor->GetNode()->Scale() = vScale;
	m_pMoonActor->GetNode()->Rotation() = mRotation;
	m_pMoonActor->GetNode()->Position() = vTranslation;

	m_pScene->AddActor(m_pMoonActor);
}

void LJMUDX::LJMULevelDemo::updateMoon()
{
	float rotationSpeed = -0.5f;

	Matrix3f rotMatrix;
	rotMatrix.RotationY(m_tpf * rotationSpeed);

	Matrix3f currentRotMatrix = m_pMoonActor->GetBody()->Rotation();
	m_pMoonActor->GetBody()->Rotation() = currentRotMatrix * rotMatrix;
}


void LJMULevelDemo::SetupSkySphere()
{
	Vector3f vScale = Vector3f(70000.0f, 70000.0f, 70000.0f);
	Matrix3f mRotation;
	mRotation.MakeIdentity();
	Vector3f vTranslation = Vector3f(-2560.0f, 0.0f, -1700.0f);

	unsigned int h_res = 50;
	unsigned int v_res = 50;

	auto sphereMesh = CreateStandardSphere(h_res, v_res, Vector4f(1, 0, 0, 1));

	m_skysphereMaterial = CreateAnimatedTexturedMaterial();
	SetTexturesToAnimatedMaterial(m_skysphereMaterial,
								  m_nightskysphereTexture,
								  m_dayskysphereTexture);

	m_pSkySphereActor = new Actor();
	m_pSkySphereActor->GetBody()->SetGeometry(sphereMesh);
	m_pSkySphereActor->GetBody()->SetMaterial(m_skysphereMaterial);
	m_pSkySphereActor->GetNode()->Scale() = vScale;
	m_pSkySphereActor->GetNode()->Rotation() = mRotation;
	m_pSkySphereActor->GetNode()->Position() = vTranslation;

	m_pScene->AddActor(m_pSkySphereActor);
}


////////////////////////////////////
// Initialise our DirectX 3D Scene
////////////////////////////////////
void LJMULevelDemo::Initialize()
{
	LoadTextures();
	inputAssemblyStage();			// Call the Input Assembly Stage to setup the layout of our Engine Objects
	setupCamera();					// Setup the camera

}

///////////////////////////////////
// Update the State of our Game and 
// Output the Results to Screen (Render)
/////////////////////////////////// 
void LJMULevelDemo::Update()
{
	this->m_pTimer->Update();
	EvtManager.ProcessEvent(EvtFrameStartPtr(new EvtFrameStart(this->m_pTimer->Elapsed())));

	m_tpf = m_pTimer->Elapsed();

	if (m_tpf > 10.0f / 60.0f)
	{
		// If this is triggered, most likely the user is debugging the code
		// hence time per frame is much larger than 10x 60fps then

		m_tpf = 1 / 60.0f;
	}

	// Tree Rotation
	m_totalTime += m_tpf;

	//---------- Object Updates -------------------------------------------------------

	updateSphere();
	UpdateSkySphere(m_totalTime);
	updateMars();
	updateSun();
	updateMoon();
	setLights2Material(m_terrainMaterial);
	updatePlanetLight(m_totalTime);
	setLights2Material(m_sphereMaterial);
	setLights2Material(m_cloudMaterial);

	//----------2D Text Rendering-------------------------------------------------------

	float tx = 30.0f;	float ty = 30.0f;
	Matrix4f ttextpos = Matrix4f::Identity();
	ttextpos.SetTranslation(Vector3f(tx, ty, 0.0f));

	static Vector4f twhiteclr(1.0f, 1.0f, 1.0f, 1.0f);
	static Vector4f tyellowclr(1.0f, 1.0f, 0.0f, 1.0f);

	m_pRender_text->writeText(outputFPSInfo(), ttextpos, twhiteclr);

	this->m_pScene->Update(m_pTimer->Elapsed());
	this->m_pScene->Render(this->m_pRenderer11);

	//--------END RENDERING-------------------------------------------------------------
	this->m_pRenderer11->Present(this->m_pWindow->GetHandle(), this->m_pWindow->GetSwapChain());
}

///////////////////////////////////
// Configure the DirectX 11 Programmable
// Pipeline Stages and Create the Window
// Calls 
///////////////////////////////////
bool LJMULevelDemo::ConfigureEngineComponents()
{

	// Set the render window parameters and initialize the window
	this->m_pWindow = new Win32RenderWindow();
	this->m_pWindow->SetPosition(25, 25);
	this->m_pWindow->SetSize(m_iscreenWidth, m_iscreenHeight);
	this->m_pWindow->SetCaption(this->GetName());
	this->m_pWindow->Initialize(this);


	// Create the renderer and initialize it for the desired device
	// type and feature level.
	this->m_pRenderer11 = new RendererDX11();

	if (!this->m_pRenderer11->Initialize(D3D_DRIVER_TYPE_HARDWARE, D3D_FEATURE_LEVEL_11_0))
	{
		Log::Get().Write(L"Could not create hardware device, trying to create the reference device...");

		if (!this->m_pRenderer11->Initialize(D3D_DRIVER_TYPE_REFERENCE, D3D_FEATURE_LEVEL_10_0))
		{
			ShowWindow(this->m_pWindow->GetHandle(), SW_HIDE);
			MessageBox(this->m_pWindow->GetHandle(), L"Could not create a hardware or software Direct3D 11 device!", L"5108COMP Coursework Template", MB_ICONEXCLAMATION | MB_SYSTEMMODAL);
			this->RequestTermination();
			return(false);
		}
		// If using the reference device, utilize a fixed time step for any animations.
		this->m_pTimer->SetFixedTimeStep(1.0f / 10.0f);
	}

	// Create a swap chain for the window that we started out with.  This
	// demonstrates using a configuration object for fast and concise object
	// creation.
	SwapChainConfigDX11 tconfig;
	tconfig.SetWidth(this->m_pWindow->GetWidth());
	tconfig.SetHeight(this->m_pWindow->GetHeight());
	tconfig.SetOutputWindow(this->m_pWindow->GetHandle());
	this->m_iSwapChain = this->m_pRenderer11->CreateSwapChain(&tconfig);
	this->m_pWindow->SetSwapChain(this->m_iSwapChain);

	//Create Colour and Depth Buffers
	this->m_RenderTarget = this->m_pRenderer11->GetSwapChainResource(this->m_iSwapChain);

	Texture2dConfigDX11 tdepthconfig;
	tdepthconfig.SetDepthBuffer(m_iscreenWidth, m_iscreenHeight);
	this->m_DepthTarget = this->m_pRenderer11->CreateTexture2D(&tdepthconfig, 0);

	// Bind the swap chain render target and the depth buffer for use in rendering.  
	this->m_pRenderer11->pImmPipeline->ClearRenderTargets();
	this->m_pRenderer11->pImmPipeline->OutputMergerStage.DesiredState.RenderTargetViews.SetState(0, this->m_RenderTarget->m_iResourceRTV);
	this->m_pRenderer11->pImmPipeline->OutputMergerStage.DesiredState.DepthTargetViews.SetState(this->m_DepthTarget->m_iResourceDSV);
	this->m_pRenderer11->pImmPipeline->ApplyRenderTargets();

	D3D11_VIEWPORT tviewport;
	tviewport.Width = static_cast<float>(m_iscreenWidth);
	tviewport.Height = static_cast<float>(m_iscreenHeight);
	tviewport.MinDepth = 0.0f;
	tviewport.MaxDepth = 1.0f;
	tviewport.TopLeftX = 0;
	tviewport.TopLeftY = 0;

	int tvpindex = this->m_pRenderer11->CreateViewPort(tviewport);
	this->m_pRenderer11->pImmPipeline->RasterizerStage.DesiredState.ViewportCount.SetState(1);
	this->m_pRenderer11->pImmPipeline->RasterizerStage.DesiredState.Viewports.SetState(0, tvpindex);
	return(true);
}

//////////////////////////////////
//Handle Input Events in the Application
//////////////////////////////////
bool LJMULevelDemo::HandleEvent(EventPtr pevent)
{
	eEVENT e = pevent->GetEventType();

	if (e == SYSTEM_KEYBOARD_KEYDOWN)
	{
		EvtKeyDownPtr tkey_down = std::static_pointer_cast<EvtKeyDown>(pevent);
		unsigned int  tkeycode = tkey_down->GetCharacterCode();
	}
	else if (e == SYSTEM_KEYBOARD_KEYUP)
	{
		EvtKeyUpPtr tkey_up = std::static_pointer_cast<EvtKeyUp>(pevent);
		unsigned int tkeycode = tkey_up->GetCharacterCode();
	}

	return(Application::HandleEvent(pevent));
}

//////////////////////////////////
// Destroy Resources created by the engine
//////////////////////////////////
void LJMULevelDemo::ShutdownEngineComponents()
{

	if (this->m_pRenderer11)
	{
		this->m_pRenderer11->Shutdown();
		delete this->m_pRenderer11;
	}

	if (this->m_pWindow)
	{
		this->m_pWindow->Shutdown();
		delete this->m_pWindow;
	}
}

//////////////////////////////////
// Shutdown the Application
//////////////////////////////////
void LJMULevelDemo::Shutdown()
{
	//NOTHING TO DO HERE

}

//////////////////////////////////
// Take a Screenshot of the Application
//////////////////////////////////
void LJMULevelDemo::TakeScreenShot()
{
	if (this->m_bSaveScreenshot)
	{
		this->m_bSaveScreenshot = false;
		this->m_pRenderer11->pImmPipeline->SaveTextureScreenShot(0, this->GetName());
	}
}

//////////////////////////////////////
// Output our Frame Rate
//////////////////////////////////////
std::wstring LJMULevelDemo::outputFPSInfo()
{
	std::wstringstream out;
	out << L"FPS: " << m_pTimer->Framerate();
	return out.str();
}


void LJMULevelDemo::GeneratePlaneIndexArray(int terrainWidth, int terrainLength,
	bool bWinding,
	std::vector<int>& indices)
{
	indices.clear();

	if (terrainWidth >= 2 && terrainLength >= 2)
	{
		for (int XIdx = 0; XIdx < terrainWidth - 1; XIdx++)
		{
			for (int YIdx = 0; YIdx < terrainLength - 1; YIdx++)
			{
				const int I0 = (XIdx + 0) * terrainLength + (YIdx + 0);
				const int I1 = (XIdx + 1) * terrainLength + (YIdx + 0);
				const int I2 = (XIdx + 1) * terrainLength + (YIdx + 1);
				const int I3 = (XIdx + 0) * terrainLength + (YIdx + 1);

				if (bWinding)
					ConvertQuadToTriangles(indices, I0, I1, I2, I3);
				else
					ConvertQuadToTriangles(indices, I0, I3, I2, I1);

			}
		}
	}
}

void LJMULevelDemo::ConvertQuadToTriangles(std::vector<int>& indices,
	int Vert0, int Vert1,
	int Vert2, int Vert3)
{
	indices.push_back(Vert3);
	indices.push_back(Vert1);
	indices.push_back(Vert0);

	indices.push_back(Vert3);
	indices.push_back(Vert2);
	indices.push_back(Vert1);
}

void LJMULevelDemo::LoadTextures()
{
	m_grassTerrainTexture = RendererDX11::Get()->LoadTexture(L"TerrainGrass.tif");
	m_earthTexture = RendererDX11::Get()->LoadTexture(L"8k_earth_daymap.jpg");
	m_grassTerrainTexture = RendererDX11::Get()->LoadTexture(L"TerrainGrass.tif");
	//m_snowTerrainTexture = RendererDX11::Get()->LoadTexture(L"TerrainSnow.tif");
	m_marsTexture = RendererDX11::Get()->LoadTexture(L"mars.tif");
	m_sunTexture = RendererDX11::Get()->LoadTexture(L"sun.jpg");
	m_moonTexture = RendererDX11::Get()->LoadTexture(L"moon.jpg");

	m_cylinderTexture = RendererDX11::Get()->LoadTexture(L"Metal.tif");

	m_sphereTexture = RendererDX11::Get()->LoadTexture(L"Metal.tif");

	m_nightskysphereTexture = RendererDX11::Get()->LoadTexture(L"NightSky.png");
	m_dayskysphereTexture = RendererDX11::Get()->LoadTexture(L"DaySky.png");
	m_sphereBumpTexture = RendererDX11::Get()->LoadTexture(L"normal.jpg");

	m_sphereLightTexture = RendererDX11::Get()->LoadTexture(L"lights.jpg");

	m_cloudTexture = RendererDX11::Get()->LoadTexture(L"cloud.png");
}

MeshPtr LJMULevelDemo::CreateStandardSphere(int h_res,
	int v_res,
	Vector4f colour)
{
	std::vector<Vector3f> vertices;
	std::vector<Vector4f> vertexColors;
	std::vector<Vector2f> texCoord;

	// Cartesian coordinates (x,y,z) variables
	float x;
	float y;
	float z;

	for (int j = 0; j <= v_res; ++j)
	{
		float elevationAngle = GLYPH_PI * float(j) / float(v_res);
		float sp = (float)std::sin(elevationAngle);
		float cp = (float)std::cos(elevationAngle);
		for (int i = 0; i <= h_res; ++i)
		{
			float azimuthAngle = 2.0 * GLYPH_PI * float(i) / float(h_res);
			float sa = std::sin(azimuthAngle);
			float ca = std::cos(azimuthAngle);

			x = sp * ca;
			y = cp;
			z = sp * sa;

			float u = float(i) / float(h_res);
			float v = float(j) / float(v_res);

			vertices.push_back(Vector3f(x, y, z));
			vertexColors.push_back(colour);
			texCoord.push_back(Vector2f(u, v));
		}
	}

	// Setup index array
	std::vector<int> indices;

	GeneratePlaneIndexArray(h_res + 1, v_res + 1, true, indices);

	int numberofIndices = indices.size();
	int numberofTriangles = numberofIndices / 3;

	auto mesh = std::make_shared<DrawExecutorDX11<BasicVertexDX11::Vertex>>();
	mesh->SetLayoutElements(BasicVertexDX11::GetElementCount(), BasicVertexDX11::Elements);
	mesh->SetPrimitiveType(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	mesh->SetMaxVertexCount(numberofIndices);

	BasicVertexDX11::Vertex tv;

	for (int i = 0; i < numberofIndices; i++)
	{
		tv.position = vertices[indices[i]];
		tv.color = vertexColors[indices[i]];
		tv.texcoords = texCoord[indices[i]];

		mesh->AddVertex(tv);
	}

	return mesh;
}

void LJMULevelDemo::updateSphere()
{
	//float rotationSpeed = -0.5f;d
	float rotationSpeed = 0.5f;

	Matrix3f rotMatrix;
	rotMatrix.RotationY(m_tpf * rotationSpeed);

	Matrix3f currentRotMatrix = m_pSphereActor->GetBody()->Rotation();
	m_pSphereActor->GetBody()->Rotation() = currentRotMatrix * rotMatrix;

	Vector4f time = Vector4f(m_tpf, m_totalTime, 0.0f, 0.0f);
	m_pSphereActor->GetBody()->GetMaterial()->Parameters.SetVectorParameter(L"time", time);

	rotationSpeed = 0.1f;
	rotMatrix.RotationY(m_tpf * rotationSpeed);

	currentRotMatrix = m_pCloudActor->GetBody()->Rotation();
	m_pCloudActor->GetBody()->Rotation() = currentRotMatrix * rotMatrix;
}


void LJMULevelDemo::setupPlane()
{
	Vector3f vScale = Vector3f(1, 1, 1);
	Matrix3f mRotation;
	mRotation.MakeIdentity();
	Vector3f vTranslation = Vector3f(0, 256, 0);

	m_terrainMaterial = CreateBasicTerrainMaterial();
	SetTextureToBasicMaterial(m_terrainMaterial, m_grassTerrainTexture);

	setTerrainLightsParameters();

	// Apply light parameters to the shader
	setLights2Material(m_terrainMaterial);

	// Set Material's surface properties
	setMaterialSurfaceProperties(m_terrainMaterial,
		Vector4f(0.0f, 1.0f, 1.0f, 20.0f),
		Vector4f(0.0f, 0.0f, 0.0f, 1.0f));

	m_SpaceBetweenVertices = Vector2f(20.0f, 20.0f);

	float CenterX = m_MapNumVerticesX * m_SpaceBetweenVertices.x / 2;
	float CenterZ = m_MapNumVerticesZ * m_SpaceBetweenVertices.y / 2;
	m_WorldOriginCoord = Vector3f(-CenterX, 0.0f, -CenterZ);

	auto planeMesh = CreatePlaneMesh(Vector3f(0.0f, 0.0f, 0.0f),
		Vector3f(1.0f, 0.0f, 0.0f),
		Vector3f(0.0f, 0.0f, 1.0f),
		Vector2f(256.0f, 256.0f),
		Vector2f(10.0f, 10.0f),
		Vector4f(1.0f, 1.0f, 1.0f, 1.0f),
		Vector2f(0, 0),
		Vector2f(64.0f, 64.0f));

	Actor* planeSegmentActor = new Actor();

	planeSegmentActor->GetBody()->SetGeometry(planeMesh);
	planeSegmentActor->GetBody()->SetMaterial(m_terrainMaterial);
	planeSegmentActor->GetNode()->Scale() = vScale;
	planeSegmentActor->GetNode()->Rotation() = mRotation;
	planeSegmentActor->GetNode()->Position() = vTranslation;

	m_pScene->AddActor(planeSegmentActor);

}

MeshPtr	LJMULevelDemo::CreatePlaneMesh(const Vector3f& center,
	const Vector3f& xdir,
	const Vector3f& zdir,
	const Vector2f& numVertices,
	const Vector2f& spacing,
	const Vector4f& colour,
	const Vector2f& gridPos,
	const Vector2f& texScale)
{
	std::vector<Vector3f> vertices;
	std::vector<Vector4f> vertexColors;
	std::vector<Vector2f> texCoord;

	Vector3f x_unit = xdir;
	Vector3f z_unit = zdir;

	x_unit.Normalize();
	z_unit.Normalize();

	int xVertices = (int)numVertices.x;
	int zVertices = (int)numVertices.y;

	float xSpacing = spacing.x;
	float zSpacing = spacing.y;

	Vector3f x = x_unit * ((float)xVertices - 1) / 2 * xSpacing;
	Vector3f z = z_unit * ((float)xVertices - 1) / 2 * zSpacing;

	// Get the location of the top-left vertex
	Vector3f startPos = center - x - z;

	// Calculate the unit up vector (y_unit).
	// Direct3D uses left-hand rule to determine the direction (since z axis direction is reversed)
	// Hence z cross x gives the y up vector
	Vector3f y_unit = z_unit.Cross(x_unit);
	y_unit.Normalize();

	float heightScale = 10.0f;

	float majorheightfrequency = 0.02f;
	float majorheight = 1.0f;

	float minorheightfrequency = 0.2;
	float minorheight = 0.25f;

	for (int i = 0; i < xVertices; i++)
	{
		for (int j = 0; j < zVertices; j++)
		{
			//float height = cos((float)i * 0.1f)* sin((float)j * 0.1f)* heightScale;

			//Vector3f pos = startPos + x_unit * xSpacing * i + z_unit * zSpacing * j;

			float shifted_i = (float)i + gridPos.x * (xVertices - 1);
			float shifted_j = (float)j + gridPos.y * (zVertices - 1);

			//Vector3f pos = startPos + x_unit * xSpacing * shifted_i + z_unit * zSpacing * shifted_j;

			float majorperiodicheight_x = sin(shifted_i * majorheightfrequency * GLYPH_PI) * majorheight;
			float majorperiodicheight_z = cos(shifted_j * majorheightfrequency * GLYPH_PI) * majorheight;
			float majorperiodicheight = majorperiodicheight_x * majorperiodicheight_z;

			float minorperiodicheight_x = sin(shifted_i * minorheightfrequency * GLYPH_PI) * minorheight;
			float minorperiodicheight_z = cos(shifted_j * minorheightfrequency * GLYPH_PI) * minorheight;
			float minorperiodicheight = minorperiodicheight_x * minorperiodicheight_z;

			float height = (majorperiodicheight + minorperiodicheight) * heightScale;

			Vector3f pos = startPos +
				x_unit * xSpacing * shifted_i +
				z_unit * zSpacing * shifted_j +
				y_unit * height * heightScale;



			vertices.push_back(pos);

			//vertexColors.push_back(colour);

			float shade = (height / heightScale + 1) / 2.0f;

			if (shade < 0)
				shade = 0;
			else if (shade > 1)
				shade = 1;

			vertexColors.push_back(Vector4f(shade, 1 - shade, shade / 2, 1));

			Vector2f normalisedTexScale = Vector2f(texScale.x / numVertices.x, texScale.y / numVertices.y);

			float u = shifted_i;
			float v = shifted_j;

			texCoord.push_back(Vector2f(u, v) * normalisedTexScale);
		}
	}

	int numberofVertices = vertices.size();

	std::vector<int> indices;

	GeneratePlaneIndexArray(xVertices, zVertices, true, indices);

	int numberofIndices = indices.size();
	int numberofTriangles = numberofIndices / 3;

	auto terrainMesh = std::make_shared<DrawExecutorDX11<BasicVertexDX11::Vertex>>();
	terrainMesh->SetLayoutElements(BasicVertexDX11::GetElementCount(),
		BasicVertexDX11::Elements);
	terrainMesh->SetPrimitiveType(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	terrainMesh->SetMaxVertexCount(numberofIndices);

	BasicVertexDX11::Vertex tv;

	for (int i = 0; i < numberofIndices; i++)
	{
		tv.position = vertices[indices[i]];
		tv.color = vertexColors[indices[i]];
		tv.texcoords = texCoord[indices[i]];

		terrainMesh->AddVertex(tv);
	}

	return terrainMesh;
}

MaterialPtr LJMULevelDemo::CreateBasicTerrainMaterial()
{
	MaterialPtr material = MaterialPtr(new MaterialDX11());

	// Create and fill the effect that will be used for this view type
	RenderEffectDX11* pEffect = new RenderEffectDX11();

	pEffect->SetVertexShader(m_pRenderer11->LoadShader(VERTEX_SHADER,
		std::wstring(L"BasicTexturedTerrain.hlsl"),
		std::wstring(L"VSMain"),
		std::wstring(L"vs_4_0")));

	pEffect->SetPixelShader(m_pRenderer11->LoadShader(PIXEL_SHADER,
		std::wstring(L"BasicTexturedTerrain.hlsl"),
		std::wstring(L"PSMain"),
		std::wstring(L"ps_4_0")));

	SamplerStateConfigDX11 SamplerConfig;
	SamplerConfig.AddressU = D3D11_TEXTURE_ADDRESS_MIRROR;
	SamplerConfig.AddressV = D3D11_TEXTURE_ADDRESS_MIRROR;
	SamplerConfig.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
	SamplerConfig.MaxAnisotropy = 1;

	int TextureSampler = RendererDX11::Get()->CreateSamplerState(&SamplerConfig);
	material->Parameters.SetSamplerParameter(L"TextureSampler", TextureSampler);

	// Enable the material to render the given view type, and set its effect.
	material->Params[VT_PERSPECTIVE].bRender = true;
	material->Params[VT_PERSPECTIVE].pEffect = pEffect;

	return material;
}

void LJMULevelDemo::SetTextureToMTMaterial(MaterialPtr material, ResourcePtr texture1, ResourcePtr texture2)
{
	material->Parameters.SetShaderResourceParameter(L"DiffuseTexture0", texture1);
	material->Parameters.SetShaderResourceParameter(L"DiffuseTexture1", texture2);
}

void LJMULevelDemo::SetTexturesToAnimatedMaterial(MaterialPtr material,
	ResourcePtr texture1,
	ResourcePtr texture2)
{
	material->Parameters.SetShaderResourceParameter(L"DiffuseTexture0", texture1);
	material->Parameters.SetShaderResourceParameter(L"DiffuseTexture1", texture2);
}

MaterialPtr LJMULevelDemo::CreateAnimatedTexturedMaterial()
{
	MaterialPtr material = MaterialPtr(new MaterialDX11());

	// Create and fill the effect that will be used for this view type
	RenderEffectDX11* pEffect = new RenderEffectDX11();

	pEffect->SetVertexShader(m_pRenderer11->LoadShader(VERTEX_SHADER,
		std::wstring(L"AnimatedTexture.hlsl"),
		std::wstring(L"VSMain"),
		std::wstring(L"vs_4_0")));

	pEffect->SetPixelShader(m_pRenderer11->LoadShader(PIXEL_SHADER,
		std::wstring(L"AnimatedTexture.hlsl"),
		std::wstring(L"PSMain"),
		std::wstring(L"ps_4_0")));

	RasterizerStateConfigDX11 rsConfig;
	rsConfig.CullMode = D3D11_CULL_NONE;
	rsConfig.FillMode = D3D11_FILL_SOLID;

	int iRasterizerState = m_pRenderer11->CreateRasterizerState(&rsConfig);
	if (iRasterizerState == -1) {
		Log::Get().Write(L"Failed to create light rasterizer state");
		assert(false);
	}

	pEffect->m_iRasterizerState = iRasterizerState;

	SamplerStateConfigDX11 SamplerConfig;
	SamplerConfig.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
	SamplerConfig.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
	SamplerConfig.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
	SamplerConfig.MaxAnisotropy = 1;

	int TextureSampler = RendererDX11::Get()->CreateSamplerState(&SamplerConfig);
	material->Parameters.SetSamplerParameter(L"TextureSampler", TextureSampler);

	// Enable the material to render the given view type, and set its effect.
	material->Params[VT_PERSPECTIVE].bRender = true;
	material->Params[VT_PERSPECTIVE].pEffect = pEffect;

	return material;
}

void LJMULevelDemo::setSkyMapTextureWeight(MaterialPtr material, float w) {
	Vector4f texweight = Vector4f(w, 0, 0, 0);
	material->Parameters.SetVectorParameter(L"texWeight", texweight);
}

void LJMULevelDemo::UpdateSkySphere(float time)
{
	float daylength = 5.0f;
	float abruptness = 5.0f;

	float s = sin(time / daylength);
	float sigmoid = 1 / (1 + exp(-s * abruptness));
	//setSkyMapTextureWeight(m_skysphereMaterial, sigmoid);
	s = (s + 1) / 2;
	setSkyMapTextureWeight(m_skysphereMaterial, s);
}

MaterialPtr LJMULevelDemo::CreateMultiTexturedTerrainMaterial()
{
	MaterialPtr material = MaterialPtr(new MaterialDX11());

	// Create and fill the effect that will be used for this view type
	RenderEffectDX11* pEffect = new RenderEffectDX11();

	pEffect->SetVertexShader(m_pRenderer11->LoadShader(VERTEX_SHADER,
		std::wstring(L"MultiTexturedTerrain.hlsl"),
		std::wstring(L"VSMain"),
		std::wstring(L"vs_4_0")));

	pEffect->SetPixelShader(m_pRenderer11->LoadShader(PIXEL_SHADER,
		std::wstring(L"MultiTexturedTerrain.hlsl"),
		std::wstring(L"PSMain"),
		std::wstring(L"ps_4_0")));

	SamplerStateConfigDX11 SamplerConfig;
	SamplerConfig.AddressU = D3D11_TEXTURE_ADDRESS_MIRROR;
	SamplerConfig.AddressV = D3D11_TEXTURE_ADDRESS_MIRROR;
	SamplerConfig.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
	SamplerConfig.MaxAnisotropy = 1;

	int TextureSampler = RendererDX11::Get()->CreateSamplerState(&SamplerConfig);
	material->Parameters.SetSamplerParameter(L"TextureSampler", TextureSampler);

	// Enable the material to render the given view type, and set its effect.
	material->Params[VT_PERSPECTIVE].bRender = true;
	material->Params[VT_PERSPECTIVE].pEffect = pEffect;

	return material;
}

MeshPtr LJMULevelDemo::generateOBJMesh(std::wstring pmeshname, Vector4f pmeshcolour)
{
	FileSystem fs;
	LJMUMeshOBJ* tmesh = new LJMUMeshOBJ(fs.GetModelsFolder() + pmeshname);
	int tvertcount = tmesh->positions.size();

	auto tia = std::make_shared<DrawExecutorDX11<BasicVertexDX11::Vertex>>();
	tia->SetLayoutElements(BasicVertexDX11::GetElementCount(), BasicVertexDX11::Elements);
	tia->SetPrimitiveType(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	tia->SetMaxVertexCount(tvertcount);

	BasicVertexDX11::Vertex tv;
	tv.color = pmeshcolour;

	for (auto& tobject : tmesh->objects)
	{
		for (auto& tface : tobject.faces)
		{
			for (size_t i = 0; i < 3; ++i)
			{
				tv.position = tmesh->positions[tface.PositionIndices[i]];
				tv.normal = tmesh->normals[tface.NormalIndices[i]];
				tv.texcoords = tmesh->coords[tface.CoordIndices[i]];

				float spinSpeed = (float)(rand() % 100) / 100;
				float spinDirX = (float)(rand() % 100) / 100;
				float spinDirY = (float)(rand() % 100) / 100;
				float spinDirZ = (float)(rand() % 100) / 100;
				tv.color = Vector4f(spinSpeed, spinDirX, spinDirY, spinDirZ);

				tia->AddVertex(tv);
			}
		}
	}
	return tia;
}

MaterialPtr LJMULevelDemo::CreateGSAnimMaterial()
{
	MaterialPtr material = MaterialPtr(new MaterialDX11());

	// Create and fill the effect that will be used for this view type
	RenderEffectDX11* pEffect = new RenderEffectDX11();

	pEffect->SetVertexShader(m_pRenderer11->LoadShader(VERTEX_SHADER,
		std::wstring(L"ProcAnimGS.hlsl"),
		std::wstring(L"VSMain"),
		std::wstring(L"vs_4_0")));

	pEffect->SetPixelShader(m_pRenderer11->LoadShader(PIXEL_SHADER,
		std::wstring(L"ProcAnimGS.hlsl"),
		std::wstring(L"PSMain"),
		std::wstring(L"ps_4_0")));

	// Addition ---
	pEffect->SetGeometryShader(m_pRenderer11->LoadShader(GEOMETRY_SHADER,
		std::wstring(L"ProcAnimGS.hlsl"),
		std::wstring(L"GSMain"),
		std::wstring(L"gs_4_0")));
	// Addition ---

	RasterizerStateConfigDX11 rsConfig;
	rsConfig.CullMode = D3D11_CULL_NONE;
	rsConfig.FillMode = D3D11_FILL_SOLID;

	int iRasterizerState = m_pRenderer11->CreateRasterizerState(&rsConfig);
	if (iRasterizerState == -1) {
		Log::Get().Write(L"Failed to create light rasterizer state");
		assert(false);
	}

	pEffect->m_iRasterizerState = iRasterizerState;

	SamplerStateConfigDX11 SamplerConfig;
	SamplerConfig.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
	SamplerConfig.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
	SamplerConfig.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
	SamplerConfig.MaxAnisotropy = 1;

	int TextureSampler = RendererDX11::Get()->CreateSamplerState(&SamplerConfig);
	material->Parameters.SetSamplerParameter(L"TextureSampler", TextureSampler);

	// Enable the material to render the given view type, and set its effect.
	material->Params[VT_PERSPECTIVE].bRender = true;
	material->Params[VT_PERSPECTIVE].pEffect = pEffect;

	return material;
}

MaterialPtr LJMULevelDemo::CreateGSAnimv2Material()
{
	MaterialPtr material = MaterialPtr(new MaterialDX11());

	// Create and fill the effect that will be used for this view type
	RenderEffectDX11* pEffect = new RenderEffectDX11();

	pEffect->SetVertexShader(m_pRenderer11->LoadShader(VERTEX_SHADER,
		std::wstring(L"ProcAnimGSv2.hlsl"),
		std::wstring(L"VSMain"),
		std::wstring(L"vs_4_0")));

	pEffect->SetPixelShader(m_pRenderer11->LoadShader(PIXEL_SHADER,
		std::wstring(L"ProcAnimGSv2.hlsl"),
		std::wstring(L"PSMain"),
		std::wstring(L"ps_4_0")));

	pEffect->SetGeometryShader(m_pRenderer11->LoadShader(GEOMETRY_SHADER,
		std::wstring(L"ProcAnimGSv2.hlsl"),
		std::wstring(L"GSMain"),
		std::wstring(L"gs_4_0")));


	RasterizerStateConfigDX11 rsConfig;
	rsConfig.CullMode = D3D11_CULL_NONE;
	rsConfig.FillMode = D3D11_FILL_SOLID;

	int iRasterizerState = m_pRenderer11->CreateRasterizerState(&rsConfig);
	if (iRasterizerState == -1) {
		Log::Get().Write(L"Failed to create light rasterizer state");
		assert(false);
	}

	pEffect->m_iRasterizerState = iRasterizerState;

	SamplerStateConfigDX11 SamplerConfig;
	SamplerConfig.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
	SamplerConfig.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
	SamplerConfig.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
	SamplerConfig.MaxAnisotropy = 1;

	int TextureSampler = RendererDX11::Get()->CreateSamplerState(&SamplerConfig);
	material->Parameters.SetSamplerParameter(L"TextureSampler", TextureSampler);

	BlendStateConfigDX11 blendConfig;
	blendConfig.AlphaToCoverageEnable = false;
	blendConfig.IndependentBlendEnable = false;
	for (int i = 0; i < 8; ++i)
	{
		blendConfig.RenderTarget[i].BlendEnable = true;
		blendConfig.RenderTarget[i].BlendOp = D3D11_BLEND_OP_ADD;
		blendConfig.RenderTarget[i].SrcBlend = D3D11_BLEND_SRC_ALPHA;
		blendConfig.RenderTarget[i].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
		blendConfig.RenderTarget[i].BlendOpAlpha = D3D11_BLEND_OP_ADD;
		blendConfig.RenderTarget[i].SrcBlendAlpha = D3D11_BLEND_ZERO;
		blendConfig.RenderTarget[i].DestBlendAlpha = D3D11_BLEND_ZERO;
		blendConfig.RenderTarget[i].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
	}
	pEffect->m_iBlendState = RendererDX11::Get()->CreateBlendState(&blendConfig);

	// Enable the material to render the given view type, and set its effect.
	material->Params[VT_PERSPECTIVE].bRender = true;
	material->Params[VT_PERSPECTIVE].pEffect = pEffect;

	return material;
}

MaterialPtr LJMULevelDemo::createLitTexturedMaterial()
{
	MaterialPtr material = MaterialPtr(new MaterialDX11());

	// Create and fill the effect that will be used for this view type
	RenderEffectDX11* pEffect = new RenderEffectDX11();

	pEffect->SetVertexShader(m_pRenderer11->LoadShader(VERTEX_SHADER,
		std::wstring(L"LitTexture.hlsl"),
		std::wstring(L"VSMain"),
		std::wstring(L"vs_4_0")));

	pEffect->SetPixelShader(m_pRenderer11->LoadShader(PIXEL_SHADER,
		std::wstring(L"LitTexture.hlsl"),
		std::wstring(L"PSMain"),
		std::wstring(L"ps_4_0")));

	SamplerStateConfigDX11 SamplerConfig;
	SamplerConfig.AddressU = D3D11_TEXTURE_ADDRESS_MIRROR;
	SamplerConfig.AddressV = D3D11_TEXTURE_ADDRESS_MIRROR;
	SamplerConfig.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
	SamplerConfig.MaxAnisotropy = 0;

	int TextureSampler = RendererDX11::Get()->CreateSamplerState(&SamplerConfig);
	material->Parameters.SetSamplerParameter(L"TextureSampler", TextureSampler);

	// Enable the material to render the given view type, and set its effect.
	material->Params[VT_PERSPECTIVE].bRender = true;
	material->Params[VT_PERSPECTIVE].pEffect = pEffect;

	return material;
}

void LJMULevelDemo::updatePlanetLight(float time)
{
	m_vAmbientLightColour = Vector4f(1.0f, 1.0f, 1.0f, 1.0f);

	m_vDirectionalLightColour = Vector4f(0.5f, 0.5f, 0.5f, 1.0f);
	m_vDirectionalLightDirection = Vector3f(cos(time), 0.0f, -sin(time));
	m_vDirectionalLightDirection.Normalize();

	// Setting light colour to (0,0,0) to switch it off
	m_vSpotLightColour = Vector4f(0.0f, 0.0f, 0.0f, 1.0f);

	// Setting light colour to (0,0,0) to switch it off
	m_vPointLightColour = Vector4f(0.0f, 0.0f, 0.0f, 1.0f);
}

void	LJMULevelDemo::setPlanetLightsParameters()
{
	m_vAmbientLightColour = Vector4f(1.0f, 1.0f, 1.0f, 1.0f);

	m_vDirectionalLightColour = Vector4f(0.5f, 0.5f, 0.5f, 1.0f);
	m_vDirectionalLightDirection = Vector3f(1.0f, 0.0f, 0.0f);
	m_vDirectionalLightDirection.Normalize();

	// Setting light colour to (0,0,0) to switch it off
	m_vSpotLightColour = Vector4f(0.0f, 0.0f, 0.0f, 1.0f);

	// Setting light colour to (0,0,0) to switch it off
	m_vPointLightColour = Vector4f(0.0f, 0.0f, 0.0f, 1.0f);

}

void	LJMULevelDemo::setLights2Material(MaterialPtr material)
{
	material->Parameters.SetVectorParameter(L"AmbientLightColour",
		m_vAmbientLightColour);

	m_vDirectionalLightDirection.Normalize();
	material->Parameters.SetVectorParameter(L"DirectionalLightColour", m_vDirectionalLightColour);
	material->Parameters.SetVectorParameter(L"DirectionalLightDirection", Vector4f(m_vDirectionalLightDirection, 1.0f));

	m_vSpotLightDirection.Normalize();
	material->Parameters.SetVectorParameter(L"SpotLightColour", m_vSpotLightColour);
	material->Parameters.SetVectorParameter(L"SpotLightDirection", Vector4f(m_vSpotLightDirection, 1.0f));
	material->Parameters.SetVectorParameter(L"SpotLightPosition", m_vSpotLightPosition);
	material->Parameters.SetVectorParameter(L"SpotLightRange", m_vSpotLightRange);
	material->Parameters.SetVectorParameter(L"SpotLightFocus", m_vSpotLightFocus);

	material->Parameters.SetVectorParameter(L"PointLightColour", m_vPointLightColour);
	material->Parameters.SetVectorParameter(L"PointLightPosition", m_vPointLightPosition);
	material->Parameters.SetVectorParameter(L"PointLightRange", m_vPointLightRange);

}

void LJMULevelDemo::setMaterialSurfaceProperties(MaterialPtr material, Vector4f surfaceConstants, Vector4f surfaceEmissiveColour)
{
	material->Parameters.SetVectorParameter(L"SurfaceConstants", surfaceConstants);
	material->Parameters.SetVectorParameter(L"SurfaceEmissiveColour", surfaceEmissiveColour);
}

void	LJMULevelDemo::setTerrainLightsParameters()
{
	m_vAmbientLightColour = Vector4f(1.0f, 1.0f, 1.0f, 1.0f);

	m_vDirectionalLightColour = Vector4f(0.5f, 0.5f, 0.5f, 1.0f);
	m_vDirectionalLightDirection = Vector3f(1.0f, 0.0f, 1.0f);
	m_vDirectionalLightDirection.Normalize();

	m_vSpotLightColour = Vector4f(1.0f, 1.0f, 0.0f, 1.0f);
	m_vSpotLightDirection = Vector3f(0.0f, -1.0f, 0.0f);
	m_vSpotLightDirection.Normalize();

	m_vSpotLightPosition = Vector4f(-500.0f, 500.0f, -700.0f, 1.0f);
	m_vSpotLightRange = Vector4f(700.0f, 0.0f, 0.0f, 0.0f);
	m_vSpotLightFocus = Vector4f(100.0f, 0.0f, 0.0f, 0.0f);

	m_vPointLightColour = Vector4f(1.0f, 0.0f, 0.0f, 1.0f);
	m_vPointLightPosition = Vector4f(100.0f, 500.0f, -100.0f, 1.0f);
	m_vPointLightRange = Vector4f(520.0f, 0.0f, 0.0f, 0.0f);
}

void LJMULevelDemo::updateTerrainLight(float time)
{
	m_vAmbientLightColour = Vector4f(1.0f, 1.0f, 1.0f, 1.0f);

	float lengthofdayinsecond = 10.0f;

	float s = sin(time * GLYPH_PI / lengthofdayinsecond);
	float c = cos(time * GLYPH_PI / lengthofdayinsecond);

	float DayNightTransAbruptness = 10.0f;
	float LightIntensity = 1.0f / (1 + exp(-s * DayNightTransAbruptness));	// Sigmoid Function

	m_vDirectionalLightColour = Vector4f(Vector3f(0.5f, 0.5f, 0.5f) * LightIntensity, 1.0f);
	m_vDirectionalLightDirection = Vector3f(-c, -s, 1.0f);
	m_vDirectionalLightDirection.Normalize();

	m_vSpotLightColour = Vector4f(1.0f, 1.0f, 0.0f, 1.0f);
	m_vSpotLightDirection = Vector3f(-c, -1.0f, -s);
	m_vSpotLightDirection.Normalize();

	m_vSpotLightPosition = Vector4f(-500.0f, 500.0f, -700.0f, 1.0f);
	m_vSpotLightRange = Vector4f(700.0f, 0.0f, 0.0f, 0.0f);
	m_vSpotLightFocus = Vector4f(100.0f, 0.0f, 0.0f, 0.0f);

	m_vPointLightColour = Vector4f(1.0f, 0.0f, 0.0f, 1.0f);
	m_vPointLightPosition = Vector4f(100.0f, 500.0f, -100.0f, 1.0f);
	m_vPointLightRange = Vector4f(520.0f, 0.0f, 0.0f, 0.0f);
}

void LJMULevelDemo::SetupHeightMap()
{

	m_MapNumVerticesX = 512;
	m_MapNumVerticesZ = 512;

	// Uncomment the next two lines to generate height map using sin and cosine functions
	//m_HeightScale = 700.0f;
	//m_WorldHeightmap = GenerateHeightMap(m_MapNumVerticesX, m_MapNumVerticesZ);

	// Uncomment the next three lines to generate height map by loading raw heightmap file
	 //m_HeightScale = 1200.0f;
	 //std::string HeightMapFilename = "heightmap512x512.r16";
	 //m_WorldHeightmap = GenerateHeightMap(HeightMapFilename, m_MapNumVerticesX, m_MapNumVerticesZ);

	// Uncomment the next four lines to generate height map by using a noise generator function
	m_HeightScale = 1550.0f;
	float frequency = 0.0025f * 4;
	int seed = 1024;
	m_WorldHeightmap = GenerateHeightMap(frequency, seed, m_MapNumVerticesX, m_MapNumVerticesZ);

	if (!m_WorldHeightmap)
	{
		return;
	}

	// the remainder of the code will normalise the height to range 0 to 1

	double maxHeight = 0;
	double minHeight = 1000000;

	int hmsize = m_MapNumVerticesX * m_MapNumVerticesZ;

	for (int index = 0; index < hmsize; index++)
	{
		if (maxHeight < m_WorldHeightmap[index])
			maxHeight = m_WorldHeightmap[index];
		if (minHeight > m_WorldHeightmap[index])
			minHeight = m_WorldHeightmap[index];
	}

	double range = maxHeight - minHeight;

	for (int index = 0; index < hmsize; index++)
	{
		m_WorldHeightmap[index] = (m_WorldHeightmap[index] - minHeight) / range;
	}
}

double* LJMULevelDemo::GenerateHeightMap(int HeightMapWidth, int HeightMapLength)
{
	// Calculate the size of the raw image data.
	long actualSize = HeightMapWidth * HeightMapLength;

	double* heightmap = new double[actualSize];
	if (!heightmap)
	{
		return nullptr;
	}

	float majorheightfrequency = 0.02f;
	float majorheight = 255.0f;

	float minorheightfrequency = 0.2;
	float minorheight = 50;

	for (int i = 0; i < HeightMapWidth; i++)
	{
		for (int j = 0; j < HeightMapLength; j++)
		{
			float majorperiodicheight_x = sin(i * majorheightfrequency * GLYPH_PI) * majorheight;
			float majorperiodicheight_z = cos(j * majorheightfrequency * GLYPH_PI) * majorheight;
			float majorperiodicheight = majorperiodicheight_x * majorperiodicheight_z;

			float minorperiodicheight_x = sin(i * minorheightfrequency * GLYPH_PI) * minorheight;
			float minorperiodicheight_z = cos(j * minorheightfrequency * GLYPH_PI) * minorheight;
			float minorperiodicheight = minorperiodicheight_x * minorperiodicheight_z;

			float height = (majorperiodicheight + minorperiodicheight);

			heightmap[i * HeightMapLength + j] = height;
		}
	}

	return heightmap;
}

double* LJMULevelDemo::GenerateHeightMap(std::string filename,
	int HeightMapWidth, int HeightMapLength)
{

	// Calculate the size of the raw image data.
	int actualSize = HeightMapWidth * HeightMapLength;

	// Allocate memory for the raw image data.
	unsigned short* rawImage = new unsigned short[actualSize];
	if (!rawImage)
	{
		return nullptr;
	}

	FILE* filePtr;

	// Open the 16 bit raw height map file for reading in binary.
	int error = fopen_s(&filePtr, filename.c_str(), "rb");
	if (error != 0)
	{
		return false;
	}

	// Read in the raw image data.
	int count = fread(rawImage, sizeof(unsigned short), actualSize, filePtr);
	if (count != actualSize)
	{
		return false;
	}

	// Close the file.
	error = fclose(filePtr);
	if (error != 0)
	{
		return false;
	}

	double* heightmap = new double[actualSize];
	if (!heightmap)
	{
		return nullptr;
	}

	for (int k = 0; k < actualSize; k++)
	{
		heightmap[k] = (double)(rawImage[k]);
	}

	// Release the bitmap image data.
	delete[] rawImage;
	rawImage = 0;

	return heightmap;
}

double* LJMULevelDemo::GenerateHeightMap(float frequency, int seed, int HeightMapWidth, int HeightMapLength)
{
	// Calculate the size of the raw image data.
	long actualSize = HeightMapWidth * HeightMapLength;

	double* heightmap = new double[actualSize];
	if (!heightmap)
	{
		return nullptr;
	}

	FastNoise noiseGenerator;
	noiseGenerator.SetNoiseType(FastNoise::Perlin);
	noiseGenerator.SetSeed(seed);
	noiseGenerator.SetFrequency(frequency);

	for (int i = 0; i < HeightMapWidth; i++)				// we use less-or-equal-than sign to join adjacent grids
	{
		for (int j = 0; j < HeightMapLength; j++)			// we use less-or-equal-than sign to join adjacent grids
		{
			heightmap[i * HeightMapLength + j] = noiseGenerator.GetNoise(i, j);
		}
	}

	return heightmap;
}

MaterialPtr LJMULevelDemo::createTransparentLitTexturedMaterial()
{
	MaterialPtr material = MaterialPtr(new MaterialDX11());

	// Create and fill the effect that will be used for this view type
	RenderEffectDX11* pEffect = new RenderEffectDX11();

	pEffect->SetVertexShader(m_pRenderer11->LoadShader(VERTEX_SHADER,
		std::wstring(L"LitTexture.hlsl"),
		std::wstring(L"VSMain"),
		std::wstring(L"vs_4_0")));

	pEffect->SetPixelShader(m_pRenderer11->LoadShader(PIXEL_SHADER,
		std::wstring(L"LitTexture.hlsl"),
		std::wstring(L"PSMain"),
		std::wstring(L"ps_4_0")));

	SamplerStateConfigDX11 SamplerConfig;
	SamplerConfig.AddressU = D3D11_TEXTURE_ADDRESS_MIRROR;
	SamplerConfig.AddressV = D3D11_TEXTURE_ADDRESS_MIRROR;
	SamplerConfig.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
	SamplerConfig.MaxAnisotropy = 0;

	int TextureSampler = RendererDX11::Get()->CreateSamplerState(&SamplerConfig);
	material->Parameters.SetSamplerParameter(L"TextureSampler", TextureSampler);

	BlendStateConfigDX11 blendConfig;
	blendConfig.AlphaToCoverageEnable = false;
	blendConfig.IndependentBlendEnable = false;
	for (int i = 0; i < 8; ++i)
	{
		blendConfig.RenderTarget[i].BlendEnable = true;
		blendConfig.RenderTarget[i].BlendOp = D3D11_BLEND_OP_ADD;
		blendConfig.RenderTarget[i].SrcBlend = D3D11_BLEND_SRC_ALPHA;
		blendConfig.RenderTarget[i].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
		blendConfig.RenderTarget[i].BlendOpAlpha = D3D11_BLEND_OP_ADD;
		blendConfig.RenderTarget[i].SrcBlendAlpha = D3D11_BLEND_ONE;
		blendConfig.RenderTarget[i].DestBlendAlpha = D3D11_BLEND_ONE;
		blendConfig.RenderTarget[i].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
	}

	pEffect->m_iBlendState = RendererDX11::Get()->CreateBlendState(&blendConfig);

	// Enable the material to render the given view type, and set its effect.
	material->Params[VT_PERSPECTIVE].bRender = true;
	material->Params[VT_PERSPECTIVE].pEffect = pEffect;

	return material;
}