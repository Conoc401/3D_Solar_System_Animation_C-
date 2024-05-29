#pragma once

#include "Application.h"

#include "Win32RenderWindow.h"
#include "RendererDX11.h"

#include "ViewPerspective.h"
#include "Camera.h"
#include "Scene.h"
#include "GeometryActor.h"
#include "FirstPersonCamera.h"

//STL Includes
#include <vector>

//LJMU Framework Includes
#include "LJMUTextOverlay.h"

using namespace Glyph3;

// We define MeshPtr here. This is data type for the 'standard' vertex type provided by Hieroglyph3
// Right-click and select peek definition to find out more
typedef std::shared_ptr<DrawExecutorDX11<BasicVertexDX11::Vertex>> MeshPtr;
typedef std::shared_ptr<DrawIndexedExecutorDX11<BasicVertexDX11::Vertex>> IndexedMeshPtr;

#include "CustomVertexDX11.h"

typedef std::shared_ptr<DrawExecutorDX11<CustomVertexDX11::Vertex>> CustomMeshPtr;

namespace LJMUDX
{
	//////////////////////////////////////
	//LJMULevelDemo.H
	//Class Application for a DirectX 11
	//Driven Application using the DirectX Toolkit
	//Hieroglyph 3 Rendering Engine and LUA.
	//
	//
	//AUTHORS:  DR PO YANG
	//			DR CHRIS CARTER
	//////////////////////////////////////

	class LJMULevelDemo : public Application //Inherit from the Hieroglyph Base Class
	{

	public:
		//------------CONSTRUCTORS------------------------------------------------
		LJMULevelDemo();	//Standard Empty Constructor which builds the object

	public:
		//------------INHERITED METHODS-------------------------------------------
		virtual void Initialize();					//Initialise the DirectX11 Scene
		virtual void Update();						//Update the DirectX Scene
		virtual void Shutdown();					//Shutdown the DirectX11 Scene

		virtual bool ConfigureEngineComponents();	//Initialise Hieroglyph and DirectX TK Modules
		virtual void ShutdownEngineComponents();	//Destroy Hieroglyph and DirectX TK Modules

		virtual void TakeScreenShot();				//Allow a screenshot to be generated

		virtual bool HandleEvent(EventPtr pEvent);	//Handle an I/O Event
		virtual std::wstring GetName();				//Get the Name of this App Instance

		//------------CUSTOM METHODS-----------------------------------------------
		void inputAssemblyStage();					//Stage to setup our VB and IB Info

		std::wstring outputFPSInfo();				//Convert the timer's Frames Per Second to a formatted string

	protected:
		//-------------CLASS MEMBERS-----------------------------------------------
		RendererDX11* m_pRenderer11;		//Pointer to our DirectX 11 Device
		Win32RenderWindow* m_pWindow;			//Pointer to our Windows-Based Window

		int						m_iSwapChain;		//Index of our Swap Chain 
		ResourcePtr				m_RenderTarget;		//Pointer to the GPU Render Target for Colour
		ResourcePtr				m_DepthTarget;		//Pointer to the GPU Render Target for Depth


		//--------------HIEROGLYPH OBJECTS-----------------------------------------
		ViewPerspective* m_pRenderView;		//3D Output View - DirectX 11 Accelerated
		LJMUTextOverlay* m_pRender_text;		//2D Output View - DirectX 11 Accelerated
		Camera* m_pCamera;			//Camera Object

		const float				DEG_TO_RAD = GLYPH_PI / 180.0f;

		float					m_iscreenWidth = 1920.0f;
		float					m_iscreenHeight = 1080.0f;

		void					setupCamera();

		MaterialPtr				m_WireframePlaneMaterial;
		MaterialPtr				createWireframePlaneMaterial();
		MeshPtr					CreateCylinderMesh( int h_res,
													int v_res,
													Vector4f colour);
		void					SetupCylinder();


		void	GeneratePlaneIndexArray(int terrainWidth,
			int terrainLength,
			bool bWinding,
			std::vector<int>& indices);

		void	ConvertQuadToTriangles(std::vector<int>& indices,
			int Vert0, int Vert1,
			int Vert2, int Vert3);

		float		m_totalTime = 0;
		float		m_tpf = 0;

		void		LoadTextures();

		MaterialPtr CreateBasicTexturedMaterial();
		void		SetTextureToBasicMaterial(MaterialPtr material,
			ResourcePtr texture);

		CustomMeshPtr	CreatePlaneCustomMesh(const Vector3f& center,
			const Vector3f& xdir,
			const Vector3f& zdir,
			const Vector2f& numVertices,
			const Vector2f& spacing,
			const Vector4f& colour,
			const Vector2f& gridPos,
			const Vector2f& texScale);

		ResourcePtr	m_earthTexture;
		ResourcePtr	m_marsTexture;
		ResourcePtr	m_sunTexture;
		ResourcePtr	m_moonTexture;
		MaterialPtr m_sphereMaterial;
		Actor* m_pSphereActor;
		Actor* m_pMoonActor;
		Actor* m_pSunActor;
		Actor* m_pMarsActor;
		Actor* m_pSkySphereActor;

		MeshPtr		CreateStandardSphere(int h_res,
			int v_res,
			Vector4f colour);

		void		SetupSphere();
		void		updateSphere();
		void		updateMars();
		void		updateSun();
		void		updateMoon();
		void		UpdateSkySphere();

		ResourcePtr	m_grassTerrainTexture;
		MaterialPtr m_terrainMaterial;
		void		setupPlane();
		MaterialPtr CreateBasicTerrainMaterial();
		MeshPtr		CreatePlaneMesh(const Vector3f& center,
			const Vector3f& xdir,
			const Vector3f& zdir,
			const Vector2f& numVertices,
			const Vector2f& spacing,
			const Vector4f& colour,
			const Vector2f& gridPos,
			const Vector2f& texScale);

		MaterialPtr CreateMultiTexturedTerrainMaterial();

		void		SetTextureToMTMaterial(MaterialPtr material, ResourcePtr texture1, ResourcePtr texture2);

		ResourcePtr	m_cylinderTexture;

		MaterialPtr m_cylinderMaterial;
		MaterialPtr m_sunMaterial;
		MaterialPtr m_moonMaterial;
		MaterialPtr m_marsMaterial;

		Actor* m_pCylinderActor;

		ResourcePtr	m_sphereTexture;

		Vector4f	m_vTextureBoundary;


		void		SetupSkySphere();
		void		SetupMars();
		void		SetupSun();
		void		SetupMoon();
		MaterialPtr m_skysphereMaterial;


		MaterialPtr CreateAnimatedTexturedMaterial();

		void		SetTexturesToAnimatedMaterial(MaterialPtr material,
			ResourcePtr texture1,
			ResourcePtr texture2);

		ResourcePtr	m_nightskysphereTexture;
		ResourcePtr	m_dayskysphereTexture;

		void		setSkyMapTextureWeight(MaterialPtr material, float w);
		void		UpdateSkySphere(float time);

		MeshPtr generateOBJMesh(std::wstring pmeshname, Vector4f pmeshcolour);

		MaterialPtr CreateGSAnimMaterial();
		MaterialPtr CreateGSAnimv2Material();

		MaterialPtr createLitTexturedMaterial();

		void setLights2Material(MaterialPtr material);
		void setMaterialSurfaceProperties(MaterialPtr material, Vector4f surfaceConstants, Vector4f surfaceEmissiveColour);

		void setPlanetLightsParameters();
		void updatePlanetLight(float time);


		// Lighting parameters -----------------

		Vector4f	m_vSurfaceConstants;
		Vector4f	m_vSurfaceEmissiveColour;

		Vector4f	m_vAmbientLightColour;

		Vector4f	m_vDirectionalLightColour;
		Vector3f	m_vDirectionalLightDirection;
		Vector4f	m_vSpotLightColour;
		Vector3f	m_vSpotLightDirection;
		Vector4f	m_vSpotLightPosition;
		Vector4f	m_vSpotLightRange;
		Vector4f	m_vSpotLightFocus;
		Vector4f	m_vPointLightColour;
		Vector4f	m_vPointLightPosition;
		Vector4f	m_vPointLightRange;

		MaterialPtr	createBumpLitTexturedMaterial();

		ResourcePtr	m_sphereBumpTexture;
		void		SetTexturesToBumpLitMaterial(MaterialPtr material,
			ResourcePtr diffusetexture,
			ResourcePtr bumptexture);

		void		SetTexturesToBumpLitMaterial(MaterialPtr material,
			ResourcePtr diffusetexture,
			ResourcePtr bumptexture,
			ResourcePtr lighttexture);

		CustomMeshPtr CreateCustomStandardSphere(int h_res,
			int v_res,
			Vector4f colour);

		ResourcePtr	m_sphereLightTexture;

		ResourcePtr	m_cloudTexture;
		MaterialPtr m_cloudMaterial;
		Actor* m_pCloudActor;

		MaterialPtr createTransparentLitTexturedMaterial();

		void		setTerrainLightsParameters();
		void		updateTerrainLight(float time);

		double* GenerateHeightMap(int HeightMapWidth, int HeightMapLength);
		double* GenerateHeightMap(std::string filename,
			int HeightMapWidth,
			int HeightMapLength);
		double* GenerateHeightMap(float frequency, int seed,
			int HeightMapWidth, int HeightMapLength);

		void		SetupHeightMap();

		double* m_WorldHeightmap;

		Vector3f	m_WorldOriginCoord;

		int			m_MapNumVerticesX;
		int			m_MapNumVerticesZ;

		float		m_HeightScale;
		Vector2f	m_SpaceBetweenVertices;

	};
}
