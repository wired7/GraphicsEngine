#pragma once
#include "FrameBuffer.h"

DecoratedFrameBuffer::DecoratedFrameBuffer(int width, int height, std::string signature, GLenum type, glm::vec4 defaultColor,
										   GLenum clearType) :
	Decorator<DecoratedFrameBuffer>(nullptr, signature), width(width), height(height), type(type), defaultColor(defaultColor),
	clearType(clearType)
{
}

DecoratedFrameBuffer::DecoratedFrameBuffer(DecoratedFrameBuffer* child, int width, int height, std::string signature,
										   GLenum type, glm::vec4 defaultColor, GLenum clearType) :
	Decorator<DecoratedFrameBuffer>(child, signature), width(width), height(height), type(type), defaultColor(defaultColor), clearType(clearType)
{
	FBO = child->FBO;
	attachmentNumber = child->attachmentNumber + 1;
}

DecoratedFrameBuffer::DecoratedFrameBuffer(int attachmentNumber, GLuint FBO, int width, int height, std::string signature,
										   GLenum type, glm::vec4 defaultColor, GLenum clearType) :
	Decorator<DecoratedFrameBuffer>(nullptr, signature), width(width), height(height), type(type), defaultColor(defaultColor), clearType(clearType),
	attachmentNumber(attachmentNumber), FBO(FBO)
{
}

void DecoratedFrameBuffer::bindFBO()
{
	if (FBO == 0)
	{
		glGenFramebuffers(1, &FBO);
	}

	glBindFramebuffer(GL_FRAMEBUFFER, FBO);
}

void DecoratedFrameBuffer::bindTexture()
{

}

void DecoratedFrameBuffer::bindRBO()
{
	glGenRenderbuffers(1, &RBO);
	glBindRenderbuffer(GL_RENDERBUFFER, RBO);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height);
	glBindRenderbuffer(GL_RENDERBUFFER, 0);

	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, RBO);
}

DecoratedFrameBuffer* DecoratedFrameBuffer::drawBuffers(std::vector<GLenum>& buff)
{
	auto lastBuffer = this;
	DecoratedFrameBuffer* currentBuffer = this;

	do
	{
		int aNumber = currentBuffer->attachmentNumber;
//		std::cout << aNumber << " " << currentBuffer->FBO << std::endl;
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, currentBuffer->FBO);
		glClearColor(currentBuffer->defaultColor.r, currentBuffer->defaultColor.g, currentBuffer->defaultColor.b, currentBuffer->defaultColor.a);
		glClear(currentBuffer->clearType);
		buff[aNumber] = GL_COLOR_ATTACHMENT0 + aNumber;
		lastBuffer = currentBuffer;
		currentBuffer = currentBuffer->child;
	} while (currentBuffer != nullptr);

	return lastBuffer;
}

void DecoratedFrameBuffer::postDrawBuffers(std::vector<GLuint>& buff)
{
/*	for (const auto& b : buff)
	{
		std::cout << b << " ";
	}

	std::cout << std::endl;*/
	glDrawBuffers(buff.size(), &(buff[0]));
	glClearColor(defaultColor.r, defaultColor.g, defaultColor.b, defaultColor.a);
	glClear(clearType);
}

int DecoratedFrameBuffer::bindTexturesForPass(int textureOffset)
{
	DecoratedFrameBuffer* currentBuffer = this;
	int index = textureOffset;
	do
	{
//		std::cout << "\t\t" << index << " " << currentBuffer->signature << " " << currentBuffer->attachmentNumber << std::endl;
		glActiveTexture(GL_TEXTURE0 + index++);
		glBindTexture(currentBuffer->type, currentBuffer->texture);
		currentBuffer = currentBuffer->child;
	} while (currentBuffer != nullptr);

	return index;
}

std::string DecoratedFrameBuffer::printOwnProperties(void)
{
	return std::to_string(attachmentNumber) + "\n";
}

DefaultFrameBuffer* DefaultFrameBuffer::frameBuffer = nullptr;

DefaultFrameBuffer* DefaultFrameBuffer::getInstance()
{
	if(frameBuffer == nullptr)
	{
		frameBuffer = new DefaultFrameBuffer();
	}

	return frameBuffer;
}

DefaultFrameBuffer::DefaultFrameBuffer()
{
	FBO = 0;
	signature = "DEFAULT";
	defaultColor = glm::vec4(1.0f);
	clearType = GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT;
}

void DefaultFrameBuffer::bindFBO()
{
	glBindFramebuffer(GL_FRAMEBUFFER, FBO);
}

void DefaultFrameBuffer::bindTexture()
{

}

void DefaultFrameBuffer::bindRBO()
{

}

int DefaultFrameBuffer::bindTexturesForPass(int textureOffset)
{
	return 0;
}

ImageFrameBuffer::ImageFrameBuffer(int width, int height, std::string signature, glm::vec4 defaultColor, GLenum clearType) :
	DecoratedFrameBuffer(width, height, signature, GL_TEXTURE_2D, defaultColor)
{
	bindFBO();
	bindTexture();
	bindRBO();

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

ImageFrameBuffer::ImageFrameBuffer(DecoratedFrameBuffer* child, int width, int height, std::string signature, glm::vec4 defaultColor,
								   GLenum clearType) : DecoratedFrameBuffer(child, width, height, signature, GL_TEXTURE_2D, defaultColor)
{
	bindFBO();
	bindTexture();
	bindRBO();

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

ImageFrameBuffer::ImageFrameBuffer(int attachmentNumber, GLuint FBO, int width, int height, std::string signature, glm::vec4 defaultColor,
								   GLenum clearType) :
	DecoratedFrameBuffer(attachmentNumber, FBO, width, height, signature, GL_TEXTURE_2D, defaultColor)
{
	bindFBO();
	bindTexture();
	bindRBO();

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void ImageFrameBuffer::bindTexture()
{
	glGenTextures(1, &texture);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(type, texture);

	glTexImage2D(type, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
	glTexParameteri(type, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(type, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glBindTexture(type, 0);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + attachmentNumber, type, texture, 0);
}

PickingBuffer::PickingBuffer(DecoratedFrameBuffer* child, int width, int height, std::string signature) :
	DecoratedFrameBuffer(child, width, height, signature, GL_TEXTURE_2D, glm::vec4(0), GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT)
{
	bindFBO();
	bindTexture();
	bindRBO();

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

PickingBuffer::PickingBuffer(int width, int height, std::string signature) :
	DecoratedFrameBuffer(width, height, signature, GL_TEXTURE_2D, glm::vec4(0), GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT)
{
	bindFBO();
	bindTexture();
	bindRBO();

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

PickingBuffer::PickingBuffer(int attachmentNumber, GLuint FBO, int width, int height, std::string signature) :
	DecoratedFrameBuffer(attachmentNumber, FBO, width, height, signature, GL_TEXTURE_2D, glm::vec4(0), GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT)
{
	bindFBO();
	bindTexture();
	bindRBO();

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void PickingBuffer::bindTexture()
{
	glGenTextures(1, &texture);
	glBindTexture(type, texture);

	glTexImage2D(type, 0, GL_R32UI, width, height, 0, GL_RED_INTEGER, GL_UNSIGNED_INT, NULL);
	glTexParameteri(type, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(type, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	auto error = glGetError();

	if (error != GL_NO_ERROR)
		std::cout << error << std::endl;

	glBindTexture(type, 0);

	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + attachmentNumber, type, texture, 0);
}

GLuint* PickingBuffer::getValues(int x, int y, int sampleW, int sampleH)
{
	glBindFramebuffer(GL_READ_FRAMEBUFFER, FBO);
	glReadBuffer(GL_COLOR_ATTACHMENT0 + attachmentNumber);
	int length = 4 * width * height;

	GLuint* data = new GLuint[4];

	glReadPixels(x, y, sampleW, sampleH, GL_RGBA_INTEGER, GL_UNSIGNED_INT, data);

	glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);

	return data;
}