//
//  main.cpp
//  OpenGL Advances Lighting
//
//  Created by CGIS on 28/11/16.
//  Copyright � 2016 CGIS. All rights reserved.
//

#define GLEW_STATIC
#define DUCK_NO 15
#define DROPLET_NO 6500
#define POINTLIGHT_NO 4
#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/matrix_inverse.hpp"
#include "glm/gtc/type_ptr.hpp"
#include  "glm/gtx/vector_angle.hpp"

#include "Shader.hpp"
#include "Model3D.hpp"
#include "Camera.hpp"
#include "SkyBox.hpp"

#include <iostream>
#include <random>
#include <string>
#include <time.h>

//structures
struct bezierCurve {
	glm::vec3 p0;
	glm::vec3 p1;
	glm::vec3 p2;
	glm::vec3 p3;
};

struct pointLight {
	glm::vec3 position;
	float constant, linear, quadratic;
	glm::vec3 color;
};

struct spotLight {
	glm::vec3 position;
	glm::vec3 direction;
	float cutOff;
	float outerCutOff;
};

struct rainDrop{
	glm::vec3 position;
	float speed;
	int moveCounter;
};

//camera
gps::Camera myCamera(
	glm::vec3(-6.887457f, 4.196992f, 16.786829f),
	glm::vec3(-6.350887f, 3.997624f, 15.966863f),
	glm::vec3(0.0f, 1.0f, 0.0f));
float cameraSpeed = 0.075f;

//window info
int glWindowWidth = 1920;
int glWindowHeight = 1080;
int retina_width, retina_height;
GLFWwindow* glWindow = NULL;

//shadows
const unsigned int SHADOW_WIDTH = 2048;
const unsigned int SHADOW_HEIGHT = 2048;
const GLfloat near_plane = 0.1f, far_plane = 50.0f;
bool showDepthMap;

bool pressedKeys[1024];
bool startRain = false;
bool cameraPreview = false;

clock_t start;
clock_t end;

//matrices
glm::mat4 model;
GLuint modelLoc;
glm::mat4 view;
GLuint viewLoc;
glm::mat4 projection;
GLuint projectionLoc;
glm::mat3 normalMatrix;
GLuint normalMatrixLoc;

//directional light
glm::vec3 lightDir;
GLuint lightDirLoc;
glm::vec3 lightColor;
GLuint lightColorLoc;
glm::mat4 lightRotation;

//point light
pointLight pointLights[POINTLIGHT_NO];
spotLight mySpotLight;

// angles
GLfloat angle = 0.0f;
GLfloat lightAngle;
GLfloat bridgeAngle = 0.0f;
GLfloat millAngle = 0.0f;
GLfloat gateAngle = 0.0f;
GLfloat t=0.01f; //bezier

GLfloat fogFactor = 0.005f;
GLuint fogFactorLoc;

//objects
gps::SkyBox mySkyBox;
gps::Model3D blenderScene;
gps::Model3D trees;
gps::Model3D river;
gps::Model3D lightCube;
gps::Model3D castleBridge;
gps::Model3D mill;
gps::Model3D gate[3];
gps::Model3D tree;
gps::Model3D monument;
gps::Model3D dragon;
gps::Model3D duck;
gps::Model3D droplet;
gps::Model3D ducks[DUCK_NO];
gps::Model3D droplets[DROPLET_NO];

//vectors
std::vector<bezierCurve> curves;
std::vector<rainDrop> rainDrops;
std::vector<const GLchar*> faces;

//shaders
gps::Shader myCustomShader;
gps::Shader lightShader;
gps::Shader screenQuadShader;
gps::Shader depthMapShader;
gps::Shader skyboxShader;

GLuint shadowMapFBO;
GLuint depthMapTexture;
GLuint textureID;

//mouse input
bool firstMouseInput = true;
float lastX = 0;
float lastY = 0;
float yaw = 0.0f;
float pitch = 0;

//bezier curve limits
float yDuckMin = 0.75751f;
float yDuckMax = 5.7896f;
float xDuckMin = 6.8781f;
float xDuckMax = 13.129f;

//rain limits
float xRainMax = 6.334f;
float xRainMin = -3.91f;

float yRainMax = 19.2f;
float yRainMin = 14.3f;

float zRainMin = 6.679f;
float zRainMax = 14.88f;

GLenum glCheckError_(const char *file, int line) {
	GLenum errorCode;
	while ((errorCode = glGetError()) != GL_NO_ERROR)
	{
		std::string error;
		switch (errorCode)
		{
		case GL_INVALID_ENUM:                  error = "INVALID_ENUM"; break;
		case GL_INVALID_VALUE:                 error = "INVALID_VALUE"; break;
		case GL_INVALID_OPERATION:             error = "INVALID_OPERATION"; break;
		case GL_STACK_OVERFLOW:                error = "STACK_OVERFLOW"; break;
		case GL_STACK_UNDERFLOW:               error = "STACK_UNDERFLOW"; break;
		case GL_OUT_OF_MEMORY:                 error = "OUT_OF_MEMORY"; break;
		case GL_INVALID_FRAMEBUFFER_OPERATION: error = "INVALID_FRAMEBUFFER_OPERATION"; break;
		}
		std::cout << error << " | " << file << " (" << line << ")" << std::endl;
	}
	return errorCode;
}
#define glCheckError() glCheckError_(__FILE__, __LINE__)

void windowResizeCallback(GLFWwindow* window, int width, int height) {
	fprintf(stdout, "window resized to width: %d , and height: %d\n", width, height);
}

//generate random float between min and max
float generateBetween(float min, float max) {
	std::random_device rd;
	std::mt19937 gen(rd());
	std::uniform_real_distribution<> dist(min, max);
	return dist(gen);
}

//bezier functions
glm::vec3 getBezierPoint(GLfloat t, bezierCurve curve) {
	return  (1 - t) * (1 - t) * (1 - t) * curve.p0 +
		3 * t * (1 - t) * (1 - t) * curve.p1 +
		3 * t * t * (1 - t) * curve.p2 +
		t * t * t * curve.p3;
}

glm::vec3 getBezierDirectionVector(GLfloat t, bezierCurve curve) {
	return  (1 - t) * (1 - t) * (curve.p1 - curve.p0) +
		2 * t * (1 - t) * (curve.p2 - curve.p1) +
		t * t * (curve.p3 - curve.p2);
}

bezierCurve getRandomBezierCurve() {

	std::vector<glm::vec3> points;

	points.push_back(glm::vec3(generateBetween(xDuckMin, xDuckMax), 0.51911f, generateBetween(yDuckMin, yDuckMax)));
	points.push_back(glm::vec3(generateBetween(xDuckMin, xDuckMax), 0.51911f, generateBetween(yDuckMin, yDuckMax)));
	points.push_back(glm::vec3(generateBetween(xDuckMin, xDuckMax), 0.51911f, generateBetween(yDuckMin, yDuckMax)));
	points.push_back(glm::vec3(generateBetween(xDuckMin, xDuckMax), 0.51911f, generateBetween(yDuckMin, yDuckMax)));
	//std::random_shuffle(points.begin(), points.end());
	bezierCurve curve;
	curve.p0 = points.at(0);
	curve.p1 = points.at(1);
	curve.p2 = points.at(3);
	curve.p3 = points.at(2);
	return curve;
}

void changeBezierExtremities(bezierCurve* curve) {
	glm::vec3 tmpPoint = curve->p0;
	curve->p0 = curve->p3;
	curve->p3 = tmpPoint;
	//tmpPoint = curve->p1;
	//curve->p1 = curve->p2;
	//curve->p2 = tmpPoint;
}

//movement 
void bridgeMovement(float step) {
	if (step > 0) {
		if (bridgeAngle < 0 && bridgeAngle >= -90.0f)
			bridgeAngle += step;
	}
	else {
		if (bridgeAngle <= 0 && bridgeAngle > -90.0f)
			bridgeAngle += step;
	}
}

void gateMovement(float step) {
	if (step > 0) {
		if (gateAngle < 0 && gateAngle >= -90.0f)
			gateAngle += step;
	}
	else {
		if (gateAngle <= 0 && gateAngle > -90.0f)
			gateAngle += step;
	}
}

void rainMovement() {
	for (int i = 0; i < DROPLET_NO; i++) {
		rainDrops.at(i).moveCounter++;
		if (rainDrops.at(i).position.y - rainDrops.at(i).moveCounter * rainDrops.at(i).speed < 0.0f)
			rainDrops.at(i).moveCounter = 0;
	}
}

void duckMovement(float step) {
	if (t < 1) {
		t += step;
	}
	else {
		for (int i = 0; i < DUCK_NO; i++)
			changeBezierExtremities(&(curves.at(i)));
		t = 0;
	}
}

//callback
void keyboardCallback(GLFWwindow* window, int key, int scancode, int action, int mode) {
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
		glfwSetWindowShouldClose(window, GL_TRUE);

	if (key == GLFW_KEY_M && action == GLFW_PRESS)
		showDepthMap = !showDepthMap;

	if (key >= 0 && key < 1024)
	{
		if (action == GLFW_PRESS)
			pressedKeys[key] = true;
		else if (action == GLFW_RELEASE)
			pressedKeys[key] = false;
	}
}

void mouseCallback(GLFWwindow* window, double xpos, double ypos) {

	if (firstMouseInput)
	{
		lastX = xpos;
		lastY = ypos;
		firstMouseInput = false;
	}

	float xoffset = xpos - lastX;
	float yoffset = lastY - ypos;
	lastX = xpos;
	lastY = ypos;

	const float sensitivity = 0.05f;
	xoffset *= sensitivity;
	yoffset *= sensitivity;

	yaw += xoffset;
	pitch += yoffset;

	if (pitch > 89.0f)
		pitch = 89.0f;
	if (pitch < -89.0f)
		pitch = -89.0f;
	myCamera.rotate(pitch, yaw);
	//printf("mouse moved %f %f\n",yaw,pitch);
	view = myCamera.getViewMatrix();
	myCustomShader.useShaderProgram();
	glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
	// compute normal matrix for teapot
	normalMatrix = glm::mat3(glm::inverseTranspose(view * model));
}

void processMovement()
{
	//solid view
	if (pressedKeys[GLFW_KEY_1]) {
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	}

	// wireframe view
	if (pressedKeys[GLFW_KEY_2]) {
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	}

	// point view
	if (pressedKeys[GLFW_KEY_3]) {
		glPolygonMode(GL_FRONT_AND_BACK, GL_POINT);
	}

	//turn pointlights off
	if (pressedKeys[GLFW_KEY_4]) {
		myCustomShader.useShaderProgram();
		glUniform1f(glGetUniformLocation(myCustomShader.shaderProgram, "pointFlag"), 0.0f);
	}
	//turn pointlights on
	if (pressedKeys[GLFW_KEY_5]) {
		myCustomShader.useShaderProgram();
		glUniform1f(glGetUniformLocation(myCustomShader.shaderProgram, "pointFlag"), 1.0f);
	}

	//turn spotlight off
	if (pressedKeys[GLFW_KEY_6]) {
		myCustomShader.useShaderProgram();
		glUniform1f(glGetUniformLocation(myCustomShader.shaderProgram, "spotFlag"), 0.0f);
	}

	//turn spotlight on
	if (pressedKeys[GLFW_KEY_7]) {
		myCustomShader.useShaderProgram();
		glUniform1f(glGetUniformLocation(myCustomShader.shaderProgram, "spotFlag"), 1.0f);
	}
	if (pressedKeys[GLFW_KEY_9]) {
		cameraPreview = false;
	}
	if (pressedKeys[GLFW_KEY_0]) {
		cameraPreview = true;
	}
	//camera move forward
	if (pressedKeys[GLFW_KEY_W]) {
		myCamera.move(gps::MOVE_FORWARD, cameraSpeed);
		//update view matrix
		view = myCamera.getViewMatrix();
		myCustomShader.useShaderProgram();
		glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
		// compute normal matrix for teapot
		normalMatrix = glm::mat3(glm::inverseTranspose(view * model));
	}

	//camera move backwards
	if (pressedKeys[GLFW_KEY_S]) {
		myCamera.move(gps::MOVE_BACKWARD, cameraSpeed);
		//update view matrix
		view = myCamera.getViewMatrix();
		myCustomShader.useShaderProgram();
		glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
		// compute normal matrix for teapot
		normalMatrix = glm::mat3(glm::inverseTranspose(view * model));
	}

	//camera move left
	if (pressedKeys[GLFW_KEY_A]) {
		myCamera.move(gps::MOVE_LEFT, cameraSpeed);
		//update view matrix
		view = myCamera.getViewMatrix();
		myCustomShader.useShaderProgram();
		glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
		// compute normal matrix for teapot
		normalMatrix = glm::mat3(glm::inverseTranspose(view * model));
	}

	//camera move right
	if (pressedKeys[GLFW_KEY_D]) {
		myCamera.move(gps::MOVE_RIGHT, cameraSpeed);
		//update view matrix
		view = myCamera.getViewMatrix();
		myCustomShader.useShaderProgram();
		glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
		// compute normal matrix for teapot
		normalMatrix = glm::mat3(glm::inverseTranspose(view * model));
	}

	//camera move up
	if (pressedKeys[GLFW_KEY_SPACE]) {
		myCamera.move(gps::MOVE_UP, cameraSpeed);
		//update view matrix
		view = myCamera.getViewMatrix();
		myCustomShader.useShaderProgram();
		glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
		// compute normal matrix for teapot
		normalMatrix = glm::mat3(glm::inverseTranspose(view * model));
	}

	//camera move down
	if (pressedKeys[GLFW_KEY_LEFT_SHIFT]) {
		myCamera.move(gps::MOVE_DOWN, cameraSpeed);
		//update view matrix
		view = myCamera.getViewMatrix();
		myCustomShader.useShaderProgram();
		glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
		// compute normal matrix for teapot
		normalMatrix = glm::mat3(glm::inverseTranspose(view * model));
	}

	//camera rotate right
	if (pressedKeys[GLFW_KEY_E]) {
		yaw += 0.5f;
		myCamera.rotate(pitch,yaw);
		//update view matrix
		view = myCamera.getViewMatrix();
		myCustomShader.useShaderProgram();
		glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
		// compute normal matrix for teapot
		normalMatrix = glm::mat3(glm::inverseTranspose(view * model));
	}

	//camera rotate left
	if (pressedKeys[GLFW_KEY_Q]) {
		yaw -= 0.5f;
		myCamera.rotate(pitch, yaw);
		//update view matrix
		view = myCamera.getViewMatrix();
		myCustomShader.useShaderProgram();
		glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
		// compute normal matrix for teapot
		normalMatrix = glm::mat3(glm::inverseTranspose(view * model));
	}

	//open bridge
	if (pressedKeys[GLFW_KEY_B]) {
		bridgeMovement(1.0f);
	}

	//close bridge
	if (pressedKeys[GLFW_KEY_N]) {
		bridgeMovement(-1.0f);
	}

	//move directional light left
	if (pressedKeys[GLFW_KEY_K]) {
		lightAngle -= 0.1f;
	}

	//move directional light right
	if (pressedKeys[GLFW_KEY_L]) {
		lightAngle += 0.1f;
	}


	//close gates
	if (pressedKeys[GLFW_KEY_H]) {
		gateMovement(1.0f);
	}

	//open gates
	if (pressedKeys[GLFW_KEY_J]) {
		gateMovement(-1.0f);
	}

	//increase fog factor
	if (pressedKeys[GLFW_KEY_F]) {
		fogFactor += 0.001f;
	}

	//decrease fog factor
	if (pressedKeys[GLFW_KEY_G]) {
		if(fogFactor > 0)
			fogFactor -= 0.001f;
	}

	//enable rain
	if (pressedKeys[GLFW_KEY_R]) {
		startRain = true;
	}

	//disable rain
	if (pressedKeys[GLFW_KEY_T]) {
		startRain = false;
		for (int i = 0; i < DROPLET_NO; i++)
			rainDrops.at(i).moveCounter = 0;
	}
}

void initBezierCurves() {
	for (int i = 0; i < DUCK_NO; i++) {
		curves.push_back(getRandomBezierCurve());
	}
}

void initDroplets() {
	for (int i = 0; i < DROPLET_NO; i++) {
		rainDrop tmp;
		tmp.position = glm::vec3(generateBetween(xRainMin, xRainMax), generateBetween(yRainMin, yRainMin), generateBetween(zRainMin, zRainMax));
		tmp.speed = generateBetween(0.03f, 0.10f);
		tmp.moveCounter = 0;
		rainDrops.push_back(tmp);
	}
}

bool initOpenGLWindow()
{
	if (!glfwInit()) {
		fprintf(stderr, "ERROR: could not start GLFW3\n");
		return false;
	}

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_SRGB_CAPABLE, GLFW_TRUE);
	glfwWindowHint(GLFW_SAMPLES, 4);

	glWindow = glfwCreateWindow(glWindowWidth, glWindowHeight, "OpenGL Shader Example", NULL, NULL);
	if (!glWindow) {
		fprintf(stderr, "ERROR: could not open window with GLFW3\n");
		glfwTerminate();
		return false;
	}

	glfwSetWindowSizeCallback(glWindow, windowResizeCallback);
	glfwSetKeyCallback(glWindow, keyboardCallback);
	glfwSetCursorPosCallback(glWindow, mouseCallback);

	glfwMakeContextCurrent(glWindow);

	glfwSwapInterval(1);

	// start GLEW extension handler
	glewExperimental = GL_TRUE;
	glewInit();

	// get version info
	const GLubyte* renderer = glGetString(GL_RENDERER); // get renderer string
	const GLubyte* version = glGetString(GL_VERSION); // version as a string
	printf("Renderer: %s\n", renderer);
	printf("OpenGL version supported %s\n", version);

	//for RETINA display
	glfwGetFramebufferSize(glWindow, &retina_width, &retina_height);

	return true;
}

void initOpenGLState()
{
	glClearColor(0.3, 0.3, 0.3, 1.0);
	glViewport(0, 0, retina_width, retina_height);

	glEnable(GL_DEPTH_TEST); // enable depth-testing
	glDepthFunc(GL_LESS); // depth-testing interprets a smaller value as "closer"
	glEnable(GL_CULL_FACE); // cull face
	glCullFace(GL_BACK); // cull back face
	glFrontFace(GL_CCW); // GL_CCW for counter clock-wise
	glfwSetInputMode(glWindow, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
	glEnable(GL_FRAMEBUFFER_SRGB);
}

void initSkybox() {

	glGenTextures(1, &textureID);
	glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);

	faces.push_back("skybox/posx.jpg");
	faces.push_back("skybox/negx.jpg");
	faces.push_back("skybox/posy.jpg"); 
	faces.push_back("skybox/negy.jpg"); 
	faces.push_back("skybox/posz.jpg"); 
	faces.push_back("skybox/negz.jpg");

	mySkyBox.Load(faces);
	skyboxShader.loadShader("shaders/skyboxShader.vert", "shaders/skyboxShader.frag");
	skyboxShader.useShaderProgram();
	view = myCamera.getViewMatrix();
	glUniformMatrix4fv(glGetUniformLocation(skyboxShader.shaderProgram, "view"), 1, GL_FALSE, glm::value_ptr(view));
	projection = glm::perspective(glm::radians(45.0f), (float)retina_width / (float)retina_height, 0.1f, 1000.0f); 
	glUniformMatrix4fv(glGetUniformLocation(skyboxShader.shaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(projection));

}

void initObjects() {
	blenderScene.LoadModel("objects/scene.obj");
	lightCube.LoadModel("objects/cube/cube.obj");
	castleBridge.LoadModel("objects/castle_bridge.obj");
	mill.LoadModel("objects/mill.obj");
	gate[0].LoadModel("objects/gate1.obj");
	gate[1].LoadModel("objects/gate2.obj");
	gate[2].LoadModel("objects/gate3.obj");
	monument.LoadModel("objects/monument.obj");	
	duck.LoadModel("objects/duck.obj");
	droplet.LoadModel("objects/rain.obj");
	river.LoadModel("objects/river.obj");
	trees.LoadModel("objects/trees.obj");
	for (int i = 0; i < DUCK_NO; i++) {
		ducks[i] = duck;
	}
	for (int i = 0; i < DROPLET_NO; i++) {
		droplets[i] = droplet;
	}
}

void initShaders() {
	myCustomShader.loadShader("shaders/shaderStart.vert", "shaders/shaderStart.frag");
	myCustomShader.useShaderProgram();
	lightShader.loadShader("shaders/lightCube.vert", "shaders/lightCube.frag");
	lightShader.useShaderProgram();
	depthMapShader.loadShader("shaders/depthMapShader.vert", "shaders/depthMapShader.frag");
	depthMapShader.useShaderProgram();
}

void sendPointLight(int index) {
	char tmp[5][40];
	sprintf_s(tmp[0], 40, "pointLights[%d].position", index);
	sprintf_s(tmp[1], 40, "pointLights[%d].constant", index);
	sprintf_s(tmp[2], 40, "pointLights[%d].linear", index);
	sprintf_s(tmp[3], 40, "pointLights[%d].quadratic", index);
	sprintf_s(tmp[4], 40, "pointLights[%d].color", index);
	myCustomShader.useShaderProgram();
	glUniform3fv(glGetUniformLocation(myCustomShader.shaderProgram, tmp[0]), 1, glm::value_ptr(pointLights[index].position));
	glUniform1f(glGetUniformLocation(myCustomShader.shaderProgram, tmp[1]), pointLights[index].constant);
	glUniform1f(glGetUniformLocation(myCustomShader.shaderProgram, tmp[2]), pointLights[index].linear);
	glUniform1f(glGetUniformLocation(myCustomShader.shaderProgram, tmp[3]), pointLights[index].quadratic);
	glUniform3fv(glGetUniformLocation(myCustomShader.shaderProgram, tmp[4]), 1, glm::value_ptr(pointLights[index].color));
}

void initPointlights() {
	pointLight tmp;
	tmp.position = glm::vec3(-5.8837f, 3.4141f, 4.008f);
	tmp.color = glm::vec3(0.9f, 0.35f, 0.0f);
	tmp.constant = 1.0f;
	tmp.linear = 0.7f;
	tmp.quadratic = 1.8f;
	pointLights[0] = tmp;
	tmp.position = glm::vec3(-9.0f, 3.4141f, -3.4834f);
	pointLights[1] = tmp;
	tmp.position = glm::vec3(-1.7211f, 3.4141f, -6.4655f);
	pointLights[2] = tmp;
	tmp.position = glm::vec3(1.369f, 3.4141f, 0.8045f);
	pointLights[3] = tmp;
}

void initSpotLight() {
	glm::vec3 spotTarget = glm::vec3(5.691f, 1.194f, 9.917f);
	mySpotLight.position = glm::vec3(8.9646f, 8.4844f, 18.235f);
	mySpotLight.direction = -mySpotLight.position + spotTarget;
	mySpotLight.cutOff = glm::cos(glm::radians(7.0f));
	mySpotLight.outerCutOff = glm::cos(glm::radians(8.f));
}

void initUniforms() {

	myCustomShader.useShaderProgram();

	fogFactorLoc = glGetUniformLocation(myCustomShader.shaderProgram, "fogDensity");
	glUniform1f(fogFactorLoc, fogFactor);

	glUniform1f(glGetUniformLocation(myCustomShader.shaderProgram, "pointFlag") , 0.0f);
	glUniform1f(glGetUniformLocation(myCustomShader.shaderProgram, "spotFlag"), 0.0f);
	glUniform1f(glGetUniformLocation(myCustomShader.shaderProgram, "transparentFlag"), 0.0f);
	glUniform1f(glGetUniformLocation(myCustomShader.shaderProgram, "reflectiveFlag"), 0.0f);

	model = glm::mat4(1.0f);
	modelLoc = glGetUniformLocation(myCustomShader.shaderProgram, "model");
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

	view = myCamera.getViewMatrix();
	viewLoc = glGetUniformLocation(myCustomShader.shaderProgram, "view");
	glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
	
	normalMatrix = glm::mat3(glm::inverseTranspose(view*model));
	normalMatrixLoc = glGetUniformLocation(myCustomShader.shaderProgram, "normalMatrix");
	glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(normalMatrix));
	
	projection = glm::perspective(glm::radians(45.0f), (float)retina_width / (float)retina_height, 0.1f, 1000.0f);
	projectionLoc = glGetUniformLocation(myCustomShader.shaderProgram, "projection");
	glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, glm::value_ptr(projection));

	//set the light direction (direction towards the light)
	lightDir = glm::vec3(1.1f, 1.68f, 13.61f);
	lightRotation = glm::rotate(glm::mat4(1.0f), glm::radians(lightAngle), glm::vec3(0, 1, 0));
	lightDirLoc = glGetUniformLocation(myCustomShader.shaderProgram, "lightDir");	
	glUniform3fv(lightDirLoc, 1, glm::value_ptr(glm::inverseTranspose(glm::mat3(view * lightRotation)) * lightDir));

	//set light color
	lightColor = glm::vec3(1.0f, 1.0f, 1.0f); //white light
	lightColorLoc = glGetUniformLocation(myCustomShader.shaderProgram, "lightColor");
	glUniform3fv(lightColorLoc, 1, glm::value_ptr(lightColor));

	//set pointlights
	initPointlights();
	for (int i = 0; i < POINTLIGHT_NO; i++) {
		sendPointLight(i);
	}

	//set spotlight
	initSpotLight();

	glUniform3fv(glGetUniformLocation(myCustomShader.shaderProgram, "mySpotLight.position"), 1, glm::value_ptr(mySpotLight.position));
	glUniform3fv(glGetUniformLocation(myCustomShader.shaderProgram, "mySpotLight.direction"), 1, glm::value_ptr(mySpotLight.direction));
	glUniform1f(glGetUniformLocation(myCustomShader.shaderProgram, "mySpotLight.cutOff"), mySpotLight.cutOff);
	glUniform1f(glGetUniformLocation(myCustomShader.shaderProgram, "mySpotLight.outerCutOff"), mySpotLight.outerCutOff);

	lightShader.useShaderProgram();
	glUniformMatrix4fv(glGetUniformLocation(lightShader.shaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
}

void initFBO() {
	//generate FBO ID
	glGenFramebuffers(1, &shadowMapFBO);
	//create depth texture for FBO 
	glGenTextures(1, &depthMapTexture);
	glBindTexture(GL_TEXTURE_2D, depthMapTexture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, SHADOW_WIDTH, SHADOW_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	float borderColor[] = { 1.0f, 1.0f, 1.0f, 1.0f };
	glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
	//attach texture to FBO
	glBindFramebuffer(GL_FRAMEBUFFER, shadowMapFBO);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthMapTexture, 0);
	glDrawBuffer(GL_NONE);
	glReadBuffer(GL_NONE);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

glm::mat4 computeLightSpaceTrMatrix() {
	glm::vec3 lightDirAux = glm::mat3(glm::inverseTranspose(lightRotation)) * lightDir;
	glm::mat4 lightView = glm::lookAt(lightDirAux, glm::vec3(0.079f, 0.57f, 10.471f), glm::vec3(0, 1, 0));
	glm::mat4 lightProjection = glm::ortho(-10.0f, 10.0f, -1.0f, 1.0f, near_plane, far_plane);
	glm::mat4 lightSpaceTrMatrix = lightProjection * lightView;
	return lightSpaceTrMatrix;
}

void drawDuck(gps::Shader shader, bool depthPass, int duckIndex) {
	shader.useShaderProgram();
	glm::mat4 modelAux = model;
	modelAux = glm::translate(modelAux, getBezierPoint(t, curves.at(duckIndex)));
	glm::vec3 directionVector = glm::normalize(-getBezierDirectionVector(t, curves.at(duckIndex)));
	float bezierAngle = glm::atan(directionVector.z, directionVector.x);
	bezierAngle = (bezierAngle * 180) / 3.14;
	modelAux = glm::rotate(modelAux, glm::radians(270.f - bezierAngle), glm::vec3(0, 1, 0));
	glUniformMatrix4fv(glGetUniformLocation(shader.shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(modelAux));
	if (!depthPass) {
		normalMatrix = glm::mat3(glm::inverseTranspose(view * modelAux));
		glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(glm::mat3(glm::inverseTranspose(normalMatrix))));
	}
	ducks[duckIndex].Draw(shader);
}

void drawDroplet(gps::Shader shader, bool depthPass, int dropletIndex) {
	shader.useShaderProgram();
	glm::mat4 modelAux = model;
	rainDrop tmpRainDrop = rainDrops.at(dropletIndex);
	glm::vec3 tmpPoint = tmpRainDrop.position;
	modelAux = glm::translate(modelAux, glm::vec3(tmpPoint.x,tmpPoint.y - tmpRainDrop.moveCounter * tmpRainDrop.speed,tmpPoint.z));
	glUniformMatrix4fv(glGetUniformLocation(shader.shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(modelAux));
	if (!depthPass) {
		normalMatrix = glm::mat3(glm::inverseTranspose(view * modelAux));
		glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(glm::mat3(glm::inverseTranspose(normalMatrix))));
	}
	droplets[dropletIndex].Draw(shader);
}

void drawObjects(gps::Shader shader, bool depthPass) {
		
	//draw Blender scene
	shader.useShaderProgram();
	
	model = glm::mat4(1.0f);
	//model = glm::rotate(glm::mat4(1.0f), glm::radians(angleY), glm::vec3(0.0f, 1.0f, 0.0f));
	glUniformMatrix4fv(glGetUniformLocation(shader.shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));

	// do not send the normal matrix if we are rendering in the depth map
	if (!depthPass) {
		normalMatrix = glm::mat3(glm::inverseTranspose(view * model));
		glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(normalMatrix));
	}
	blenderScene.Draw(shader);

	//draw trees
	glDisable(GL_CULL_FACE);
	trees.Draw(shader);
	glEnable(GL_CULL_FACE);

	//draw reflective monument
	if (!depthPass) {
		glUniform1f(glGetUniformLocation(shader.shaderProgram, "reflectiveFlag"), 1.0f);
	}
	monument.Draw(shader);
	if (!depthPass) {
		glUniform1f(glGetUniformLocation(shader.shaderProgram, "reflectiveFlag"), 0.0f);
	}

	//draw bridge gate
	glm::mat4 modelAux = model;
	glm::vec3 bridgePoint = glm::vec3(-1.3194f, 0.58868f, 4.6881f);

	modelAux = glm::translate(modelAux, bridgePoint);
	modelAux = glm::rotate(modelAux, glm::radians(23.3f), glm::vec3(0, 1, 0));
	modelAux = glm::rotate(modelAux, glm::radians(bridgeAngle), glm::vec3(1.0f, 0.0f, 0.0f));
	modelAux = glm::rotate(modelAux, glm::radians(-23.3f), glm::vec3(0, 1, 0));
	modelAux = glm::translate(modelAux, -bridgePoint);

	glUniformMatrix4fv(glGetUniformLocation(shader.shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(modelAux));
	if (!depthPass) {
		normalMatrix = glm::mat3(glm::inverseTranspose(view * modelAux));
		glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(glm::mat3(glm::inverseTranspose(normalMatrix))));
	}
	castleBridge.Draw(shader);

	//draw mill 
	modelAux = model;
	glm::vec3 millPoint = glm::vec3(1.584f, 1.152f, 7.912f);

	modelAux = glm::translate(modelAux, millPoint);
	modelAux = glm::rotate(modelAux, glm::radians(millAngle), glm::vec3(0, 0, 1));
	modelAux = glm::translate(modelAux, -millPoint);
	
	glUniformMatrix4fv(glGetUniformLocation(shader.shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(modelAux));
	if (!depthPass) {
		normalMatrix = glm::mat3(glm::inverseTranspose(view * modelAux));
		glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(glm::mat3(glm::inverseTranspose(normalMatrix))));
	}
	glDisable(GL_CULL_FACE);
	mill.Draw(shader);
	glEnable(GL_CULL_FACE);
	//draw gates
	modelAux = model;
	glm::vec3 gate0Point = glm::vec3(-0.134f, 0.6211f, 12.46f);

	modelAux = glm::translate(modelAux, gate0Point);
	modelAux = glm::rotate(modelAux, glm::radians(-gateAngle), glm::vec3(0, 1, 0));
	modelAux = glm::translate(modelAux, -gate0Point);

	glUniformMatrix4fv(glGetUniformLocation(shader.shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(modelAux));
	if (!depthPass) {
		normalMatrix = glm::mat3(glm::inverseTranspose(view * modelAux));
		glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(glm::mat3(glm::inverseTranspose(normalMatrix))));
	}
	gate[0].Draw(shader);

	modelAux = model;
	glm::vec3 gate1Point = glm::vec3(-0.7941f, 0.6211f, 12.46f);

	modelAux = glm::translate(modelAux, gate1Point);
	modelAux = glm::rotate(modelAux, glm::radians(gateAngle), glm::vec3(0, 1, 0));
	modelAux = glm::translate(modelAux, -gate1Point);

	glUniformMatrix4fv(glGetUniformLocation(shader.shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(modelAux));
	if (!depthPass) {
		normalMatrix = glm::mat3(glm::inverseTranspose(view * modelAux));
		glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(glm::mat3(glm::inverseTranspose(normalMatrix))));
	}
	gate[1].Draw(shader);

	modelAux = model;
	glm::vec3 gate2Point = glm::vec3(-1.737f, 0.6211f, 12.46f);

	modelAux = glm::translate(modelAux, gate2Point);
	modelAux = glm::rotate(modelAux, glm::radians(gateAngle), glm::vec3(0, 1, 0));
	modelAux = glm::translate(modelAux, -gate2Point);

	glUniformMatrix4fv(glGetUniformLocation(shader.shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(modelAux));
	if (!depthPass) {
		normalMatrix = glm::mat3(glm::inverseTranspose(view * modelAux));
		glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(glm::mat3(glm::inverseTranspose(normalMatrix))));
	}
	gate[2].Draw(shader);
	
	//draw ducks
	for (int i = 0; i < DUCK_NO; i++) {
		drawDuck(shader, depthPass, i);
	}

	//DRAW TRANSPARENT OBJS
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	if (!depthPass) {
		glUniform1f(glGetUniformLocation(shader.shaderProgram, "transparentFlag"), 1.0f);
	}

	//draw river
	glUniformMatrix4fv(glGetUniformLocation(shader.shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));

	if (!depthPass) {
		normalMatrix = glm::mat3(glm::inverseTranspose(view * model));
		glUniformMatrix3fv(glGetUniformLocation(shader.shaderProgram, "normalMatrix"), 1, GL_FALSE, glm::value_ptr(normalMatrix));
	}
	river.Draw(shader);

	//draw rain
	for (int i = 0; i < DROPLET_NO; i++) {
		drawDroplet(shader, depthPass, i);
	}
	if (!depthPass) {
		glUniform1f(glGetUniformLocation(shader.shaderProgram, "transparentFlag"), 0.0f);
	}
	glDisable(GL_BLEND);
 }

void renderScene() {

	//render the scene in the depth map
	depthMapShader.useShaderProgram();
	glUniformMatrix4fv(glGetUniformLocation(depthMapShader.shaderProgram, "lightSpaceTrMatrix"),
		1,
		GL_FALSE,
		glm::value_ptr(computeLightSpaceTrMatrix()));

	glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);
	glBindFramebuffer(GL_FRAMEBUFFER, shadowMapFBO);
	glClear(GL_DEPTH_BUFFER_BIT);
	drawObjects(depthMapShader, true);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	glViewport(0, 0, retina_width, retina_height);

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	myCustomShader.useShaderProgram();

	view = myCamera.getViewMatrix();
	glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
	
	lightRotation = glm::rotate(glm::mat4(1.0f), glm::radians(lightAngle), glm::vec3(0, 1, 0));
	glUniform3fv(lightDirLoc, 1, glm::value_ptr(glm::inverseTranspose(glm::mat3(view * lightRotation)) * lightDir));

	//bind the shadow map
	glActiveTexture(GL_TEXTURE3);
	glBindTexture(GL_TEXTURE_2D, depthMapTexture);
	glUniform1i(glGetUniformLocation(myCustomShader.shaderProgram, "shadowMap"), 3);

	glUniformMatrix4fv(glGetUniformLocation(myCustomShader.shaderProgram, "lightSpaceTrMatrix"),
		1,
		GL_FALSE,
		glm::value_ptr(computeLightSpaceTrMatrix()));

	fogFactorLoc = glGetUniformLocation(myCustomShader.shaderProgram, "fogDensity");
	glUniform1f(fogFactorLoc, fogFactor);

	drawObjects(myCustomShader, false);

	//draw a white cube around the light
	lightShader.useShaderProgram();

	glUniformMatrix4fv(glGetUniformLocation(lightShader.shaderProgram, "view"), 1, GL_FALSE, glm::value_ptr(view));

	model = lightRotation;
	model = glm::translate(model, 1.0f * lightDir);
	model = glm::scale(model, glm::vec3(0.05f, 0.05f, 0.05f));
	glUniformMatrix4fv(glGetUniformLocation(lightShader.shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));

	lightCube.Draw(lightShader);

	//draw skybox
	skyboxShader.useShaderProgram();
	view = myCamera.getViewMatrix();
	glUniformMatrix4fv(glGetUniformLocation(skyboxShader.shaderProgram, "view"), 1, GL_FALSE,
		glm::value_ptr(view));

	projection = glm::perspective(glm::radians(45.0f), (float)retina_width / (float)retina_height, 0.1f, 1000.0f);
	glUniformMatrix4fv(glGetUniformLocation(skyboxShader.shaderProgram, "projection"), 1, GL_FALSE,
		glm::value_ptr(projection));

	mySkyBox.Draw(skyboxShader, view, projection);
	
}

void cleanup() {
	glDeleteTextures(1,& depthMapTexture);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glDeleteFramebuffers(1, &shadowMapFBO);
	glfwDestroyWindow(glWindow);
	//close GL context and any other GLFW resources
	glfwTerminate();
}

int main(int argc, const char * argv[]) {

	if (!initOpenGLWindow()) {
		glfwTerminate();
		return 1;
	}
	initOpenGLState();
	initObjects();
	initShaders();
	initUniforms();
	initFBO();
	initSkybox();
	initBezierCurves();
	initDroplets();
	float interval = 1;
	start = clock();
	
	glCheckError();
	FILE* pf = fopen("cameraLog.txt", "r");
	if (pf == NULL) {
		puts("Error Opening File!");
	}
	while (!glfwWindowShouldClose(glWindow)) {
		processMovement();

		//animation handling
		end = clock();
		if ((end - start)  > interval) {
			//rain
			if (startRain) {
				rainMovement();
			}
			//ducks
			duckMovement(0.001f);
			//mill
			millAngle += 4.0f;
			//camera
			myCamera.preview(cameraPreview, pf);
			start = end;
		}
		renderScene();
		glfwPollEvents();
		glfwSwapBuffers(glWindow);
		glCheckError();
	}
	cleanup();
	fclose(pf);
	return 0;
}
