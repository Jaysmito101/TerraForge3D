#include "ModelImporter.h"

#include <string>

#include <Utils.h>

#include <assimp/Importer.hpp>      // C++ importer interface
#include <assimp/scene.h>           // Output data structure
#include <assimp/postprocess.h>     // Post processing flags


Mesh* LoadMesh(aiMesh* paiMesh) {
    Mesh mesh;

    const aiVector3D Zero3D(0.0f, 0.0f, 0.0f);
    Vert* verts = new Vert[paiMesh->mNumVertices];
    mesh.vertexCount = paiMesh->mNumVertices;
    for (unsigned int i = 0; i < paiMesh->mNumVertices; i++) {
        const aiVector3D* pPos = &(paiMesh->mVertices[i]);
        const aiVector3D* pNormal = &(paiMesh->mNormals[i]);
        const aiVector3D* pTexCoord = paiMesh->HasTextureCoords(0) ? &(paiMesh->mTextureCoords[0][i]) : &Zero3D;
        Vert tmp;
        tmp.position = glm::vec4(0.0f);
        tmp.position.x = pPos->x;
        tmp.position.y = pPos->y;
        tmp.position.z = pPos->z;

        tmp.texCoord = glm::vec2(1.0f);
        if (pTexCoord) {
            tmp.texCoord.x = pTexCoord->x;
            tmp.texCoord.y = pTexCoord->y;
        }

        tmp.normal = glm::vec4(1.0f);
        
        /*
        if (pNormal) {
            tmp.normal.x = pNormal->x;
            tmp.normal.y = pNormal->y;
            tmp.normal.z = pNormal->z;
        }
        */

        verts[i] = tmp;
    }
    mesh.vert = verts;
    mesh.indexCount = paiMesh->mNumFaces * 3;
    mesh.indices = new int[mesh.indexCount];
    int co = 0;
    for (unsigned int i = 0; i < paiMesh->mNumFaces; i++) {
        const aiFace& Face = paiMesh->mFaces[i];
        mesh.indices[co++] = Face.mIndices[0];
        mesh.indices[co++] = Face.mIndices[1];
        mesh.indices[co++] = Face.mIndices[2];
    }
    return mesh.Clone();
}

Model* LoadModel(std::string path)
{   

    Assimp::Importer importer;

    // And have it read the given file with some example postprocessing
    // Usually - if speed is not the most important aspect for you - you'll
    // probably to request more postprocessing than we do in this example.
    const aiScene* scene = importer.ReadFile(path,
        aiProcess_CalcTangentSpace |
        aiProcess_Triangulate |
        aiProcess_JoinIdenticalVertices |
        aiProcess_SortByPType);

    if (nullptr == scene) {
        Log("Assimp Error : " + std::string(importer.GetErrorString()));
        return nullptr;
    }

    
    aiMesh* paiMesh = scene->mMeshes[0];
    
    Model* model = new Model(std::string(paiMesh->mName.C_Str()));
    model->SetupMeshOnGPU();
       

    model->mesh = LoadMesh(paiMesh);
    return model;
}
