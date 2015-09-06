#pragma once

#include "vidf/common/color.h"

namespace vidf { namespace proto {



	class Module
	{
	public:
		static const unsigned invalidIndex = ~0u;

		struct Map
		{
			Map();

			std::string filePath;
			bool active;
		};

		struct Material
		{
			Material();

			Color ambientColor;
			Color diffuseColor;
			Color specularColor;
			Color emissiveColor;
			Color transparencyColor;
			float shininess;
			float opticalDensity;
			float dissolve;
			float transparency;

			int illumMode;

			Map ambientMap;
			Map diffuseMap;
			Map specularMap;
			Map shininessMap;
			Map opacityMap;
			Map bumpMap;
			Map displacementMap;
			Map decalMap;
			Map reflectionMap;
		};

		struct Polygon
		{
			Polygon();
			unsigned firstIndex;
			unsigned numVertices;
			unsigned materialIndex;
		};

		struct PolyLine
		{
			PolyLine();
			unsigned firstIndex;
			unsigned numVertices;
			unsigned materialIndex;
		};

		struct Geometry
		{
			Geometry();
			std::string name;
			unsigned int firstPolygon;
			unsigned int numPolygons;
			unsigned int firstPolyLine;
			unsigned int numPolyLines;
		};

	public:
		Module();

		void AddVertex(Vector3f vertex);
		void AddNormal(Vector3f normal);
		void AddTexCoord(Vector2f texCoord);

		void BeginGeometry(const std::string& name);

		void CreateMaterial(const std::string& name);
		void SetCurrentMaterial(const std::string& name);

		void BeginPolygon();
		void AddPolygonIndices(unsigned vertexIdx, unsigned normalIdx, unsigned texCoordIdx);
		void EndPolygon();

		void BeginPolyLine();
		void AddPolyLineIndex(unsigned vertexIdx);
		void EndPolyLine();

		bool HasNormals() const {return hasNormals;}
		bool HasTexCoords() const {return hasTexCoords;}

		unsigned GetNumVertices() const {return unsigned(vertices.size());}
		unsigned GetNumNormals() const {return unsigned(normals.size());}
		unsigned GetNumTexCoords() const {return unsigned(texCoords.size());}
		unsigned GetNumPolygons() const {return unsigned(polygons.size());}
		unsigned GetNumGeometries() const {return unsigned(geometries.size());}
		unsigned GetNumMaterials() const {return unsigned(materials.size());}
		Vector3f GetVertex(unsigned int idx) const {return vertices[idx];}
		Vector3f GetNormal(unsigned int idx) const {return normals[idx];}
		Vector2f GetTexCoord(unsigned int idx) const {return texCoords[idx];}
		const Geometry& GetGeometry(unsigned int idx) const {return geometries[idx];}
		unsigned int GetGeometry(const std::string& name) const;
		const Material& GetMaterial(unsigned int idx) const {return materials[idx];}
		Material& GetMaterial(unsigned int idx) {return materials[idx];}

		unsigned GetPolygonNumVertices(unsigned int polygon) const {return polygons[polygon].numVertices;}
		unsigned GetPolygonVertexIndex(unsigned int polygon, unsigned int vertex) const;
		unsigned GetPolygonNormalIndex(unsigned int polygon, unsigned int vertex) const;
		unsigned GetPolygonTexCoordIndex(unsigned int polygon, unsigned int vertex) const;
		unsigned GetPolygonMaterialIndex(unsigned int polygon) const {return polygons[polygon].materialIndex;}

		unsigned GetPolyLineNumVertices(unsigned int polyline) const {return polyLines[polyline].numVertices;}
		unsigned GetPolyLineVertexIndex(unsigned int polyline, unsigned int vertex) const {return lineVertexIndices[polyLines[polyline].firstIndex + vertex];}
		unsigned GetPolyLineMaterialIndex(unsigned int polyline) const {return polyLines[polyline].materialIndex;}

	private:
		Polygon currentPolygon;
		PolyLine currentPolyLine;
		std::vector<Vector3f> vertices;
		std::vector<Vector3f> normals;
		std::vector<Vector2f> texCoords;
		std::vector<Polygon> polygons;
		std::vector<PolyLine> polyLines;
		std::vector<Geometry> geometries;
		std::vector<Material> materials;
		std::vector<unsigned> vertexIndices;
		std::vector<unsigned> normalIndices;
		std::vector<unsigned> texCoordIndices;
		std::vector<unsigned> lineVertexIndices;
		std::map<std::string, unsigned> materialNameToIndex;
		unsigned currentMaterialIndex;
		bool hasNormals;
		bool hasTexCoords;
	};

	typedef std::shared_ptr<Module> Module_ptr;



	Module_ptr LoadObjModuleFromFile(const std::string& fileName);


} }
