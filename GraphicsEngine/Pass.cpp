#pragma once
#include "Pass.h"
#include "WindowContext.h"
#include "GLFWWindowContext.h"
#include "GeometricalMeshObjects.h"
#include "FrameBuffer.h"
#include "ShaderProgramPipeline.h"

Pass::Pass()
{
}

Pass::Pass(std::unordered_map<std::string, ShaderProgramPipeline*> shaderPipelines, std::string signature) :
	DirectedGraphNode(signature), shaderPipelines(shaderPipelines)
{
}

Pass::~Pass()
{
}

void Pass::setTimedPassTimeLimit(long time)
{
	start = std::chrono::steady_clock::now();
	isTimedPass = true;
}

void Pass::execute(void)
{
	if (++currentCount < parentEdges.size())
	{
		return;
	}

	if (isTimedPass)
	{
		auto end = std::chrono::steady_clock::now();
		auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

		if (duration < timedPassTimeLimit)
			return;
	}

	currentCount = 0;

	executeOwnBehaviour();

	for (const auto& func : postExecuteFunctors)
	{
		func.second();
	}

	for (const auto& edge : neighborEdges)
	{
		edge->data->destination->execute();
	}

	if (isTimedPass)
	{
		start = std::chrono::steady_clock::now();
	}
}

void Pass::registerPostExecuteFunctor(std::string signature, std::function<void()> functor)
{
	postExecuteFunctors.insert(std::make_pair(signature, functor));
}

Graphics::DecoratedGraphicsObject* makeQuad(void)
{
	GLFWWindowContext::initialize();
	auto displayQuad = new Graphics::Quad();

	std::vector<glm::vec2> uvMap;
	uvMap.push_back(glm::vec2(0, 0));
	uvMap.push_back(glm::vec2(1, 0));
	uvMap.push_back(glm::vec2(0, 1));
	uvMap.push_back(glm::vec2(1, 1));

	return new Graphics::ExtendedMeshObject<glm::vec2, float>(displayQuad, uvMap, "TEXTURECOORD");
}

Graphics::DecoratedGraphicsObject* RenderPass::quad = makeQuad();

RenderPass::RenderPass(std::unordered_map<std::string, ShaderProgramPipeline*> shaderPipelines, std::string signature,
										  DecoratedFrameBuffer* frameBuffer, bool terminal) :
	Pass(shaderPipelines, signature), terminal(terminal)
{
	registerUniforms();

	for (const auto& pipeline : shaderPipelines)
	{
		renderableObjects[pipeline.second->signature] = std::unordered_map<std::string, Graphics::DecoratedGraphicsObject*>();
		floatTypeUniformPointers[pipeline.second->signature] = std::unordered_map<std::string, std::tuple<std::string, GLfloat*>>();
		uintTypeUniformPointers[pipeline.second->signature] = std::unordered_map<std::string, std::tuple<std::string, GLuint*>>();
		intTypeUniformPointers[pipeline.second->signature] = std::unordered_map<std::string, std::tuple<std::string, GLint*>>();
		uintTypeUniformValues[pipeline.second->signature] = std::unordered_map<std::string, std::tuple<std::string, GLuint>>();
	}
}

void RenderPass::clearRenderableObjects(const std::string& signature)
{
	renderableObjects[signature].clear();
}

void RenderPass::clearRenderableObjects(const std::string& signature, const std::string& programSignature)
{
	if (renderableObjects.find(programSignature) != renderableObjects.end())
	{
		renderableObjects[programSignature].erase(signature);

		if (renderableObjects[programSignature].empty())
		{
			renderableObjects.erase(programSignature);
		}
	}
}

void RenderPass::setRenderableObjects(std::unordered_map<std::string,
									  std::unordered_map<std::string, Graphics::DecoratedGraphicsObject*>> input)
{
	renderableObjects = input;
}

void RenderPass::addRenderableObjects(Graphics::DecoratedGraphicsObject* input, const std::string& signature, const std::string& programSignature)
{
	renderableObjects[programSignature][signature] = input;
}

void RenderPass::addFrameBuffer(DecoratedFrameBuffer* fb)
{
	frameBuffers.insert(std::make_pair(fb->signature, fb));
}

void RenderPass::setUniforms(const std::string& programSignature)
{
	auto idsByProgram = uniformIDs[programSignature];

	for (const auto& uniformID : idsByProgram)
	{
		if (std::get<3>(uniformID.second) == TEXTURE)
		{
			int pos = std::get<4>(uniformID.second);
			glProgramUniform1i(std::get<1>(uniformID.second), std::get<2>(uniformID.second), pos);
		}
	}

	auto floatPointersByProgram = floatTypeUniformPointers[programSignature];
	// IMPLEMENT FOR OTHER DATA TYPES
	for (const auto& ptr : floatPointersByProgram)
	{
		auto uID = uniformIDs[programSignature][std::get<0>(ptr.second)];

		if (std::get<3>(uID) == MATRIX4FV)
		{
			glProgramUniformMatrix4fv(std::get<1>(uID), std::get<2>(uID), 1, GL_FALSE, std::get<1>(ptr.second));
		}
		else if (std::get<3>(uID) == VECTOR4FV)
		{
			glProgramUniform4fv(std::get<1>(uID), std::get<2>(uID), 1, std::get<1>(ptr.second));
		}
		else if (std::get<3>(uID) == FLOAT)
		{
			glProgramUniform1fv(std::get<1>(uID), std::get<2>(uID), 1, std::get<1>(ptr.second));
		}
	}

	auto intPointerByProgram = intTypeUniformPointers[programSignature];
	for (const auto& ptr : intPointerByProgram)
	{
		auto uID = uniformIDs[programSignature][std::get<0>(ptr.second)];
		
		if (std::get<3>(uID) == VECTOR2IV)
		{
			glProgramUniform2iv(std::get<1>(uID), std::get<2>(uID), 1, std::get<1>(ptr.second));
		}
	}

	auto uintValuesByProgram = uintTypeUniformValues[programSignature];
	for (const auto& val : uintValuesByProgram)
	{
		auto uID = uniformIDs[programSignature][std::get<0>(val.second)];

		if (std::get<3>(uID) == ONEUI)
		{
			glProgramUniform1ui(std::get<1>(uID), std::get<2>(uID), std::get<1>(val.second));
		}
	}
}

void RenderPass::renderObjects(const std::string& programSignature)
{
	auto objectsByProgram = renderableObjects[programSignature];
	for (const auto& object :objectsByProgram)
	{
		setupObjectwiseUniforms(programSignature, object.first);
		object.second->enableBuffers();
		object.second->draw();
	}
}

void RenderPass::registerUniforms(void)
{
	for (const auto& pipeline : shaderPipelines)
	{
		uniformIDs[pipeline.second->signature] = std::unordered_map<std::string, std::tuple<std::string, GLuint, GLuint, UniformType, int>>();

		for (const auto& program : pipeline.second->attachedPrograms)
		{
			for (auto& uniformIDPair : program->uniformIDs)
			{
				auto uniformID = uniformIDPair.second;

				uniformIDs[pipeline.second->signature][std::get<0>(uniformID)] = std::make_tuple(
					std::get<0>(uniformID),
					program->program,
					std::get<1>(uniformID),
					std::get<2>(uniformID),
					std::get<3>(uniformID));
			}
		}
	}
}

void RenderPass::executeOwnBehaviour()
{
	// Set input textures from incoming passes for this stage
//	std::cout << "PASS: " << signature << std::endl;
	int count = 0;
	for (const auto& edge : parentEdges)
	{
		auto data = dynamic_cast<RenderEdgeData*>(edge->data);

		if (data == nullptr)
		{
			continue;
		}

		auto fbs = data->frameBuffers;

/*		if(fbs.size())
			std::cout << "\tFROM: " << data->source->signature << std::endl;*/
		for (const auto fb : fbs)
		{
			count = fb->bindTexturesForPass(count);
		}
	}

	// Set output textures
	std::vector<GLuint> buff(frameBuffers.size());
	DecoratedFrameBuffer* lastBuffer = nullptr;
	for (const auto& fb : frameBuffers)
	{
		lastBuffer = fb.second->drawBuffers(buff);
	}

	if (lastBuffer != nullptr)
	{
		lastBuffer->postDrawBuffers(buff);
	}

//	std::cout << std::endl;

	for (const auto pipeline : shaderPipelines)
	{
		// GL configuration
		configureGL(pipeline.second->signature);

		// Shader setup
		pipeline.second->use();

		// Texture and miscellaneous uniforms setup
		setUniforms(pipeline.second->signature);

		// Object rendering
		renderObjects(pipeline.second->signature);
	}
}

void RenderPass::setupCamera(Camera* cam)
{
	for (const auto pipeline : shaderPipelines)
	{
		updateFloatPointerBySignature<float>(pipeline.second->signature, "Projection", &(cam->Projection[0][0]));
		updateFloatPointerBySignature<float>(pipeline.second->signature, "View", &(cam->View[0][0]));
	}
}

void RenderPass::setupVec4f(glm::vec4& input, std::string name)
{
	for (const auto pipeline : shaderPipelines)
	{
		updateFloatPointerBySignature<float>(pipeline.second->signature, name, &(input[0]));
	}
}

void RenderPass::setupVec2i(glm::ivec2& input, std::string name)
{
	for (const auto pipeline : shaderPipelines)
	{
		updateIntPointerBySignature<int>(pipeline.second->signature, name, &(input[0]));
	}
}

void RenderPass::setupFloat(float* input, std::string name)
{
	for (const auto pipeline : shaderPipelines)
	{
		updateFloatPointerBySignature<float>(pipeline.second->signature, name, input);
	}
}

void RenderPass::addNeighbor(Pass* neighbor, std::vector<std::string> passedFrameBufferSignatures)
{
	if (neighbor == nullptr)
	{
		return;
	}

	std::vector<DecoratedFrameBuffer*> passedFb;

	if (passedFrameBufferSignatures.size() == 0)
	{
		for (const auto& fb : frameBuffers)
		{
			passedFb.push_back(fb.second);
		}
	}
	else if (passedFrameBufferSignatures[0] != "NONE")
	{
		auto castNeighbor = dynamic_cast<RenderPass*>(neighbor);
		if (castNeighbor != nullptr)
		{
			for (const auto& fbSignature : passedFrameBufferSignatures)
			{
				auto fb = frameBuffers.find(fbSignature);
				if (fb != frameBuffers.end())
				{
					passedFb.push_back(fb->second);
				}
			}
		}
	}

	DirectedGraphNode<Pass>::addNeighbor(neighbor, new DirectedEdge(new RenderEdgeData(this, neighbor, passedFb)));
}

DecoratedFrameBuffer* RenderPass::getFrameBuffer(std::string signature)
{
	for (const auto& fbPair : frameBuffers)
	{
		auto frameBuffer = fbPair.second;
		auto foundFb = frameBuffer->signatureLookup(signature);

		if (foundFb != nullptr)
		{
			return foundFb;
		}
	}
}

GeometryPass::GeometryPass(std::unordered_map<std::string, ShaderProgramPipeline*> shaderPipelines,
			  std::string signature,
			  DecoratedFrameBuffer* frameBuffer,
			  int pickingBufferCount,
			  int stencilBufferCount,
			  bool terminal,
			  GLenum clearType) :
	pickingBufferCount(pickingBufferCount), stencilBufferCount(stencilBufferCount), clearType(clearType),
	RenderPass(shaderPipelines, signature, frameBuffer, terminal)
{
	initFrameBuffers();
}

void GeometryPass::initFrameBuffers(void)
{
	auto widthHeight = WindowContext::context->getSize();

	int width = widthHeight.first;
	int height = widthHeight.second;

	int attachmentNumber = 0;
	DecoratedFrameBuffer* lastBuffer = nullptr;

	lastBuffer = frameBuffers["COLORS"] =
		new ImageFrameBuffer(attachmentNumber++, 0, width, height, "COLORS", glm::vec4(), clearType);
	frameBuffers["NORMALS"] =
		new ImageFrameBuffer(attachmentNumber++, lastBuffer->FBO, width, height, "NORMALS", glm::vec4(), clearType);
	frameBuffers["POSITIONS"] =
		new ImageFrameBuffer(attachmentNumber++, lastBuffer->FBO, width, height, "POSITIONS", glm::vec4(), clearType);
	frameBuffers["ABS_POSITIONS"] =
		new ImageFrameBuffer(attachmentNumber++, lastBuffer->FBO, width, height, "ABS_POSITIONS", glm::vec4(), clearType);

	for (int i = 0; i < pickingBufferCount; ++i)
	{
		std::string key = "PICKING" + std::to_string(i);
		frameBuffers[key] = new PickingBuffer(attachmentNumber++, lastBuffer->FBO, width, height, key);
	}

	for (int i = 0; i < stencilBufferCount; ++i)
	{
		std::string key = "STENCIL" + std::to_string(i);
		frameBuffers[key] = new PickingBuffer(attachmentNumber++, lastBuffer->FBO, width, height, key);
	}
}

void GeometryPass::configureGL(const std::string& programSignature)
{
	if (!shaderPipelines[programSignature]->alphaRendered)
	{
		glDisable(GL_BLEND);
		glEnable(GL_DEPTH_TEST);
	}
	else
	{
		glEnable(GL_BLEND);
		glDisable(GL_DEPTH_TEST);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	}

	if (shaderPipelines[programSignature]->cullFace)
	{
		glPolygonMode(GL_FRONT, GL_FILL);
		glEnable(GL_CULL_FACE);
	}
	else
	{
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		glDisable(GL_CULL_FACE);
	}
}

void GeometryPass::setupObjectwiseUniforms(const std::string& programSignature, const std::string& signature)
{
	GLuint p = shaderPipelines[programSignature]->getProgramByEnum(GL_VERTEX_SHADER)->program;
	glProgramUniformMatrix4fv(p, glGetUniformLocation(p, "Model"), 1, GL_FALSE,
							  &(renderableObjects[programSignature][signature]->getModelMatrix()[0][0]));
}

void GeometryPass::setupOnHover(unsigned int id)
{
	for (const auto pipeline : shaderPipelines)
	{
		updateValueBySignature<unsigned int>(pipeline.second->signature, "selectedRef", id);
	}
}

IntermediatePass::IntermediatePass(std::unordered_map<std::string, ShaderProgramPipeline*> shaderPipelines,
								   int pickingBuffers,
								   const std::string& signature) :
	GeometryPass(shaderPipelines, signature, nullptr, pickingBuffers, 0, false, GL_COLOR_BUFFER_BIT)
{
	for (const auto& pipeline : renderableObjects)
	{
		addRenderableObjects(quad, "DISPLAYQUAD", pipeline.first);
	}
}

void IntermediatePass::configureGL(const std::string& programSignature)
{
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_BLEND);
	glEnable(GL_CULL_FACE);
	glPolygonMode(GL_FRONT, GL_FILL);
}

LightPass::LightPass(std::unordered_map<std::string, ShaderProgramPipeline*> shaderPipelines, bool terminal) :
	RenderPass(shaderPipelines, "LIGHTPASS", nullptr)
{
	if (terminal)
	{
		frameBuffers["DEFAULT"] = DefaultFrameBuffer::getInstance();
	}
	else
	{
		initFrameBuffers();
	}
	
	for (const auto& pipeline : renderableObjects)
	{
		addRenderableObjects(quad, "DISPLAYQUAD", pipeline.first);
	}
}

LightPass::LightPass(std::unordered_map<std::string, ShaderProgramPipeline*> shaderPipelines, DecoratedFrameBuffer* frameBuffer) :
	RenderPass(shaderPipelines, "LIGHTPASS", frameBuffer)
{
	for (const auto& pipeline : renderableObjects)
	{
		addRenderableObjects(quad, "DISPLAYQUAD", pipeline.first);
	}
}

void LightPass::initFrameBuffers(void)
{
	auto widthHeight = WindowContext::context->getSize();
	frameBuffers["MAINIMAGE"] = new ImageFrameBuffer(widthHeight.first, widthHeight.second, "MAINIMAGE", glm::vec4(), GL_COLOR_BUFFER_BIT);
}

void LightPass::configureGL(const std::string& programSignature)
{
	if (depthTest)
	{
		glEnable(GL_DEPTH_TEST);
		
	}
	else
	{
		glDisable(GL_DEPTH_TEST);
	}

	glDisable(GL_BLEND);
	glEnable(GL_CULL_FACE);
	glPolygonMode(GL_FRONT, GL_FILL);
}

void LightPass::setDepthTest(bool dT)
{
	depthTest = dT;
}