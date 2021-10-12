#include "ModelImporter.h"

#include <Utils.h>

#include <assimp/Importer.hpp>      // C++ importer interface
#include <assimp/scene.h>           // Output data structure
#include <assimp/postprocess.h>     // Post processing flags


void LoadModel(std::string path)
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

    // If the import failed, report it
    if (nullptr != scene) {
        Log("Assimp Error : " + std::string(importer.GetErrorString()));
        return;
    }

    // Now we can access the file's contents.
//    DoTheSceneProcessing(scene);

}
