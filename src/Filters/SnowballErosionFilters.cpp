#include "SnowballErosionFilters.h"

#include <stdlib.h>
#include <ctime>
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/ext/matrix_relational.hpp>
#include <glm/ext/vector_relational.hpp>
#include <glm/ext/scalar_relational.hpp>
#include <glm/vec3.hpp>

#include <imgui.h>

struct Particle {
    //Construct Particle at Position
    Particle(glm::vec2 _pos) { pos = _pos; }

    glm::vec2 pos;
    glm::vec2 speed = glm::vec2(0.0);

    float volume = 1.0;   //This will vary in time
    float sediment = 0.0; //Fraction of Volume that is Sediment!
};


SnowballErosionProperties props;

void RenderSnowballErosionFilterPropUI() {
    ImGui::DragInt("Seed", &props.SEED, 1);
    ImGui::DragInt("Cycles", &props.cycles, 1);
    ImGui::DragInt("Erosion Step", &props.erosionstep, 1);
    
    ImGui::DragFloat("Density", &props.density, 0.1f);
    ImGui::DragFloat("Evaporation Rate", &props.evapRate, 0.1f);
    ImGui::DragFloat("Deposition Rate", &props.depositionRate, 0.1f);
    ImGui::DragFloat("Friction", &props.friction, 0.1f);
    ImGui::DragFloat("Min Vol", &props.minVol, 0.1f);
    ImGui::DragFloat("dt", &props.dt, 0.1f);
    
    
}

SnowballErosionProperties* GetSnowballErosionProps() {
    return &props;
}

void ApplySnowballErosion(Model* model){
    model->mesh->RecalculateNormals();
    /*
      Note: Everything is properly scaled by a time step-size "dt"
    */
    //Do a series of iterations! (5 Particles)
    props.dim.x = model->mesh->res;
    props.dim.y = model->mesh->res;
    for (int i = 0; i < props.cycles; i++) {
        //Spawn New Particle
        glm::vec2 newpos = glm::vec2(rand() % (int)props.dim.x, rand() % (int)props.dim.y);
        Particle drop(newpos);

        //As long as the droplet exists...
        while (drop.volume > props.minVol) {

            glm::ivec2 ipos = drop.pos;                   //Floored Droplet Initial Position
            glm::vec3 n = model->mesh->GetNormals(ipos.x, ipos.y);  //Surface Normal at Position

            //Accelerate particle using newtonian mechanics using the surface normal.
            drop.speed += props.dt * glm::vec2(n.x, n.z) / (drop.volume * props.density);//F = ma, so a = F/m
            drop.pos += props.dt * drop.speed;
            drop.speed *= (1.0 - props.dt * props.friction);       //Friction Factor

            /*
              Note: For multiplied factors (e.g. friction, evaporation)
              time-scaling is correctly implemented like above.
            */

            //Check if Particle is still in-bounds
            if (!glm::all(glm::greaterThanEqual(drop.pos, glm::vec2(0))) ||
                !glm::all(glm::lessThan(drop.pos, props.dim))) break;

            //Compute sediment capacity difference
            float maxsediment = drop.volume * glm::length(drop.speed) * (model->mesh->GetElevation(ipos.x, ipos.y) - model->mesh->GetElevation((int)drop.pos.x, (int)drop.pos.y));
            if (maxsediment < 0.0) maxsediment = 0.0;
            float sdiff = maxsediment - drop.sediment;

            //Act on the Heightmap and Droplet!
            drop.sediment += props.dt * props.depositionRate * sdiff;
            model->mesh->SetElevation(props.dt * drop.volume * props.depositionRate * sdiff, ipos.x, ipos.y);
            //Evaporate the Droplet (Note: Proportional to Volume! Better: Use shape factor to make proportional to the area instead.)
            drop.volume *= (1.0 - props.dt * props.evapRate);
        }
            model->mesh->RecalculateNormals();
            model->UploadToGPU();
    }
}