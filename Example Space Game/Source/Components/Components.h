// define all ECS components related to identification
#ifndef COMPONENTS_H
#define COMPONENTS_H

// example space game (avoid name collisions)
namespace ESG
{
	struct BlenderName { std::string name; };
	struct ModelBoundary { GW::MATH::GOBBF obb; };
	struct ModelTransform {
		GW::MATH::GMATRIXF matrix;
		unsigned int rendererIndex;
	};
	struct Instance { 
		Level_Data::MODEL_INSTANCES instance;
	};
	struct Object {
		Level_Data::LEVEL_MODEL object;
	};
	struct RenderingSystem {};
	//struct Material { float red, greem, blue; };
}
#endif