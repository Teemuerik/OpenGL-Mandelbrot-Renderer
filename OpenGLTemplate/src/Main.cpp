#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <vector>
#include <fstream>
#include <string>
#include <sstream>
#include <Windows.h>

#define EXPORT_AS_STANDALONE 1

#if EXPORT_AS_STANDALONE
#define USE_HARDCODED_SHADERS 1
#else
#define USE_HARDCODED_SHADERS 0
#endif

static std::vector<std::string> shaderFiles = {
	"vertex_shader.vert",
	"mandelbrot_fragment_shader.frag"
};

#if USE_HARDCODED_SHADERS

static std::string vertS = {
#include "../res/shaders/vertex_shader.vert"
};

static std::string mandelbrotFragS = {
#include "../res/shaders/mandelbrot_fragment_shader.frag"
};

#endif

static int CompileShader(const std::string source, const unsigned int type) {
	unsigned int id = glCreateShader(type);
	const char* src = source.c_str();
	glShaderSource(id, 1, &src, nullptr);
	glCompileShader(id);

	int success;
	glGetShaderiv(id, GL_COMPILE_STATUS, &success);
	if (!success) {
		int length;
		glGetShaderiv(id, GL_INFO_LOG_LENGTH, &length);
		char* message = (char*)alloca(length * sizeof(char));
		glGetShaderInfoLog(id, length, &length, message);
		std::cout << "Failed to compile ";
		switch (type) {
		case GL_VERTEX_SHADER: std::cout << "vertex "; break;
		case GL_FRAGMENT_SHADER: std::cout << "fragment "; break;
		}
		std::cout << "shader!" << std::endl;
		std::cout << message << std::endl;
		glDeleteShader(id);
		return 0;
	}
	return id;
}

static unsigned int ReadShaders(const std::vector<std::string> shaders) {
	unsigned int program = glCreateProgram();
#if USE_HARDCODED_SHADERS
	unsigned int vs = CompileShader(vertS, GL_VERTEX_SHADER);
	unsigned int fs = CompileShader(mandelbrotFragS, GL_FRAGMENT_SHADER);
	glAttachShader(program, vs);
	glAttachShader(program, fs);
#else
	std::vector<unsigned int> shaderIDs;
	for (unsigned int i = 0; i < shaders.size(); ++i) {
		std::string fileName = shaders[i];
		std::string filePath = "res/shaders/" + fileName;
		std::ifstream fileStream(filePath);
		std::stringstream sourceStream;
		std::string sourceLine;
		while (getline(fileStream, sourceLine)) {
			if (sourceLine != "R\"(" && sourceLine != ")\"") {
				sourceStream << sourceLine << '\n';
			}
		}
		if (fileName.find(".vert") != fileName.npos) {
			shaderIDs.push_back(CompileShader(sourceStream.str(), GL_VERTEX_SHADER));
		}
		else if (fileName.find(".frag") != fileName.npos) {
			shaderIDs.push_back(CompileShader(sourceStream.str(), GL_FRAGMENT_SHADER));
		}
	}
	for (unsigned int i = 0; i < shaderIDs.size(); ++i) {
		glAttachShader(program, shaderIDs[i]);
	}
#endif
	glLinkProgram(program);
	glValidateProgram(program);
#if USE_HARDCODED_SHADERS
	glDeleteShader(vs);
	glDeleteShader(fs);
#else
	for (unsigned int i = 0; i < shaderIDs.size(); ++i) {
		glDeleteShader(shaderIDs[i]);
	}
#endif
	return program;
}

// Stores the current program.
unsigned int currentProgram;

GLFWwindow* currentWindow;

struct Point {
	double X;
	double Y;
};

unsigned int zoomUniform;
double currentZoom = 0;
double currentZoomSpeed = 0;
unsigned int maxIterUniform;
unsigned int camPointXUniform;
unsigned int camPointYUniform;
Point camPoint = { 0, 0 };
Point targetPoint = camPoint;
double currentMoveSpeed;
int flags = 0;

class Flags {
public:
	static const int Started = 0x1;
	static const int Paused = 0x2;
};

double Width = 1332;
double Height = 740;

unsigned int maxIterations = 100;
float zoomSpeed = 1;
float moveSpeed = 1;

unsigned int GetProgram() { return currentProgram; }

void mouse_pressed(GLFWwindow* window, int button, int action, int mods) {
	if (action == GLFW_PRESS) {
		double xpos, ypos;
		glfwGetCursorPos(window, &xpos, &ypos);
		xpos = (xpos - Width / 2) / (Width / 2);
		ypos = ((Height - ypos) - Height / 2) / (Height / 2);
		xpos = (double)xpos / (double)pow(10, (float)currentZoom) + (double)camPoint.X;
		ypos = (double)ypos / (double)pow(10, (float)currentZoom) + (double)camPoint.Y;
		std::cout << "X: " << xpos << ", Y: " << ypos << "." << std::endl;
		targetPoint = { xpos, ypos };
		if (button == GLFW_MOUSE_BUTTON_LEFT) {
			currentZoomSpeed = zoomSpeed;
			currentMoveSpeed = moveSpeed;
		}
		else if (button == GLFW_MOUSE_BUTTON_RIGHT) {
			currentZoomSpeed = -zoomSpeed;
			currentMoveSpeed = 0;
		}
		flags |= Flags::Started;
	}
}

#ifdef _WIN64
void WriteDWORD(byte* a, DWORD b) {
	*a = *((byte*)(&b) + 3);
	*(a + 1) = *((byte*)(&b) + 2);
	*(a + 2) = *((byte*)(&b) + 1);
	*(a + 3) = *((byte*)(&b));
}
void WriteWORD(byte* a, WORD b) {
	*a = *((byte*)(&b) + 1);
	*(a + 1) = *((byte*)(&b));
}
#else
void WriteDWORD(byte* a, DWORD b) {
	*a = *((byte*)(&b));
	*(a + 1) = *((byte*)(&b) + 1);
	*(a + 2) = *((byte*)(&b) + 2);
	*(a + 3) = *((byte*)(&b) + 3);
}
void WriteWORD(byte* a, WORD b) {
	*a = *((byte*)(&b));
	*(a + 1) = *((byte*)(&b) + 1);
}
#endif

void WriteNULL(byte* a, size_t size) {
	for (size_t i = 0; i < size; ++i) {
		*(a + i) = NULL;
	}
}

void WriteToBMP(char* filePath, byte* pixelBuffer) {
	byte bytes[56];
	byte* start = &bytes[0];
	WriteWORD(start, *(unsigned short*)"BM");
}

void SavePixelData() {
	unsigned int byteCount = Width * Height * 3;
	byte* pixelData = new byte[byteCount];
	glReadPixels(0, 0, Width, Height, GL_RGB, GL_UNSIGNED_BYTE, pixelData);
	std::cout << "Read pixel data." << std::endl;
}

void key_pressed(GLFWwindow* window, int key, int scancode, int action, int mods) {
	if (key == GLFW_KEY_Z && action == GLFW_PRESS) {
		if (mods & GLFW_MOD_SHIFT) {
			if (mods & GLFW_MOD_CONTROL) {
				camPoint = { 0, 0 };
			}
			else {
				targetPoint = camPoint;
				camPoint = { 0, 0 };
			}
			currentZoom = 0;
			std::cout << "Zooming in on: " << targetPoint.X << ", " << targetPoint.Y << "." << std::endl;
		}
		else {
			std::cout << "The current zoom is: " << currentZoom << "." << std::endl;
		}
	}
	else if (key == GLFW_KEY_P && action == GLFW_PRESS) {
		if (flags & Flags::Started) {
			if (flags & Flags::Paused) {
				std::cout << "Resumed." << std::endl;
				currentZoomSpeed = zoomSpeed;
				currentMoveSpeed = moveSpeed;
				flags ^= Flags::Paused;
			}
			else {
				std::cout << "Paused." << std::endl;
				currentZoomSpeed = 0;
				currentMoveSpeed = 0;
				flags |= Flags::Paused;
			}
		}
		else std::cout << "Zoom cannot be resumed because no zoom point is specified." << std::endl;
	}
	else if (key == GLFW_KEY_S && action == GLFW_PRESS) {
		if (mods & GLFW_MOD_SHIFT) {
			//SavePixelData();
		}
		else {
			double x, y;
			std::cout << "Zoom point X: " << std::endl;
			std::cin >> x;
			std::cout << "Zoom point Y: " << std::endl;
			std::cin >> y;
			targetPoint.X = x;
			targetPoint.Y = y;
			camPoint = { 0, 0 };
			currentZoom = 0;
			if (~flags & Flags::Paused) {
				currentZoomSpeed = zoomSpeed;
				currentMoveSpeed = moveSpeed;
			}
			flags |= Flags::Started;
		}
	}
	else if (key == GLFW_KEY_KP_ADD && action == GLFW_PRESS) {
		if (mods & GLFW_MOD_SHIFT) {
			maxIterations += 10;
		}
		else {
			maxIterations += 1;
		}
	}
	else if (key == GLFW_KEY_KP_SUBTRACT && action == GLFW_PRESS) {
		if (mods & GLFW_MOD_SHIFT) {
			if (maxIterations > 9) maxIterations -= 10;
			else maxIterations = 0;
		}
		else {
			if (maxIterations > 0) maxIterations -= 1;
		}
	}
}

void RenderLoop() {
	// Loop until the user closes the window
	while (!glfwWindowShouldClose(currentWindow))
	{
		// Render here
		glClear(GL_COLOR_BUFFER_BIT);

		currentZoom += currentZoomSpeed / 1000;
		camPoint.X += (targetPoint.X - camPoint.X) / 100 * currentMoveSpeed;
		camPoint.Y += (targetPoint.Y - camPoint.Y) / 100 * currentMoveSpeed;
		glUniform1d(zoomUniform, currentZoom);
		glUniform1i(maxIterUniform, maxIterations);
		glUniform1d(camPointXUniform, camPoint.X);
		glUniform1d(camPointYUniform, camPoint.Y);

		glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

		// Swap front and back buffers
		glfwSwapBuffers(currentWindow);

		// Poll for and process events
		glfwPollEvents();

		Sleep(10);
	}

	glfwTerminate();
}

int main()
{
	/* Initialize the library */
	if (!glfwInit())
		return -1;

	/* Create a windowed mode window and its OpenGL context */
	currentWindow = glfwCreateWindow(Width, Height, "Mandelbrot", NULL, NULL);
	if (!currentWindow)
	{
		glfwTerminate();
		return -1;
	}

	/* Make the window's context current */
	glfwMakeContextCurrent(currentWindow);

	if (glewInit() != GLEW_OK)
		std::cout << "Error initializing GLEW!" << std::endl;

	// Create a program that runs on the GPU. */
	unsigned int program = ReadShaders(shaderFiles);
	glUseProgram(program);

	float vertices[8] = {
		-1, -1,
		1, -1,
		1, 1,
		-1, 1
	};

	unsigned int indices[6] = {
		0, 1, 2,
		0, 2, 3
	};

	unsigned int vertexBuffer;
	glGenBuffers(1, &vertexBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
	glBufferData(GL_ARRAY_BUFFER, 8 * sizeof(float), vertices, GL_STATIC_DRAW);

	unsigned int indexBuffer;
	glGenBuffers(1, &indexBuffer);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBuffer);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, 6 * sizeof(unsigned int), indices, GL_STATIC_DRAW);

	unsigned int posAttrib = glGetAttribLocation(program, "position");
	glEnableVertexAttribArray(posAttrib);
	glVertexAttribPointer(posAttrib, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), 0);

	zoomUniform = glGetUniformLocation(program, "zoomLevel");
	maxIterUniform = glGetUniformLocation(program, "MaxIterations");
	camPointXUniform = glGetUniformLocation(program, "camPointX");
	camPointYUniform = glGetUniformLocation(program, "camPointY");

	// Set the current program.
	currentProgram = program;

	glfwSetMouseButtonCallback(currentWindow, mouse_pressed);
	glfwSetKeyCallback(currentWindow, key_pressed);

	RenderLoop();

	// Exit.
	return 0;
}