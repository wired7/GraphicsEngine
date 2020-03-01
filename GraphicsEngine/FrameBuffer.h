#pragma once
#include "Decorator.h"
#include <unordered_set>
#include <glew.h>
#include <glm.hpp>

class DecoratedFrameBuffer : public Decorator<DecoratedFrameBuffer>
{
protected:
	virtual void bindFBO(void);
	virtual void bindTexture(void) = 0;
	virtual void bindRBO(void);
	GLuint texture;
	GLuint RBO;
	GLenum type;
	GLenum clearType;
	int attachmentNumber = 0;
	int width;
	int height;
	glm::vec4 defaultColor;
public:
	GLuint FBO = 0;

	DecoratedFrameBuffer() {};
	DecoratedFrameBuffer(int width, int height, std::string signature, GLenum type, glm::vec4 defaultColor = glm::vec4(),
						 GLenum clearType = GL_COLOR_BUFFER_BIT);
	DecoratedFrameBuffer(DecoratedFrameBuffer* child, int width, int height, std::string signature, GLenum type,
						 glm::vec4 defaultColor = glm::vec4(), GLenum clearType = GL_COLOR_BUFFER_BIT);
	DecoratedFrameBuffer(int attachmentNumber, GLuint FBO, int width, int height, std::string signature, GLenum type,
						 glm::vec4 defaultColor = glm::vec4(), GLenum clearType = GL_COLOR_BUFFER_BIT);
	~DecoratedFrameBuffer() {};

	virtual DecoratedFrameBuffer* drawBuffers(std::vector<GLuint>& buff);
	virtual void postDrawBuffers(std::vector<GLuint>& buff);
	void drawBuffer(std::string signature);
	void drawBuffers(std::vector<std::string> signatures);

	virtual int bindTexturesForPass(int textureOffset = 0);

	DecoratedFrameBuffer* make(void) { return NULL; };
	std::string printOwnProperties(void);
};

// TODO : make singleton pattern
class DefaultFrameBuffer : public DecoratedFrameBuffer
{
private:
	static DefaultFrameBuffer* frameBuffer;
protected:
	void bindFBO(void);
	void bindTexture(void);
	void bindRBO(void);
	int bindTexturesForPass(int textureOffset = 0);
	DefaultFrameBuffer();
	~DefaultFrameBuffer() {};
public:
	static DefaultFrameBuffer* getInstance();
};

class ImageFrameBuffer : public DecoratedFrameBuffer
{
protected:
	virtual void bindTexture(void);
public:
	ImageFrameBuffer(int width, int height, std::string signature, glm::vec4 defaultColor = glm::vec4(), GLenum clearType = GL_COLOR_BUFFER_BIT);
	ImageFrameBuffer(DecoratedFrameBuffer* child, int width, int height, std::string signature, glm::vec4 defaultColor = glm::vec4(),
					 GLenum clearType = GL_COLOR_BUFFER_BIT);
	ImageFrameBuffer(int attachmentNumber, GLuint FBO, int width, int height, std::string signature, glm::vec4 defaultColor = glm::vec4(),
					 GLenum clearType = GL_COLOR_BUFFER_BIT);
	~ImageFrameBuffer() {};
};

class PickingBuffer : public DecoratedFrameBuffer
{
protected:
	virtual void bindTexture(void);
public:
	PickingBuffer(int width, int height, std::string signature);
	PickingBuffer(DecoratedFrameBuffer* child, int width, int height, std::string signature);
	PickingBuffer(int attachmentNumber, GLuint FBO, int width, int height, std::string signature);
	~PickingBuffer() {};

	GLuint* getValues(int x, int y, int sampleW = 1, int sampleH = 1);
};