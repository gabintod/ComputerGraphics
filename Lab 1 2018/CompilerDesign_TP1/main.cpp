// Windows includes (For Time, IO, etc.)
#include <windows.h>
#include <mmsystem.h>
#include <iostream>
#include <string>
#include <stdio.h>
#include <math.h>
#include <vector> // STL dynamic memory.

// OpenGL includes
#include <GL/glew.h>
#include <GL/freeglut.h>

// Assimp includes
#include <assimp/cimport.h> // scene importer
#include <assimp/scene.h> // collects data
#include <assimp/postprocess.h> // various extra operations

// Project includes
#include "maths_funcs.h"



/*----------------------------------------------------------------------------
MESH TO LOAD
----------------------------------------------------------------------------*/
// this mesh is a dae file format but you should be able to use any other format too, obj is typically what is used
// put the mesh in your project directory, or provide a filepath for it here
#define MESH_NAME "C:/Users/Gabin/Documents/I4S/TrinityCollege/ComputerGraphics/ComputerGraphics/Lab 1 2018/CompilerDesign_TP1/CompilerDesign_TP1/skulltruck.dae"  // monkeyhead_smooth.dae"
#define SUBMESH_NAME "C:/Users/Gabin/Documents/I4S/TrinityCollege/ComputerGraphics/ComputerGraphics/Lab 1 2018/CompilerDesign_TP1/CompilerDesign_TP1/skulltruckdoor.dae"
/*----------------------------------------------------------------------------
----------------------------------------------------------------------------*/

#pragma region SimpleTypes
typedef struct
{
	size_t mPointCount = 0;
	std::vector<vec3> mVertices;
	std::vector<vec3> mNormals;
	std::vector<vec2> mTextureCoords;
} ModelData;
#pragma endregion SimpleTypes

typedef struct
{
	const char* mesh_name;
	ModelData mesh_data;
	unsigned int vp_vbo;
	unsigned int vn_vbo;
	unsigned int vao;
	GLuint program_id;
} ObjectBufferMesh;

ObjectBufferMesh sktr_body;
ObjectBufferMesh sktr_left_door;
ObjectBufferMesh sktr_right_door;

using namespace std;
GLuint shaderProgramID;

ModelData mesh_data;
unsigned int mesh_vao = 0;
int width = 800;
int height = 600;

GLuint loc1, loc2, loc3;
GLfloat rotate_y = 0.0f;


#define MOUSE_NONE 0
#define MOUSE_LEFT 1
#define MOUSE_CENTER 2
#define MOUSE_RIGHT 3

GLfloat tX, tY, tZ;
GLfloat aX, aY;
GLfloat scaleValue = 1;
GLfloat doorAngle = 0;
int pmouseX, pmouseY;
int pressedMouse = MOUSE_NONE;

#pragma region MESH LOADING
/*----------------------------------------------------------------------------
MESH LOADING FUNCTION
----------------------------------------------------------------------------*/

ModelData load_mesh(const char* file_name) {
	ModelData modelData;

	/* Use assimp to read the model file, forcing it to be read as    */
	/* triangles. The second flag (aiProcess_PreTransformVertices) is */
	/* relevant if there are multiple meshes in the model file that   */
	/* are offset from the origin. This is pre-transform them so      */
	/* they're in the right position.                                 */
	const aiScene* scene = aiImportFile(
		file_name, 
		aiProcess_Triangulate | aiProcess_PreTransformVertices
	); 

	if (!scene) {
		fprintf(stderr, "ERROR: reading mesh %s\n", file_name);
		return modelData;
	}

	printf("  %i materials\n", scene->mNumMaterials);
	printf("  %i meshes\n", scene->mNumMeshes);
	printf("  %i textures\n", scene->mNumTextures);

	for (unsigned int m_i = 0; m_i < scene->mNumMeshes; m_i++) {
		const aiMesh* mesh = scene->mMeshes[m_i];
		printf("    %i vertices in mesh\n", mesh->mNumVertices);
		modelData.mPointCount += mesh->mNumVertices;
		for (unsigned int v_i = 0; v_i < mesh->mNumVertices; v_i++) {
			if (mesh->HasPositions()) {
				const aiVector3D* vp = &(mesh->mVertices[v_i]);
				modelData.mVertices.push_back(vec3(vp->x, vp->y, vp->z));
			}
			if (mesh->HasNormals()) {
				const aiVector3D* vn = &(mesh->mNormals[v_i]);
				modelData.mNormals.push_back(vec3(vn->x, vn->y, vn->z));
			}
			if (mesh->HasTextureCoords(0)) {
				const aiVector3D* vt = &(mesh->mTextureCoords[0][v_i]);
				modelData.mTextureCoords.push_back(vec2(vt->x, vt->y));
			}
			if (mesh->HasTangentsAndBitangents()) {
				/* You can extract tangents and bitangents here              */
				/* Note that you might need to make Assimp generate this     */
				/* data for you. Take a look at the flags that aiImportFile  */
				/* can take.                                                 */
			}
		}
	}

	aiReleaseImport(scene);
	return modelData;
}

#pragma endregion MESH LOADING

// Shader Functions- click on + to expand
#pragma region SHADER_FUNCTIONS
char* readShaderSource(const char* shaderFile) {
	FILE* fp;
	fopen_s(&fp, shaderFile, "rb");

	if (fp == NULL) { return NULL; }

	fseek(fp, 0L, SEEK_END);
	long size = ftell(fp);

	fseek(fp, 0L, SEEK_SET);
	char* buf = new char[size + 1];
	fread(buf, 1, size, fp);
	buf[size] = '\0';

	fclose(fp);

	return buf;
}


static void AddShader(GLuint ShaderProgram, const char* pShaderText, GLenum ShaderType)
{
	// create a shader object
	GLuint ShaderObj = glCreateShader(ShaderType);

	if (ShaderObj == 0) {
		std::cerr << "Error creating shader..." << std::endl;
		std::cerr << "Press enter/return to exit..." << std::endl;
		std::cin.get();
		exit(1);
	}
	const char* pShaderSource = readShaderSource(pShaderText);

	// Bind the source code to the shader, this happens before compilation
	glShaderSource(ShaderObj, 1, (const GLchar**)&pShaderSource, NULL);
	// compile the shader and check for errors
	glCompileShader(ShaderObj);
	GLint success;
	// check for shader related errors using glGetShaderiv
	glGetShaderiv(ShaderObj, GL_COMPILE_STATUS, &success);
	if (!success) {
		GLchar InfoLog[1024] = { '\0' };
		glGetShaderInfoLog(ShaderObj, 1024, NULL, InfoLog);
		std::cerr << "Error compiling "
			<< (ShaderType == GL_VERTEX_SHADER ? "vertex" : "fragment")
			<< " shader program: " << InfoLog << std::endl;
		std::cerr << "Press enter/return to exit..." << std::endl;
		std::cin.get();
		exit(1);
	}
	// Attach the compiled shader object to the program object
	glAttachShader(ShaderProgram, ShaderObj);
}

GLuint CompileShaders()
{
	//Start the process of setting up our shaders by creating a program ID
	//Note: we will link all the shaders together into this ID
	shaderProgramID = glCreateProgram();
	if (shaderProgramID == 0) {
		std::cerr << "Error creating shader program..." << std::endl;
		std::cerr << "Press enter/return to exit..." << std::endl;
		std::cin.get();
		exit(1);
	}

	// Create two shader objects, one for the vertex, and one for the fragment shader
	AddShader(shaderProgramID, "C:/Users/Gabin/Documents/I4S/TrinityCollege/ComputerGraphics/ComputerGraphics/Lab 1 2018/CompilerDesign_TP1/CompilerDesign_TP1/simpleVertexShader.txt", GL_VERTEX_SHADER);
	AddShader(shaderProgramID, "C:/Users/Gabin/Documents/I4S/TrinityCollege/ComputerGraphics/ComputerGraphics/Lab 1 2018/CompilerDesign_TP1/CompilerDesign_TP1/simpleFragmentShader.txt", GL_FRAGMENT_SHADER);

	GLint Success = 0;
	GLchar ErrorLog[1024] = { '\0' };
	// After compiling all shader objects and attaching them to the program, we can finally link it
	glLinkProgram(shaderProgramID);
	// check for program related errors using glGetProgramiv
	glGetProgramiv(shaderProgramID, GL_LINK_STATUS, &Success);
	if (Success == 0) {
		glGetProgramInfoLog(shaderProgramID, sizeof(ErrorLog), NULL, ErrorLog);
		std::cerr << "Error linking shader program: " << ErrorLog << std::endl;
		std::cerr << "Press enter/return to exit..." << std::endl;
		std::cin.get();
		exit(1);
	}

	// program has been successfully linked but needs to be validated to check whether the program can execute given the current pipeline state
	glValidateProgram(shaderProgramID);
	// check for program related errors using glGetProgramiv
	glGetProgramiv(shaderProgramID, GL_VALIDATE_STATUS, &Success);
	if (!Success) {
		glGetProgramInfoLog(shaderProgramID, sizeof(ErrorLog), NULL, ErrorLog);
		std::cerr << "Invalid shader program: " << ErrorLog << std::endl;
		std::cerr << "Press enter/return to exit..." << std::endl;
		std::cin.get();
		exit(1);
	}
	// Finally, use the linked shader program
	// Note: this program will stay in effect for all draw calls until you replace it with another or explicitly disable its use
	glUseProgram(shaderProgramID);
	return shaderProgramID;
}
#pragma endregion SHADER_FUNCTIONS

// VBO Functions - click on + to expand
#pragma region VBO_FUNCTIONS
ObjectBufferMesh generateObjectBufferMesh(ObjectBufferMesh obm) {
	/*----------------------------------------------------------------------------
	LOAD MESH HERE AND COPY INTO BUFFERS
	----------------------------------------------------------------------------*/

	//Note: you may get an error "vector subscript out of range" if you are using this code for a mesh that doesnt have positions and normals
	//Might be an idea to do a check for that before generating and binding the buffer.

	mesh_data = load_mesh(obm.mesh_name);
	unsigned int vp_vbo = 0;
	loc1 = glGetAttribLocation(obm.program_id, "vertex_position");
	loc2 = glGetAttribLocation(obm.program_id, "vertex_normal");
	loc3 = glGetAttribLocation(obm.program_id, "vertex_texture");

	glGenBuffers(1, &vp_vbo);
	glBindBuffer(GL_ARRAY_BUFFER, vp_vbo);
	glBufferData(GL_ARRAY_BUFFER, mesh_data.mPointCount * sizeof(vec3), &mesh_data.mVertices[0], GL_STATIC_DRAW);
	unsigned int vn_vbo = 0;
	glGenBuffers(1, &vn_vbo);
	glBindBuffer(GL_ARRAY_BUFFER, vn_vbo);
	glBufferData(GL_ARRAY_BUFFER, mesh_data.mPointCount * sizeof(vec3), &mesh_data.mNormals[0], GL_STATIC_DRAW);

	//	This is for texture coordinates which you don't currently need, so I have commented it out
	//	unsigned int vt_vbo = 0;
	//	glGenBuffers (1, &vt_vbo);
	//	glBindBuffer (GL_ARRAY_BUFFER, vt_vbo);
	//	glBufferData (GL_ARRAY_BUFFER, monkey_head_data.mTextureCoords * sizeof (vec2), &monkey_head_data.mTextureCoords[0], GL_STATIC_DRAW);

	unsigned int vao = 0;
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);

	glEnableVertexAttribArray(loc1);
	glBindBuffer(GL_ARRAY_BUFFER, vp_vbo);
	glVertexAttribPointer(loc1, 3, GL_FLOAT, GL_FALSE, 0, NULL);
	glEnableVertexAttribArray(loc2);
	glBindBuffer(GL_ARRAY_BUFFER, vn_vbo);
	glVertexAttribPointer(loc2, 3, GL_FLOAT, GL_FALSE, 0, NULL);

	//	This is for texture coordinates which you don't currently need, so I have commented it out
	//	glEnableVertexAttribArray (loc3);
	//	glBindBuffer (GL_ARRAY_BUFFER, vt_vbo);
	//	glVertexAttribPointer (loc3, 2, GL_FLOAT, GL_FALSE, 0, NULL);

	obm.mesh_data = mesh_data;
	obm.vp_vbo = vp_vbo;
	obm.vn_vbo = vn_vbo;
	obm.vao = vao;

	cout << "OBM CREATED :\n\tmesh name : " << obm.mesh_name << "\n\tvp vbo : " << obm.vp_vbo << "\n\tvn vbo : " << obm.vn_vbo << "\n\tvao : " << obm.vao << "\n";

	return obm;
}
#pragma endregion VBO_FUNCTIONS

void bindObjectBufferMesh(ObjectBufferMesh obm)
{
	glUseProgram(obm.program_id);
	glBindVertexArray(obm.vao);
}

void display() {

	// tell GL to only draw onto a pixel if the shape is closer to the viewer
	glEnable(GL_DEPTH_TEST); // enable depth-testing
	glDepthFunc(GL_LESS); // depth-testing interprets a smaller value as "closer"
	glClearColor(0.5f, 0.5f, 0.5f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	bindObjectBufferMesh(sktr_body);

	//Declare your uniform variables that will be used in your shader
	int matrix_location = glGetUniformLocation(sktr_body.program_id, "model");
	int view_mat_location = glGetUniformLocation(sktr_body.program_id, "view");
	int proj_mat_location = glGetUniformLocation(sktr_body.program_id, "proj");


	// Root of the Hierarchy
	mat4 view = identity_mat4();
	mat4 persp_proj = perspective(45.0f, (float)width / (float)height, 0.1f, 1000.0f);
	mat4 model = identity_mat4();
	model = rotate_z_deg(model, rotate_y);
	model = rotate_x_deg(model, aX);
	model = rotate_y_deg(model, aY);
	model = scale(model, vec3(scaleValue, scaleValue, scaleValue));
	view = translate(view, vec3(tX, tY, tZ));

	// update uniforms & draw
	glUniformMatrix4fv(proj_mat_location, 1, GL_FALSE, persp_proj.m);
	glUniformMatrix4fv(view_mat_location, 1, GL_FALSE, view.m);
	glUniformMatrix4fv(matrix_location, 1, GL_FALSE, model.m);
	glDrawArrays(GL_TRIANGLES, 0, sktr_body.mesh_data.mPointCount);


	// LEFT DOOR
	bindObjectBufferMesh(sktr_left_door);
	// Reset uniform variables for shader
	matrix_location = glGetUniformLocation(sktr_left_door.program_id, "model");
	view_mat_location = glGetUniformLocation(sktr_left_door.program_id, "view");
	proj_mat_location = glGetUniformLocation(sktr_left_door.program_id, "proj");

	// Set up the child matrix
	mat4 modelChild = identity_mat4();
	modelChild = translate(modelChild, vec3(-1.1f, 0.0f, -0.6f));
	modelChild = rotate_y_deg(modelChild, -max(doorAngle, 0.0f));
	modelChild = translate(modelChild, vec3(1.1f, 0.0f, 0.6f));

	// Apply the root matrix to the child matrix
	modelChild = model * modelChild;

	// Update the appropriate uniform and draw the mesh again
	glUniformMatrix4fv(proj_mat_location, 1, GL_FALSE, persp_proj.m);
	glUniformMatrix4fv(view_mat_location, 1, GL_FALSE, view.m);
	glUniformMatrix4fv(matrix_location, 1, GL_FALSE, modelChild.m);
	glDrawArrays(GL_TRIANGLES, 0, sktr_left_door.mesh_data.mPointCount);

	// RIGHT DOOR
	bindObjectBufferMesh(sktr_right_door);
	// Reset uniform variables for shader
	matrix_location = glGetUniformLocation(sktr_right_door.program_id, "model");
	view_mat_location = glGetUniformLocation(sktr_right_door.program_id, "view");
	proj_mat_location = glGetUniformLocation(sktr_right_door.program_id, "proj");

	// Set up the child matrix
	modelChild = identity_mat4();
	modelChild = scale(modelChild, vec3(-1.0f, 1.0f, 1.0f));
	modelChild = translate(modelChild, vec3(1.1f, 0.0f, -0.6f));
	modelChild = rotate_y_deg(modelChild, -min(doorAngle, 0.0f));
	modelChild = translate(modelChild, vec3(-1.1f, 0.0f, 0.6f));

	// Apply the root matrix to the child matrix
	modelChild = model * modelChild;

	// Update the appropriate uniform and draw the mesh again
	glUniformMatrix4fv(proj_mat_location, 1, GL_FALSE, persp_proj.m);
	glUniformMatrix4fv(view_mat_location, 1, GL_FALSE, view.m);
	glUniformMatrix4fv(matrix_location, 1, GL_FALSE, modelChild.m);
	glDrawArrays(GL_TRIANGLES, 0, sktr_right_door.mesh_data.mPointCount);
	
	glutSwapBuffers();
}


void updateScene() {

	static DWORD last_time = 0;
	DWORD curr_time = timeGetTime();
	if (last_time == 0)
		last_time = curr_time;
	float delta = (curr_time - last_time) * 0.001f;
	last_time = curr_time;

	// Rotate the model slowly around the y axis at 20 degrees per second
	/*rotate_y += 20.0f * delta;
	rotate_y = fmodf(rotate_y, 360.0f);*/

	// Draw the next frame
	glutPostRedisplay();
}


void init()
{
	sktr_body.mesh_name = MESH_NAME;
	sktr_left_door.mesh_name = SUBMESH_NAME;
	sktr_right_door.mesh_name = SUBMESH_NAME;
	// Set up the shaders
	GLuint shaderProgramID = CompileShaders();
	sktr_body.program_id = shaderProgramID;
	sktr_left_door.program_id = shaderProgramID;
	sktr_right_door.program_id = shaderProgramID;
	// load mesh into a vertex buffer array
	sktr_body = generateObjectBufferMesh(sktr_body);
	sktr_left_door = generateObjectBufferMesh(sktr_left_door);
	sktr_right_door = generateObjectBufferMesh(sktr_right_door);

	tX = 0.0f;
	tY = 0.0f;
	tZ = -10.0f;

	aX = 0.0f;
	aY = 0.0f;

	scaleValue = 1;
	doorAngle = 0;
}

// Placeholder code for the keypress
void keypress(unsigned char key, int x, int y) {
	if (key == 'x') {
		//Translate the base, etc.
		tX += 0.1f;
	}
	if (key == 'X') {
		//Translate the base, etc.
		tX -= 0.1f;
	}
	if (key == 'y') {
		//Translate the base, etc.
		tY += 0.1f;
	}
	if (key == 'Y') {
		//Translate the base, etc.
		tY -= 0.1f;
	}
	if (key == 'z') {
		//Translate the base, etc.
		tZ += 0.1f;
	}
	if (key == 'Z') {
		//Translate the base, etc.
		tZ -= 0.1f;
	}
	if (key == 'r')
	{
		tX = 0.0f;
		tY = 0.0f;
		tZ = -10.0f;

		aX = 0.0f;
		aY = 0.0f;

		scaleValue = 1;
		doorAngle = 0;
	}
}

void mousePress(int button, int state, int x, int y)
{
	if (state == GLUT_DOWN)
	{
		pmouseX = x;
		pmouseY = y;

		if (button == GLUT_LEFT_BUTTON)
			pressedMouse = MOUSE_LEFT;
		if (button == GLUT_MIDDLE_BUTTON)
			pressedMouse = MOUSE_CENTER;
		if (button == GLUT_RIGHT_BUTTON)
			pressedMouse = MOUSE_RIGHT;
		if (button == 3)
			scaleValue += 0.1;
		if (button == 4)
			scaleValue -= 0.1;
	}
	if (state == GLUT_UP)
		pressedMouse = MOUSE_NONE;
}

void mouseDrag(int x, int y)
{
	if (pressedMouse == MOUSE_LEFT)
	{
		aX -= float(pmouseY - y) / 10 * 3.14;
		aY -= float(pmouseX - x) / 10 * 3.14;
	}
	if (pressedMouse == MOUSE_RIGHT)
	{
		tX -= float(pmouseX - x)/100;
		tY += float(pmouseY - y)/100;
	}
	if (pressedMouse == MOUSE_CENTER)
	{
		doorAngle = max(min(doorAngle + float(x-pmouseX)/2, 60.0f), -60.0f);
	}

	pmouseX = x;
	pmouseY = y;
}

int main(int argc, char** argv) {

	// Set up the window
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);
	glutInitWindowSize(width, height);
	glutCreateWindow("Computer Graphics TP4");

	// Tell glut where the display function is
	glutDisplayFunc(display);
	glutIdleFunc(updateScene);
	glutKeyboardFunc(keypress);
	glutMouseFunc(mousePress);
	glutMotionFunc(mouseDrag);

	// A call to glewInit() must be done after glut is initialized!
	GLenum res = glewInit();
	// Check for any errors
	if (res != GLEW_OK) {
		fprintf(stderr, "Error: '%s'\n", glewGetErrorString(res));
		return 1;
	}
	// Set up your objects and shaders
	init();

	// Begin infinite event loop
	glutMainLoop();
	return 0;
}


// ---------------------------------------------- Personal GT classes -----------------------------------------

//	GTObject : every object in the scene with position and parent
class GTObject
{
private:
	mat4 position;
	GTObject* parent;

public:
	GTObject()
	{
		position = identity_mat4();
	}

	mat4 getPosition()
	{
		return position;
	}

	mat4 getRelativePosition()
	{
		return (parent != NULL) ? position * (*parent).getRelativePosition() : position;
	}

	void move(float x, float y, float z)
	{
		position = translate(position, vec3(x, y, z));
	}

	void turnX(float x)
	{
		position = rotate_x_deg(position, x);
	}

	void turnY(float y)
	{
		position = rotate_y_deg(position, y);
	}

	void turnZ(float z)
	{
		position = rotate_z_deg(position, z);
	}

	void resize(float x, float y, float z)
	{
		position = scale(position, vec3(x, y, z));
	}

	void setPosition(mat4 p)
	{
		position = p;
	}
};


//	GTShaderProgram : a program of shaders with an id
class GTShaderProgram
{
private:
	unsigned int id;

public:
	GTShaderProgram()
	{
		id = glCreateProgram();
		if (id == 0) {
			cerr << "Error creating shader program..." << endl;
			cerr << "Press enter/return to exit..." << endl;
			cin.get();
			exit(1);
		}
	}

	void addShader(const char* path, GLenum type)
	{
		AddShader(id, path, type);
	}

	void compile()
	{
		GLint Success = 0;
		GLchar ErrorLog[1024] = { '\0' };
		// After compiling all shader objects and attaching them to the program, we can finally link it
		glLinkProgram(id);
		// check for program related errors using glGetProgramiv
		glGetProgramiv(id, GL_LINK_STATUS, &Success);
		if (Success == 0)
		{
			glGetProgramInfoLog(id, sizeof(ErrorLog), NULL, ErrorLog);
			cerr << "Error linking shader program: " << ErrorLog << endl;
			cerr << "Press enter/return to exit..." << endl;
			cin.get();
			exit(1);
		}

		// program has been successfully linked but needs to be validated to check whether the program can execute given the current pipeline state
		glValidateProgram(id);
		// check for program related errors using glGetProgramiv
		glGetProgramiv(id, GL_VALIDATE_STATUS, &Success);
		if (!Success)
		{
			glGetProgramInfoLog(id, sizeof(ErrorLog), NULL, ErrorLog);
			cerr << "Invalid shader program: " << ErrorLog << endl;
			cerr << "Press enter/return to exit..." << endl;
			cin.get();
			exit(1);
		}
	}

	unsigned int getId()
	{
		return id;
	}
};


//	GTCamera : camera object
class GTCamera : public GTObject
{
private:
	mat4 projection;

public:
	GTCamera()
	{
		projection = perspective(45.0f, (float)width / (float)height, 0.1f, 1000.0f);
	}

	mat4 getProjection()
	{
		return projection;
	}
};


//	GTModel : Object to display with a mesh, vao, programId...
class GTModel : public GTObject
{
private:
	const char* meshFilePath;
	ModelData meshData;
	unsigned int vpVBO = 0;
	unsigned int vnVBO = 0;
	unsigned int VAO = 0;
	GTShaderProgram* shaderProgram;
	GTCamera* camera;

public:
	GTModel(const char* path, GTShaderProgram* sp, GTCamera* cam)
	{
		meshFilePath = path;
		shaderProgram = sp;
		camera = cam;

		meshData = load_mesh(path);
		GLuint loc1 = glGetAttribLocation((*sp).getId(), "vertex_position");
		GLuint loc2 = glGetAttribLocation((*sp).getId(), "vertex_normal");
		GLuint loc3 = glGetAttribLocation((*sp).getId(), "vertex_texture");

		glGenBuffers(1, &vpVBO);
		glBindBuffer(GL_ARRAY_BUFFER, vpVBO);
		glBufferData(GL_ARRAY_BUFFER, meshData.mPointCount * sizeof(vec3), &meshData.mVertices[0], GL_STATIC_DRAW);
		glGenBuffers(1, &vnVBO);
		glBindBuffer(GL_ARRAY_BUFFER, vnVBO);
		glBufferData(GL_ARRAY_BUFFER, meshData.mPointCount * sizeof(vec3), &meshData.mNormals[0], GL_STATIC_DRAW);

		glGenVertexArrays(1, &VAO);
		glBindVertexArray(VAO);

		glEnableVertexAttribArray(loc1);
		glBindBuffer(GL_ARRAY_BUFFER, vpVBO);
		glVertexAttribPointer(loc1, 3, GL_FLOAT, GL_FALSE, 0, NULL);
		glEnableVertexAttribArray(loc2);
		glBindBuffer(GL_ARRAY_BUFFER, vnVBO);
		glVertexAttribPointer(loc2, 3, GL_FLOAT, GL_FALSE, 0, NULL);
	}

	void display()
	{
		glUseProgram((*shaderProgram).getId());
		glBindVertexArray(VAO);

		//Declare your uniform variables that will be used in your shader
		int matrix_location = glGetUniformLocation((*shaderProgram).getId(), "model");
		int view_mat_location = glGetUniformLocation((*shaderProgram).getId(), "view");
		int proj_mat_location = glGetUniformLocation((*shaderProgram).getId(), "proj");

		// update uniforms & draw
		glUniformMatrix4fv(proj_mat_location, 1, GL_FALSE, (*camera).getProjection().m);
		glUniformMatrix4fv(view_mat_location, 1, GL_FALSE, (*camera).getRelativePosition().m);
		glUniformMatrix4fv(matrix_location, 1, GL_FALSE, getRelativePosition().m);
		glDrawArrays(GL_TRIANGLES, 0, meshData.mPointCount);
	}
};