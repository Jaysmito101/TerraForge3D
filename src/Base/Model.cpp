#include <Model.h>


#include <glm/gtc/constants.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/ext/matrix_relational.hpp>
#include <glm/ext/vector_relational.hpp>
#include <glm/ext/scalar_relational.hpp>

Model::Model(std::string n)
{
	name = n;
}

void Model::Update()
{
	modelMatrix = glm::translate(glm::mat4(0.0f), position);
}
