#pragma once
#include "PCH.h"
#include "Vector2f.h"
#include "Vector3f.h"
#include "Vector4f.h"
//--------------------------------------------------------------------------------
using namespace Glyph3;

class CustomVertexDX11
{
public:
	CustomVertexDX11();
	~CustomVertexDX11();

	struct Vertex {
		Vector3f position;
		Vector3f normal;
		Vector4f color;
		Vector2f texcoords;
		Vector2f texweights;
		Vector3f tangent;	// Additional element
		Vector3f binormal;	// Additional element
	};

	static unsigned int GetElementCount();
	static D3D11_INPUT_ELEMENT_DESC Elements[5]; // changed to 7 from 5
};
