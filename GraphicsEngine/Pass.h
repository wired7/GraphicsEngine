#pragma once
#include "Camera.h"
#include "DirectedGraphNode.h"
#include "GraphicsObject.h"
#include "ShaderProgram.h"
#include <unordered_map>
#include <unordered_set>
#include <string>
#include <typeinfo>
#include <typeindex>
#include <tuple>
#include <chrono>
#include <functional>

class ShaderProgramPipeline;
class CLKernel;
class AbstractCLBuffer;
class DecoratedFrameBuffer;
class Observer;

class Pass : public DirectedGraphNode<Pass>
{
protected:
	int currentCount = 0;
	bool isTimedPass = false;
	long timedPassTimeLimit = 0;
	std::chrono::time_point<std::chrono::steady_clock> start;
	std::unordered_map<std::string, ShaderProgramPipeline*> shaderPipelines;
	std::unordered_map<std::string, std::function<void()>> postExecuteFunctors;

	virtual void executeOwnBehaviour() = 0;
public:
	Pass();
	Pass(std::unordered_map<std::string, ShaderProgramPipeline*> shaderPipelines, std::vector<Pass*> neighbors, std::string signature);
	Pass(std::unordered_map<std::string, ShaderProgramPipeline*> shaderPipelines, std::string signature);
	~Pass();
	virtual void execute(void);
	virtual void setTimedPassTimeLimit(long time);
	virtual void registerPostExecuteFunctor(std::string signature, std::function<void()> functor);
};

class RenderPass : public Pass
{
protected:
	static Graphics::DecoratedGraphicsObject* quad;

	class RenderEdgeData : public DirectedGraphNode<Pass>::EdgeData
	{
	public:
		std::vector<DecoratedFrameBuffer*> frameBuffers;

		RenderEdgeData(Pass* source, Pass* destination, std::vector<DecoratedFrameBuffer*> fb = {}) : frameBuffers(fb),
			DirectedGraphNode<Pass>::EdgeData(source, destination) {};
	};

	std::unordered_map<std::string, std::unordered_map<std::string, std::tuple<std::string, GLuint, GLuint, UniformType, int>>> uniformIDs;
	std::unordered_map<std::string, std::unordered_map<std::string, std::tuple<std::string, GLfloat*>>> floatTypeUniformPointers;
	std::unordered_map<std::string, std::unordered_map<std::string, std::tuple<std::string, GLuint*>>> uintTypeUniformPointers;
	std::unordered_map<std::string, std::unordered_map<std::string, std::tuple<std::string, GLint*>>> intTypeUniformPointers;
	std::unordered_map<std::string, std::unordered_map<std::string, std::tuple<std::string, GLuint>>> uintTypeUniformValues;
	std::unordered_map<std::string, std::unordered_map<std::string, Graphics::DecoratedGraphicsObject*>> renderableObjects;
	bool terminal;
	virtual void initFrameBuffers(void) = 0;
	virtual void configureGL(const std::string& programSignature) {};
	virtual void renderObjects(const std::string& programSignature);
	virtual void setupObjectwiseUniforms(const std::string& programSignature, const std::string& signature) {};
	virtual void executeOwnBehaviour(void);
public:
	bool clearBuff = true;
	std::unordered_map<std::string, DecoratedFrameBuffer*> frameBuffers;
	RenderPass(std::unordered_map<std::string, ShaderProgramPipeline*> shaderPipelines, std::string signature,
			   DecoratedFrameBuffer* frameBuffer, bool terminal = false);
	~RenderPass() {};
	// TODO: Give the user the ability to apply renderable objects to any inner pass for rendering before the call
	virtual void clearRenderableObjects(const std::string& programSignature);
	virtual void clearRenderableObjects(const std::string& signature, const std::string& programSignature);
	virtual void setRenderableObjects(std::unordered_map<std::string,
									  std::unordered_map<std::string, Graphics::DecoratedGraphicsObject*>> input);
//	virtual void setRenderableObjects(std::vector<std::vector<Graphics::DecoratedGraphicsObject*>> input, std::string signature) {};
	virtual void addRenderableObjects(Graphics::DecoratedGraphicsObject* input, const std::string& signature,
									  const std::string& programSignature);
	virtual void setProbe(const std::string& passSignature, const std::string& frameBufferSignature) {};
	virtual void addFrameBuffer(DecoratedFrameBuffer* fb);
	virtual void registerUniforms(void);
	template<typename T> void updateFloatPointerBySignature(const std::string& programSignature, std::string signature, T* pointer);
	template<typename T> void updateIntPointerBySignature(const std::string& programSignature, std::string signature, T* pointer);
	template<typename T> void updateValueBySignature(const std::string& programSignature, std::string signature, T value);
	virtual void setUniforms(const std::string& programSignature);
	virtual void setupCamera(Camera* cam);
	virtual void setupVec4f(glm::vec4& input, std::string name);
	virtual void setupVec2i(glm::ivec2& input, std::string name);
	virtual void setupFloat(float* input, std::string name);
	using Pass::addNeighbor;
	virtual void addNeighbor(Pass* neighbor, std::vector<std::string> passedFrameBufferSignatures);
	virtual DecoratedFrameBuffer* getFrameBuffer(std::string signature);
};

// IMPLEMENT FOR OTHER TYPES!!!
template<typename T> void RenderPass::updateFloatPointerBySignature(const std::string& programSignature, std::string signature, T* pointer)
{
	if (std::is_same<T, GLfloat>::value || std::is_same<T, float>::value)
	{
		if (floatTypeUniformPointers[programSignature].find(signature) != floatTypeUniformPointers[programSignature].end())
		{
			std::get<1>(floatTypeUniformPointers[programSignature][signature]) = pointer;
			return;
		}

		floatTypeUniformPointers[programSignature][signature] = std::make_tuple(signature, pointer);
	}
}

template<typename T> void RenderPass::updateIntPointerBySignature(const std::string& programSignature, std::string signature, T* pointer)
{
	if (std::is_same<T, GLint>::value || std::is_same<T, int>::value)
	{
		if (intTypeUniformPointers[programSignature].find(signature) != intTypeUniformPointers[programSignature].end())
		{
			std::get<1>(intTypeUniformPointers[programSignature][signature]) = pointer;
			return;
		}

		intTypeUniformPointers[programSignature][signature] = std::make_tuple(signature, pointer);
	}
}

template<typename T> void RenderPass::updateValueBySignature(const std::string& programSignature, std::string signature, T value)
{
	if (std::is_same<T, GLuint>::value || std::is_same<T, unsigned int>::value)
	{
		if (uintTypeUniformValues[programSignature].find(signature) != uintTypeUniformValues[programSignature].end())
		{
			std::get<1>(uintTypeUniformValues[programSignature][signature]) = value;
			return;
		}

		uintTypeUniformValues[programSignature][signature] = std::make_tuple(signature, value);
	}
}

class GeometryPass : public RenderPass
{
protected:
	GLenum clearType;
	virtual void initFrameBuffers(void);
	virtual void configureGL(const std::string& programSignature);
	void setupObjectwiseUniforms(const std::string& programSignature, const std::string& signature) override;
public:
	int pickingBufferCount;
	int stencilBufferCount;
	GeometryPass(std::unordered_map<std::string, ShaderProgramPipeline*> shaderPipelines,
				 std::string signature = "GEOMETRYPASS",
				 DecoratedFrameBuffer* frameBuffer = nullptr,
				 int pickingBufferCount = 0,
				 int stencilBufferCount = 0,
				 bool terminal = false,
				 GLenum clearType = GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	~GeometryPass() {};
	virtual void setupOnHover(unsigned int id);
};

class IntermediatePass : public GeometryPass
{
protected:
	virtual void configureGL(const std::string& programSignature);
public:
	IntermediatePass(std::unordered_map<std::string, ShaderProgramPipeline*> shaderPipelines,
					 int pickingBuffers,
					 const std::string& signature = "INTERMEDIATEPASS");
	~IntermediatePass() {};
};

class LightPass : public RenderPass
{
protected:
	bool depthTest = false;
	virtual void initFrameBuffers(void);
	virtual void configureGL(const std::string& programSignature);
public:
	LightPass(std::unordered_map<std::string, ShaderProgramPipeline*> shaderPipelines, bool terminal = false);
	LightPass(std::unordered_map<std::string, ShaderProgramPipeline*> shaderPipelines,
			  DecoratedFrameBuffer* frameBuffer);
	~LightPass() {};
	virtual void setDepthTest(bool dT);
};

