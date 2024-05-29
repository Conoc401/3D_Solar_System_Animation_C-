#pragma once 

#include <vector>
#include <array>
#include <string>
#include <sstream>
#include <fstream>
#include <assert.h>
#include "Vector2f.h"
#include "Vector3f.h"
namespace LJMUDX
{
class LJMUMeshOBJ
{
public:
	//------------CONSTRUCTORS-----------------------------------------------------
	LJMUMeshOBJ(const std::wstring& pfilename)
	{
		this->filename = pfilename;
		this->parseMesh();
	}


protected:
	//-------------HELPER METHODS--------------------------------------------------
	void				parseMesh()
	{	
		std::ifstream tfileobj(this->filename, std::ios::in);
		if (!tfileobj.is_open())
			return; 

		std::string tline;
		size_t tcg = 0;
		this->objects.push_back(object_t());

		while (std::getline(tfileobj,tline))
		{
			std::stringstream			ss(tline);
			std::string					ttoken;
			std::vector<std::string>	ttokens;

			while (ss >> ttoken) 
			{
				ttokens.push_back(ttoken);
			}

			if (ttokens.size() != 0)
			{
				if (ttokens[0] == "v") 
					this->positions.emplace_back(toVec3(ttokens));
				else if (ttokens[0] == "vn") 
					this->normals.emplace_back(toVec3(ttokens));
				else if (ttokens[0] == "vt")
				{
					Vector2f uv = toVec2(ttokens);
					uv.y = 1- uv.y;
					this->coords.emplace_back(uv);
				}
				else if (ttokens[0] == "f") 
				{
					face_t tf;
					for (size_t i = 0; i < 3; ++i)
					{
						auto ttriple = this->toIndexTriple(ttokens[i + 1]);
						tf.PositionIndices[i] = this->wrapOffset(ttriple[0], this->positions.size());
						tf.CoordIndices[i] = this->wrapOffset(ttriple[1], this->coords.size());
						tf.NormalIndices[i] = this->wrapOffset(ttriple[2], this->normals.size());
					}
					this->objects.back().faces.emplace_back(tf);
				}
				else if (ttokens[0] == "o") 
				{
					if (this->objects.back().faces.size() == 0)
					{
						this->objects.back().name = ttokens[1];
					}
					else
					{
						this->objects.push_back(object_t());
						this->objects.back().name = ttokens[1];
					}
				}
				else if (ttokens[0] == "mtllib")
				{
					this->materials.push_back(ttokens[1]);
				}
			}
		}
	}
	Glyph3::Vector3f	toVec3(const std::vector<std::string>& ptokens)
	{
		assert(ptokens.size() >= 4);

		return Glyph3::Vector3f(std::stof(ptokens[1]), std::stof(ptokens[2]), std::stof(ptokens[3]));
	}
	Glyph3::Vector2f	toVec2(const std::vector<std::string>& ptokens)
	{
		assert(ptokens.size() >= 3);
		return Glyph3::Vector2f(std::stof(ptokens[1]),1.0f - std::stof(ptokens[2]));
	}
	std::array<int, 3>	toIndexTriple(const std::string pstr)
	{
		// Split the string according to '/' into tokens
		std::vector<std::string> telems;
		std::stringstream ss(pstr);
		std::string titem;

		while (std::getline(ss,titem, '/')) 
		{
			telems.push_back(titem);
		}

		// We need to have at least one index to do anything.
		assert(telems.size() >= 1 && telems.size() <= 3);

		// initialize to 0, then fill in indices with the available data.
		std::array<int, 3> ttriple = {0,0,0};

		for (size_t i = 0; i < telems.size(); ++i)
		{
			if (telems[i].size() > 0) 
			{
				ttriple[i] = std::stoi(telems[i]) - 1; 
			}
		}
		return ttriple;
	}
	int					wrapOffset(int pindex, int psize)
	{
		if (pindex < 0)
			pindex = pindex + psize + 1; 
		return pindex;
	}

public:

	//Structure to Represent Face Mappings
	typedef struct
	{
		std::array<int,3> PositionIndices;
		std::array<int,3> NormalIndices;
		std::array<int,3> CoordIndices;
	} face_t;

	//Structure to Represent Object Mappings
	typedef struct
	{
		std::string name;
		std::string material_name;
		std::vector<face_t> faces;
	} object_t;
	
	//--------CLASS MEMBERS-------------------------------------------

	//Vertex Data Lists
	std::vector<Glyph3::Vector3f> positions;
	std::vector<Glyph3::Vector3f> normals;
	std::vector<Glyph3::Vector2f> coords;

	//Lists of Objects and Material Details
	std::vector<object_t> objects;
	std::vector<std::string> materials;
	
	//Path to the OBJ File. 
	std::wstring			 filename;
};


}
