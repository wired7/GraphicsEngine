#pragma once
#include "Controller.h"
#include "GeometricalMeshObjects.h"
#include "ShaderProgramPipeline.h"
#include "Pass.h"

class AbstractContext
{
public:
	static AbstractContext* activeContext;
	virtual void update() = 0;
};

template<class ControllerType, class CameraType, class ContextType> class Context : public AbstractContext
{
public:
	ControllerType* controller = nullptr;
	std::vector<CameraType*> cameras;
	AbstractContext* nextContext = nullptr;
	AbstractContext* prevContext = nullptr;
	Context();
	~Context() {};
	virtual void setAsActiveContext(void);
};

#pragma region ContextTemplate
template<class ControllerType, class CameraType, class ContextType> Context<ControllerType, CameraType, ContextType>::Context()
{
}

template<class ControllerType, class CameraType, class ContextType> void Context<ControllerType, CameraType, ContextType>::setAsActiveContext(void)
{
	activeContext = this;

	controller = ControllerType::getController();
	controller->setContext(dynamic_cast<ContextType*>(this));
	controller->setController();
}
#pragma endregion


template<class ControllerType, class CameraType, class ContextType> class GraphicsSceneContext : public Context<ControllerType, CameraType, ContextType>
{
protected:
	virtual void setupCameras(void) = 0;
	virtual void setupGeometries(void) = 0;
	virtual void setupPasses(const std::vector<std::string>& gProgramSignatures,
							 const std::vector<std::string>& lProgramSignatures);
public:
	bool dirty = true;
	Pass* passRootNode = nullptr;
	std::map<std::string, Graphics::DecoratedGraphicsObject*> geometries;
	GraphicsSceneContext() {};
	~GraphicsSceneContext() {};
	virtual void update(void);
	void setAsActiveContext(void) override;
};

#pragma region GraphicsSceneContextTemplate

template<class ControllerType, class CameraType, class ContextType>
void GraphicsSceneContext<ControllerType, CameraType, ContextType>::setupPasses(const std::vector<std::string>& gProgramSignatures,
																				const std::vector<std::string>& lProgramSignatures)
{
	// TODO: might want to manage passes as well
	std::unordered_map<std::string, ShaderProgramPipeline*> gPrograms;

	for (const auto& programSignature : gProgramSignatures)
	{
		gPrograms[programSignature] = ShaderProgramPipeline::getPipeline(programSignature);
	}

	GeometryPass* gP = new GeometryPass(gPrograms, "GEOMETRYPASS", nullptr, 1);
	gP->setupCamera(GraphicsSceneContext<ControllerType, CameraType, ContextType>::cameras[0]);

	std::unordered_map<std::string, ShaderProgramPipeline*> lPrograms;

	for (const auto& programSignature : lProgramSignatures)
	{
		lPrograms[programSignature] = ShaderProgramPipeline::getPipeline(programSignature);
	}

	LightPass* lP = new LightPass(lPrograms, true);

	gP->addNeighbor(lP);

	passRootNode = gP;
}

template<class ControllerType, class CameraType, class ContextType>
void GraphicsSceneContext<ControllerType, CameraType, ContextType>::update(void)
{
	for (const auto& camera : GraphicsSceneContext<ControllerType, CameraType, ContextType>::cameras)
	{
		if (camera->timeStepUpdate())
		{
			dirty = true;
		}
	}

	if (dirty)
	{
		if (passRootNode != nullptr)
		{
			passRootNode->execute();
		}
		dirty = false;
	}
}

template<class ControllerType, class CameraType, class ContextType>
void GraphicsSceneContext<ControllerType, CameraType, ContextType>::setAsActiveContext(void)
{
	Context<ControllerType, CameraType, ContextType>::setAsActiveContext();
	dirty = true;
}

#pragma endregion
