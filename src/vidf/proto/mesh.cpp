#include "pch.h"
#include "mesh.h"
#include "vidf/common/vector2.h"
#include "vidf/platform/filesystemutils.h"



namespace vidf { namespace proto {


	namespace
	{



		void OutputError(const std::string& fileName, unsigned line, const std::string& message)
		{
			std::cout << fileName;
			if (line > 0)
				std::cout << "(" << line << ")";
			std::cout << " : " << message << std::endl;
		}



		std::string FixIncludeFilePath(const std::string& objFile, const std::string& includeFile)
		{
			std::string path = GetPathFromFilePath(objFile);
			return path + includeFile;
		}



		std::string ReadObjLine(std::istream& is)
		{
			char str[1024];
			is.getline(str, 1024);
			return str;
		}



		void ProcessMap(std::stringstream& line, Module::Map* map)
		{
			map->active = true;
			unsigned mapIdx = unsigned(line.tellg());
			const std::string& str = line.str();
			map->filePath = str.substr(mapIdx+1, str.size()-mapIdx-1);
		}



		void ProcessMtlLine(const std::string& fileName, std::stringstream& line, unsigned lineNumber, Module_ptr outData)
		{
			Module::Material defaultMaterial;
			Module::Material* currentMaterial = outData->GetNumMaterials() > 0 ? &outData->GetMaterial(outData->GetNumMaterials()-1) : &defaultMaterial;

			std::string command;
			line >> command;

			std::string name;
			Color color;
			float fvalue;

			if (command.empty() || command == "#")
			{
			}
			else if (command == "newmtl")
			{
				line >> name;
				outData->CreateMaterial(name);
			}
			else if (command == "Ka")
			{
				line >> color.r >> color.g >> color.b;
				currentMaterial->ambientColor = color;
			}
			else if (command == "Kd")
			{
				line >> color.r >> color.g >> color.b;
				currentMaterial->diffuseColor = color;
			}
			else if (command == "Ks")
			{
				line >> color.r >> color.g >> color.b;
				currentMaterial->specularColor = color;
			}
			else if (command == "Ke")
			{
				line >> color.r >> color.g >> color.b;
				currentMaterial->emissiveColor = color;
			}
			else if (command == "Tf")
			{
				line >> color.r >> color.g >> color.b;
				currentMaterial->transparencyColor = color;
			}
			else if (command == "Ns")
			{
				line >> fvalue;
				currentMaterial->shininess = fvalue;
			}
			else if (command == "Ni")
			{
				line >> fvalue;
				currentMaterial->opticalDensity = fvalue;
			}
			else if (command == "d")
			{
				line >> fvalue;
				currentMaterial->dissolve = fvalue;
			}
			else if (command == "Tr")
			{
				line >> fvalue;
				currentMaterial->transparency = fvalue;
			}
			else if (command == "map_Ka")
			{
				ProcessMap(line, &currentMaterial->ambientMap);
			}
			else if (command == "map_Kd")
			{
				ProcessMap(line, &currentMaterial->diffuseMap);
			}
			else if (command == "map_Ks")
			{
				ProcessMap(line, &currentMaterial->specularMap);
			}
			else if (command == "map_Nd")
			{
				ProcessMap(line, &currentMaterial->opacityMap);
			}
			else if (command == "map_bump" || command == "bump")
			{
				ProcessMap(line, &currentMaterial->bumpMap);
			}
			else if (command == "disp")
			{
				ProcessMap(line, &currentMaterial->displacementMap);
			}
			else if (command == "decal")
			{
				ProcessMap(line, &currentMaterial->decalMap);
			}
			else if (command == "refl")
			{
				ProcessMap(line, &currentMaterial->reflectionMap);
			}
			else if (command == "illum")
			{
				int ivalue;
				line >> ivalue;
				currentMaterial->illumMode;
			}
			else
			{
				OutputError(fileName, lineNumber, "unkown command '" + command + "'");
			}
		}



		void LoadMaterialData(const std::string& fileName, Module_ptr outData)
		{
			std::ifstream mtlStream(fileName.c_str());
			if (!mtlStream)
			{
				OutputError(fileName, 0, "File not found");
				return;
			}

			unsigned lineNumber = 1;
			while (!mtlStream.eof())
			{
				std::stringstream ss;
				std::string line = ReadObjLine(mtlStream);
				ss << line;
				ProcessMtlLine(fileName, ss, lineNumber, outData);
				++lineNumber;
			}
		}



		void ProcessObjLine(const std::string& fileName, std::stringstream& line, unsigned lineNumber, Module_ptr outData)
		{
			std::string command;
			line >> command;

			if (command.empty() || command == "#")
			{
			}
			else if (command == "v")
			{
				float x, y, z;
				line >> x >> y >> z;
				Vector3f p = Vector3f(x, y, z);
				outData->AddVertex(p);
			}
			else if (command == "vn")
			{
				float x, y, z;
				line >> x >> y >> z;
				Vector3f p = Vector3f(x, y, z);
				outData->AddNormal(p);
			}
			else if (command == "vt")
			{
				float x, y;
				line >> x >> y;
				Vector2f p = Vector2f(x, 1-y);
				outData->AddTexCoord(p);
			}
			else if (command == "f")
			{
				outData->BeginPolygon();

				while (!line.eof())
				{
					int v=~0, vn=~0, vt=~0;
					char b;
					line >> v;
					if (v == ~0)
						break;
					if (outData->HasTexCoords())
					{
						line >> b;
						line >> vt;
					}
					if (outData->HasNormals())
					{
						line >> b;
						line >> vn;
					}

					outData->AddPolygonIndices(v-1, vn-1, vt-1);
				}

				outData->EndPolygon();
			}
			else if(command == "g")
			{
				std::string name;
				line >> name;
				outData->BeginGeometry(name);
			}
			else if(command == "s")
			{
				unsigned int smoothGroup;
				line >> smoothGroup;
				// do nothing for now
			}
			else if(command == "l")
			{
				outData->BeginPolyLine();
				while (!line.eof())
				{
					int v = ~0;
					line >> v;
					outData->AddPolyLineIndex(v-1);
				}
				outData->EndPolyLine();
			}
			else if(command == "usemtl")
			{
				std::string name;
				line >> name;
				outData->SetCurrentMaterial(name);
			}
			else if(command == "mtllib")
			{
				std::string name;
				line >> name;
				LoadMaterialData(FixIncludeFilePath(fileName, name), outData);
			}
			else
			{
				OutputError(fileName, lineNumber, "unkown command '" + command + "'");
			}
		}



		void LoadObjData(const std::string& fileName, std::istream& is, Module_ptr outData)
		{
			unsigned lineNumber = 1;
			while (!is.eof())
			{
				std::stringstream ss;
				std::string line = ReadObjLine(is);
				ss << line;
				ProcessObjLine(fileName, ss, lineNumber, outData);
				++lineNumber;
			}
		}


	}



	Module::Map::Map()
		:	active(false)
	{
	}



	Module::Material::Material()
		:	ambientColor(0.0f, 0.0f, 0.0f)
		,	diffuseColor(1.0f, 1.0f, 1.0f)
		,	specularColor(0.0f, 0.0f, 0.0f)
		,	emissiveColor(0.0f, 0.0f, 0.0f)
		,	transparencyColor(0.0f, 0.0f, 0.0f)
		,	shininess(1.0f)
		,	opticalDensity(1.0f)
		,	dissolve(1.0f)
		,	transparency(0.0f)
		,	illumMode(2)
	{
	}



	Module::Geometry::Geometry()
		:	firstPolygon(invalidIndex)
		,	numPolygons(invalidIndex)
	{
	}



	Module::Polygon::Polygon()
		:	firstIndex(invalidIndex)
		,	numVertices(invalidIndex)
		,	materialIndex(invalidIndex)
	{
	}



	Module::PolyLine::PolyLine()
		:	firstIndex(invalidIndex)
		,	numVertices(invalidIndex)
		,	materialIndex(invalidIndex)
	{
	}



	Module::Module()
		:	currentMaterialIndex(invalidIndex)
		,	hasNormals(false)
		,	hasTexCoords(false)
	{
	}



	void Module::AddVertex(Vector3f vertex)
	{
		vertices.push_back(vertex);
	}



	void Module::AddNormal(Vector3f normal)
	{
		normals.push_back(normal);
		hasNormals = true;
	}



	void Module::AddTexCoord(Vector2f texCoord)
	{
		texCoords.push_back(texCoord);
		hasTexCoords = true;
	}



	void Module::BeginGeometry(const std::string& name)
	{
		Geometry geometry;
		geometry.name = name;
		geometry.firstPolygon = unsigned(polygons.size());
		geometry.numPolygons = 0;
		geometry.firstPolyLine = unsigned(polyLines.size());
		geometry.numPolyLines = 0;
		geometries.push_back(geometry);
	}



	void Module::CreateMaterial(const std::string& name)
	{
		materials.push_back(Material());
		materialNameToIndex[name] = unsigned(materials.size()-1);
	}



	void Module::SetCurrentMaterial(const std::string& name)
	{
		std::map<std::string, unsigned>::iterator it = materialNameToIndex.find(name);
		if (it == materialNameToIndex.end())
			currentMaterialIndex = invalidIndex;
		else
			currentMaterialIndex = it->second;
	}



	void Module::BeginPolygon()
	{
		currentPolygon.firstIndex = unsigned(vertexIndices.size());
		currentPolygon.numVertices = 0;
		currentPolygon.materialIndex = currentMaterialIndex;
	}



	void Module::AddPolygonIndices(unsigned vertexIdx, unsigned normalIdx, unsigned texCoordIdx)
	{
		vertexIndices.push_back(vertexIdx);
		normalIndices.push_back(normalIdx);
		texCoordIndices.push_back(texCoordIdx);
		++currentPolygon.numVertices;
	}



	void Module::EndPolygon()
	{
		polygons.push_back(currentPolygon);
		if (!geometries.empty())
			++geometries[geometries.size()-1].numPolygons;
	}



	void Module::BeginPolyLine()
	{
		currentPolyLine.firstIndex = unsigned(lineVertexIndices.size());
		currentPolyLine.numVertices = 0;
		currentPolyLine.materialIndex = currentMaterialIndex;
	}



	void Module::AddPolyLineIndex(unsigned vertexIdx)
	{
		lineVertexIndices.push_back(vertexIdx);
		++currentPolyLine.numVertices;
	}



	void Module::EndPolyLine()
	{
		polyLines.push_back(currentPolyLine);
		if (!geometries.empty())
			++geometries[geometries.size()-1].numPolyLines;
	}



	unsigned Module::GetPolygonVertexIndex(unsigned int polygon, unsigned int vertex) const
	{
		assert(vertex < polygons[polygon].numVertices);
		return vertexIndices[polygons[polygon].firstIndex + vertex];
	}



	unsigned Module::GetPolygonNormalIndex(unsigned int polygon, unsigned int vertex) const
	{
		assert(vertex < polygons[polygon].numVertices);
		return normalIndices[polygons[polygon].firstIndex + vertex];
	}



	unsigned int Module::GetGeometry(const std::string& name) const
	{
		for (size_t i = 0; i < geometries.size(); ++i)
			if (geometries[i].name == name)
				return unsigned(i);
		return invalidIndex;
	}



	unsigned Module::GetPolygonTexCoordIndex(unsigned int polygon, unsigned int vertex) const
	{
		assert(vertex < polygons[polygon].numVertices);
		return texCoordIndices[polygons[polygon].firstIndex + vertex];
	}



	Module_ptr LoadObjModuleFromFile(const std::string& fileName)
	{
		std::ifstream objStream(fileName.c_str());
		if (!objStream)
		{
			OutputError(fileName, 0, "File not found");
			return Module_ptr();
		}

		Module_ptr module = Module_ptr(new Module());
		LoadObjData(fileName, objStream, module);

		return module;
	}



} }

