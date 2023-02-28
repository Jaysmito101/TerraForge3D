#include "Exporters/ColladaExporter.h"
#include "Base/BinaryFileWriter.h"
#include "Utils/Utils.h"

#include <string>
#include <sstream>

// This is not really a good way to do this but i dont really want to take the pains
// of implementing this with a proper XML writer as of now if you are interesed in improving
// this please go ahead and send a Pull Request
static const std::string PART_0 = R"(<?xml version="1.0" encoding="utf-8"?>
<COLLADA xmlns="http://www.collada.org/2005/11/COLLADASchema" version="1.4.1" xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance">
  <asset>
    <contributor>
      <author>TerraForge3D User</author>
      <authoring_tool>TerraForge3D Gen 2</authoring_tool>
    </contributor>
    <created>)";
static const std::string PART_1 = R"(</created>
    <modified>)";
static const std::string PART_2 = R"(</modified>
    <unit name="meter" meter="1"/>
    <up_axis>Y_UP</up_axis>
  </asset>
  <library_images/>
  <library_geometries>
    <geometry id="MainOBJ-mesh" name="MainOBJ">
      <mesh>
        <source id="MainOBJ-mesh-positions">
          <float_array id="MainOBJ-mesh-positions-array" count=")";
static const std::string PART_3 = R"(</float_array>
          <technique_common>
            <accessor source="#MainOBJ-mesh-positions-array" count=")";
static const std::string PART_4 = R"(" stride="3">
              <param name="X" type="float"/>
              <param name="Y" type="float"/>
              <param name="Z" type="float"/>
            </accessor>
          </technique_common>
        </source>
        <source id="MainOBJ-mesh-map-0">
          <float_array id="MainOBJ-mesh-map-0-array" count=")";
static const std::string PART_5 = R"(</float_array>
          <technique_common>
            <accessor source="#MainOBJ-mesh-map-0-array" count=")";
static const std::string PART_6 = R"(" stride="2">
              <param name="S" type="float"/>
              <param name="T" type="float"/>
            </accessor>
          </technique_common>
        </source>
        <vertices id="MainOBJ-mesh-vertices">
          <input semantic="POSITION" source="#MainOBJ-mesh-positions"/>
        </vertices>
        <triangles count=")";
static const std::string PART_7 = R"(">
          <input semantic="VERTEX" source="#MainOBJ-mesh-vertices" offset="0"/>
          <input semantic="TEXCOORD" source="#MainOBJ-mesh-map-0" offset="1" set="0"/>
          <p>)";
static const std::string PART_8 = R"(</p>
        </triangles>
      </mesh>
    </geometry>
  </library_geometries>
  <library_visual_scenes>
    <visual_scene id="Scene" name="Scene">
      <node id="MainOBJ" name="MainOBJ" type="NODE">
        <matrix sid="transform">1 0 0 0 0 1 0 0 0 0 1 0 0 0 0 1</matrix>
        <instance_geometry url="#MainOBJ-mesh" name="MainOBJ"/>
      </node>
    </visual_scene>
  </library_visual_scenes>
  <scene>
    <instance_visual_scene url="#Scene"/>
  </scene>
</COLLADA>
)";

ColladaExporter::ColladaExporter()
{
}

ColladaExporter::~ColladaExporter()
{
}

bool ColladaExporter::Export(const std::string& path, Mesh* mesh, float* progress)
{
	if (!progress) progress = &m_Progress;
	std::stringstream vts, txs, ind;
    *progress = 0.0f;
	if (!PrepareVertices(mesh, vts)) return false; *progress = 0.3f;
    if (!PrepareTexCoords(mesh, txs)) return false; *progress = 0.6f;
    if (!PrepareIndices(mesh, ind)) return false; *progress = 0.9f;
    std::ofstream out_file; out_file.open(path);
	if (!out_file.is_open()) return false;
    out_file << PART_0 << GetTimeStamp() << PART_1 << GetTimeStamp() << PART_2;
	out_file << (mesh->GetVertexCount() * 3) << R"(">)" << vts.str() << PART_3;
    out_file << mesh->GetVertexCount() << PART_4 << (mesh->GetVertexCount() * 2) << R"(">)";
    out_file << txs.str() << PART_5 << mesh->GetVertexCount() << PART_6 << (mesh->GetFaceCount());
    out_file << PART_7 << ind.str() << PART_8;
	out_file.close();
	*progress = 1.0f;
	return true;
}

bool ColladaExporter::PrepareTexCoords(Mesh* mesh, std::stringstream& strm)
{
    for (int i = 0; i < mesh->GetVertexCount(); i++)
    {
        const auto& texCoord = mesh->GetTexCoord(i);
        strm << texCoord.x << ' ' << texCoord.y << ' ';
    }
	return true;
}

bool ColladaExporter::PrepareVertices(Mesh* mesh, std::stringstream& strm)
{
    for (int i = 0; i < mesh->GetVertexCount(); i++)
    {
        const auto& pos = mesh->GetPosition(i);
        strm << pos.x << ' ' << pos.y << ' ' << pos.z << ' ';
    }
    return true;
}

bool ColladaExporter::PrepareIndices(Mesh* mesh, std::stringstream& strm)
{
    for (int i = 0; i < mesh->GetFaceCount(); i++)
    {
        const auto& face = mesh->GetFace(i);
        strm << face.a << ' ' << face.a << ' ' << face.b << ' ' << face.b << ' ' << face.c << ' ' << face.c << ' ';
    }
	return true;
}

